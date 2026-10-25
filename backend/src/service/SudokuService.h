#pragma once

#include <cstdint>
#include <string>

#include "../solver/DlxSolver.h"

namespace dlx {

enum class SolveStatus {
    Solved,
    NoSolution,
    Invalid,        // the clues contradict each other (message explains how)
    LimitExceeded,  // the configured search budget ran out
};

struct ServiceResult {
    SolveStatus status = SolveStatus::NoSolution;
    std::string message;  // for Invalid / LimitExceeded
    SolveResult detail;   // for Solved / NoSolution
};

struct ServiceOptions {
    int maxSolutions = 2;
    std::uint64_t maxNodes = 5'000'000;
};

/**
 * Business logic of the backend: validate the clues, run the solver, classify the result.
 * Stateless and thread-safe: every call uses its own solver instance.
 */
class SudokuService {
public:
    explicit SudokuService(ServiceOptions options = {}) : options_(options) {}

    ServiceResult solve(const Grid& puzzle) const;
    const ServiceOptions& options() const { return options_; }

private:
    ServiceOptions options_;
};

}  // namespace dlx
