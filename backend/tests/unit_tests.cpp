// Unit tests for everything that does not need a running HTTP server.
#include <drogon/drogon_test.h>

#include <map>
#include <string>

#include "Config.h"
#include "http/CorsPolicy.h"
#include "service/GridValidation.h"
#include "service/ResultMapper.h"
#include "service/SudokuService.h"
#include "solver/DlxSolver.h"

using namespace dlx;

namespace {

Grid gridFrom(const std::string& s) {
    Grid g{};
    for (int i = 0; i < 81; ++i) g[i / 9][i % 9] = (s[i] == '.') ? 0 : s[i] - '0';
    return g;
}

std::string toString(const Grid& g) {
    std::string s;
    for (const auto& row : g)
        for (int v : row) s += static_cast<char>('0' + v);
    return s;
}

Json::Value jsonGrid(const Grid& g) {
    Json::Value rows(Json::arrayValue);
    for (const auto& row : g) {
        Json::Value r(Json::arrayValue);
        for (int v : row) r.append(v);
        rows.append(r);
    }
    Json::Value body(Json::objectValue);
    body["grid"] = rows;
    return body;
}

const char* kWikiPuzzle =
    "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
const char* kWikiSolution =
    "534678912672195348198342567859761423426853791713924856961537284287419635345286179";
const char* kHardPuzzle =  // Arto Inkala, "world's hardest sudoku"
    "800000000003600000070090200050007000000045700000100030001000068008500010090000400";
const char* kHardSolution =
    "812753649943682175675491283154237896369845721287169534521974368438526917796318452";
const std::string kEmpty(81, '0');

}  // namespace

// ---------------------------------------------------------------- solver

DROGON_TEST(SolverSolvesKnownPuzzle) {
    DlxSolver solver;
    auto r = solver.solve(gridFrom(kWikiPuzzle));
    CHECK(r.outcome == Outcome::Solved);
    CHECK(toString(r.solution) == std::string(kWikiSolution));
    CHECK(r.solutionCount == 1);
    CHECK(!r.solutionLimitReached);
    CHECK(r.stats.recursiveCalls > 0);
    CHECK(r.stats.nodesVisited > 0);
    CHECK(r.elapsedMs >= 0.0);
}

DROGON_TEST(SolverSolvesHardPuzzleUniquely) {
    DlxSolver solver;
    auto r = solver.solve(gridFrom(kHardPuzzle));
    CHECK(r.outcome == Outcome::Solved);
    CHECK(toString(r.solution) == std::string(kHardSolution));
    CHECK(r.solutionCount == 1);
    CHECK(r.stats.backtracks > 0);  // genuinely needs search
}

DROGON_TEST(SolverKeepsAllClues) {
    DlxSolver solver;
    const Grid puzzle = gridFrom(kHardPuzzle);
    auto r = solver.solve(puzzle);
    bool ok = true;
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            if (puzzle[i][j] != 0 && puzzle[i][j] != r.solution[i][j]) ok = false;
    CHECK(ok);
}

DROGON_TEST(SolverReportsMultipleSolutionsOnEmptyGrid) {
    DlxSolver solver;
    auto r = solver.solve(gridFrom(kEmpty));
    CHECK(r.outcome == Outcome::Solved);
    CHECK(r.solutionCount == 2);
    CHECK(r.solutionLimitReached);
    CHECK(findClueConflict(r.solution) == std::nullopt);
}

DROGON_TEST(SolverHonoursMaxSolutions) {
    SolveOptions o;
    o.maxSolutions = 5;
    DlxSolver solver(o);
    auto r = solver.solve(gridFrom(kEmpty));
    CHECK(r.solutionCount == 5);
    CHECK(r.solutionLimitReached);
}

DROGON_TEST(SolverDetectsNoSolutionWithoutDuplicateClues) {
    // Row 1 holds 1..8, so (1,9) needs a 9, but column 9 already has one at row 2.
    DlxSolver solver;
    auto r = solver.solve(gridFrom("123456780"
                                   "000000009" + std::string(63, '0')));
    CHECK(r.outcome == Outcome::NoSolution);
    CHECK(r.solutionCount == 0);
}

DROGON_TEST(SolverTreatsContradictoryCluesAsNoSolution) {
    DlxSolver solver;
    auto r = solver.solve(gridFrom("55" + std::string(79, '0')));
    CHECK(r.outcome == Outcome::NoSolution);
}

DROGON_TEST(SolverAcceptsCompleteGrid) {
    DlxSolver solver;
    auto r = solver.solve(gridFrom(kWikiSolution));
    CHECK(r.outcome == Outcome::Solved);
    CHECK(toString(r.solution) == std::string(kWikiSolution));
}

DROGON_TEST(SolverStopsAtNodeLimit) {
    SolveOptions o;
    o.maxNodes = 1;
    DlxSolver solver(o);
    auto r = solver.solve(gridFrom(kHardPuzzle));
    CHECK(r.outcome == Outcome::LimitExceeded);
}

DROGON_TEST(SolverIsReusableAcrossPuzzles) {
    DlxSolver solver;
    auto a = solver.solve(gridFrom(kWikiPuzzle));
    auto b = solver.solve(gridFrom(kHardPuzzle));
    auto c = solver.solve(gridFrom(kWikiPuzzle));
    CHECK(toString(a.solution) == std::string(kWikiSolution));
    CHECK(toString(b.solution) == std::string(kHardSolution));
    CHECK(toString(c.solution) == std::string(kWikiSolution));
    CHECK(a.stats.nodesVisited == c.stats.nodesVisited);  // deterministic
}

// ---------------------------------------------------------------- validation

DROGON_TEST(ParseGridAcceptsValidBody) {
    auto p = parseGrid(jsonGrid(gridFrom(kWikiPuzzle)));
    CHECK(p.ok);
    CHECK(toString(p.grid) == std::string(kWikiPuzzle));
}

DROGON_TEST(ParseGridRejectsBadShapes) {
    Json::Value v;
    CHECK(!parseGrid(v).ok);  // null

    Json::Value noGrid(Json::objectValue);
    CHECK(!parseGrid(noGrid).ok);

    Json::Value body = jsonGrid(gridFrom(kEmpty));
    body["grid"].removeIndex(8, nullptr);
    CHECK(!parseGrid(body).ok);  // 8 rows

    body = jsonGrid(gridFrom(kEmpty));
    body["grid"][3].removeIndex(0, nullptr);
    auto p = parseGrid(body);
    CHECK(!p.ok);
    CHECK(p.message.find("Row 4") != std::string::npos);
}

DROGON_TEST(ParseGridRejectsBadCells) {
    const Json::Value bad[] = {Json::Value(10), Json::Value(-1), Json::Value(1.5),
                               Json::Value("5"), Json::Value(true), Json::Value()};
    for (const auto& cell : bad) {
        Json::Value body = jsonGrid(gridFrom(kEmpty));
        body["grid"][2][5] = cell;
        auto p = parseGrid(body);
        CHECK(!p.ok);
        CHECK(p.message.find("Row 3, column 6") != std::string::npos);
    }
}

DROGON_TEST(ClueConflictsAreExplained) {
    auto row = findClueConflict(gridFrom("5000000050" + std::string(71, '0')));
    REQUIRE(row.has_value());
    CHECK(row->find("row 1") != std::string::npos);

    std::string colPuzzle(81, '0');
    colPuzzle[0] = '7';
    colPuzzle[36] = '7';  // row 5, column 1
    auto col = findClueConflict(gridFrom(colPuzzle));
    REQUIRE(col.has_value());
    CHECK(col->find("column 1") != std::string::npos);

    std::string boxPuzzle(81, '0');
    boxPuzzle[0] = '3';
    boxPuzzle[10] = '3';  // row 2, column 2: same box, different row and column
    auto box = findClueConflict(gridFrom(boxPuzzle));
    REQUIRE(box.has_value());
    CHECK(box->find("box") != std::string::npos);

    CHECK(!findClueConflict(gridFrom(kWikiPuzzle)).has_value());
}

// ---------------------------------------------------------------- service + JSON

DROGON_TEST(ServiceRejectsConflictingClues) {
    SudokuService svc;
    auto r = svc.solve(gridFrom("55" + std::string(79, '0')));
    CHECK(r.status == SolveStatus::Invalid);
    CHECK(httpStatusFor(r.status) == 422);
    auto j = toJson(r);
    CHECK(j["status"].asString() == "invalid");
    CHECK(!j["message"].asString().empty());
}

DROGON_TEST(SolvedJsonMatchesFrontendContract) {
    SudokuService svc;
    auto r = svc.solve(gridFrom(kWikiPuzzle));
    auto j = toJson(r);
    CHECK(httpStatusFor(r.status) == 200);
    CHECK(j["status"].asString() == "solved");
    CHECK(j["multipleSolutions"].asBool() == false);
    CHECK(j["solutionCount"].asInt() == 1);
    CHECK(j["solution"].size() == 9u);
    CHECK(j["solution"][0].size() == 9u);
    CHECK(j["solution"][0][0].asInt() == 5);
    CHECK(j["stats"].isMember("executionMs"));
    CHECK(j["stats"]["recursiveCalls"].asUInt64() > 0);
    CHECK(j["stats"].isMember("nodesVisited"));
    CHECK(j["stats"].isMember("backtracks"));
}

DROGON_TEST(NoSolutionJsonHasZeroCount) {
    SudokuService svc;
    auto j = toJson(svc.solve(gridFrom("123456780"
                                       "000000009" + std::string(63, '0'))));
    CHECK(j["status"].asString() == "no-solution");
    CHECK(j["solutionCount"].asInt() == 0);
    CHECK(j.isMember("stats"));
}

DROGON_TEST(LimitExceededMapsToErrorJson) {
    SudokuService svc(ServiceOptions{2, 1});
    auto r = svc.solve(gridFrom(kHardPuzzle));
    CHECK(r.status == SolveStatus::LimitExceeded);
    auto j = toJson(r);
    CHECK(j["status"].asString() == "error");
    CHECK(j["code"].asString() == "SEARCH_LIMIT_EXCEEDED");
}

DROGON_TEST(MultipleSolutionsAreFlagged) {
    SudokuService svc;
    auto j = toJson(svc.solve(gridFrom(kEmpty)));
    CHECK(j["multipleSolutions"].asBool());
    CHECK(j["solutionCountIsLowerBound"].asBool());
}

// ---------------------------------------------------------------- config + CORS

static Config configFrom(std::map<std::string, std::string> env) {
    return Config::fromLookup([env](const std::string& k) -> std::optional<std::string> {
        auto it = env.find(k);
        if (it == env.end()) return std::nullopt;
        return it->second;
    });
}

DROGON_TEST(ConfigDefaultsAreLocalOnly) {
    auto c = configFrom({});
    CHECK(c.host == "127.0.0.1");
    CHECK(c.port == 8080);
    CHECK(c.staticDir.empty());
    CHECK(c.maxSolutions == 2);
}

DROGON_TEST(ConfigReadsOverrides) {
    auto c = configFrom({{"DLX_PORT", "9090"}, {"DLX_HOST", "0.0.0.0"},
                         {"DLX_CORS_ORIGINS", " http://a.test , null ,"},
                         {"DLX_MAX_NODES", "1000"}, {"DLX_LOG_LEVEL", "WARN"}});
    CHECK(c.port == 9090);
    CHECK(c.host == "0.0.0.0");
    REQUIRE(c.corsOrigins.size() == 2u);
    CHECK(c.corsOrigins[0] == "http://a.test");
    CHECK(c.corsOrigins[1] == "null");
    CHECK(c.maxNodes == 1000u);
    CHECK(c.logLevel == "warn");
}

DROGON_TEST(ConfigRejectsBadValues) {
    CHECK_THROWS_AS(configFrom({{"DLX_PORT", "70000"}}), ConfigError);
    CHECK_THROWS_AS(configFrom({{"DLX_PORT", "80x"}}), ConfigError);
    CHECK_THROWS_AS(configFrom({{"DLX_MAX_SOLUTIONS", "0"}}), ConfigError);
    CHECK_THROWS_AS(configFrom({{"DLX_LOG_LEVEL", "loud"}}), ConfigError);
}

DROGON_TEST(CorsPolicyMatchesExactOrigins) {
    CorsPolicy p({"http://localhost:3000", "null"});
    CHECK(p.isAllowed("http://localhost:3000"));
    CHECK(p.isAllowed("null"));
    CHECK(!p.isAllowed("http://localhost:3001"));
    CHECK(!p.isAllowed("http://evil.test"));
    CHECK(!p.isAllowed(""));
    CHECK(p.allowOriginValue("http://localhost:3000") == "http://localhost:3000");
    CHECK(p.allowOriginValue("http://evil.test").empty());
    CHECK(!CorsPolicy().isAllowed("http://localhost:3000"));
    CHECK(CorsPolicy({"*"}).allowOriginValue("http://any.test") == "*");
}

int main(int argc, char** argv) {
    return drogon::test::run(argc, argv);
}
