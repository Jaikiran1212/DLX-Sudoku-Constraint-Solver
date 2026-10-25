#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "Types.h"

namespace dlx {

struct SolveOptions {
    // The search stops once this many solutions have been found. 2 is enough to tell
    // "unique" from "multiple" without enumerating huge numbers of solutions.
    int maxSolutions = 2;
    // Safety valve: abort after this many candidate placements have been tried.
    std::uint64_t maxNodes = 5'000'000;
};

// All counters describe the search only. Clues are applied before the search starts and
// are not counted.
struct SearchStats {
    std::uint64_t recursiveCalls = 0;  // calls of the recursive search routine
    std::uint64_t nodesVisited = 0;    // candidate placements (matrix rows) tried
    std::uint64_t backtracks = 0;      // placements undone because their subtree held no solution
};

enum class Outcome {
    Solved,         // at least one solution found
    NoSolution,     // the search space was exhausted without a solution
    LimitExceeded,  // maxNodes reached before the question was settled
};

struct SolveResult {
    Outcome outcome = Outcome::NoSolution;
    Grid solution{};                    // first solution found (valid when outcome == Solved)
    int solutionCount = 0;              // solutions found, never more than maxSolutions
    bool solutionLimitReached = false;  // true when counting stopped because of maxSolutions
    SearchStats stats;
    double elapsedMs = 0.0;             // matrix construction + search
};

/**
 * Sudoku as an exact-cover problem, solved with Knuth's Algorithm X on Dancing Links.
 *
 * 324 constraint columns (cell filled, digit in row, digit in column, digit in box) and
 * 729 candidate rows (row x column x digit), 4 nodes each. One instance can solve any
 * number of puzzles, but it is not thread-safe; create one per request/thread.
 */
class DlxSolver {
public:
    explicit DlxSolver(SolveOptions options = {});

    // The puzzle must be well-formed (9x9, digits 0..9). Clues that contradict each other
    // yield Outcome::NoSolution.
    SolveResult solve(const Grid& puzzle);

private:
    static constexpr int kColumns = 4 * kSize * kSize;                 // 324
    static constexpr int kRows = kSize * kSize * kSize;                // 729
    static constexpr int kNodes = 1 + kColumns + kRows * 4;            // root + headers + cells

    void buildMatrix();
    void cover(int column);
    void uncover(int column);
    int chooseColumn() const;
    int search(int depth);
    void recordSolution();

    SolveOptions options_;

    // Circular doubly-linked lists in both directions. Node 0 is the root, 1..324 are the
    // column headers, everything after is a matrix cell.
    std::array<int, kNodes> left_{}, right_{}, up_{}, down_{}, column_{};
    std::array<int, kColumns + 1> size_{};
    std::array<int, kNodes> rowId_{};
    std::array<int, kRows> rowFirstNode_{};

    // Per-solve state.
    const Grid* puzzle_ = nullptr;
    std::vector<int> partial_;  // row ids chosen by the search so far
    std::array<bool, kColumns + 1> covered_{};
    SolveResult result_;
    bool aborted_ = false;
    bool stop_ = false;
};

}  // namespace dlx
