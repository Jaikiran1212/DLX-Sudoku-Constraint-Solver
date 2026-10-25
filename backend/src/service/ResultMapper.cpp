#include "ResultMapper.h"

namespace dlx {

namespace {

Json::Value statsToJson(const SolveResult& r) {
    Json::Value stats(Json::objectValue);
    stats["executionMs"] = r.elapsedMs;
    stats["recursiveCalls"] = static_cast<Json::UInt64>(r.stats.recursiveCalls);
    stats["nodesVisited"] = static_cast<Json::UInt64>(r.stats.nodesVisited);
    stats["backtracks"] = static_cast<Json::UInt64>(r.stats.backtracks);
    return stats;
}

}  // namespace

Json::Value toJson(const ServiceResult& result) {
    Json::Value body(Json::objectValue);
    switch (result.status) {
        case SolveStatus::Solved: {
            const SolveResult& d = result.detail;
            body["status"] = "solved";
            Json::Value grid(Json::arrayValue);
            for (const auto& row : d.solution) {
                Json::Value jr(Json::arrayValue);
                for (int v : row) jr.append(v);
                grid.append(jr);
            }
            body["solution"] = grid;
            body["multipleSolutions"] = d.solutionCount > 1;
            body["solutionCount"] = d.solutionCount;
            // True when counting stopped at the configured cap: "at least this many".
            body["solutionCountIsLowerBound"] = d.solutionLimitReached;
            body["stats"] = statsToJson(d);
            break;
        }
        case SolveStatus::NoSolution:
            body["status"] = "no-solution";
            body["solutionCount"] = 0;
            body["stats"] = statsToJson(result.detail);
            break;
        case SolveStatus::Invalid:
            body["status"] = "invalid";
            body["message"] = result.message;
            break;
        case SolveStatus::LimitExceeded:
            body["status"] = "error";
            body["code"] = "SEARCH_LIMIT_EXCEEDED";
            body["message"] = result.message;
            break;
    }
    return body;
}

int httpStatusFor(SolveStatus status) {
    switch (status) {
        case SolveStatus::Solved:
        case SolveStatus::NoSolution:
            return 200;
        case SolveStatus::Invalid:
        case SolveStatus::LimitExceeded:
            return 422;
    }
    return 500;
}

}  // namespace dlx
