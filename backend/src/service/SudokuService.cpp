#include "SudokuService.h"

#include "GridValidation.h"

namespace dlx {

ServiceResult SudokuService::solve(const Grid& puzzle) const {
    ServiceResult out;

    if (auto conflict = findClueConflict(puzzle)) {
        out.status = SolveStatus::Invalid;
        out.message = *conflict;
        return out;
    }

    SolveOptions solverOptions;
    solverOptions.maxSolutions = options_.maxSolutions;
    solverOptions.maxNodes = options_.maxNodes;
    DlxSolver solver(solverOptions);
    out.detail = solver.solve(puzzle);

    switch (out.detail.outcome) {
        case Outcome::Solved:
            out.status = SolveStatus::Solved;
            break;
        case Outcome::NoSolution:
            out.status = SolveStatus::NoSolution;
            break;
        case Outcome::LimitExceeded:
            out.status = SolveStatus::LimitExceeded;
            out.message = "The search was stopped because it exceeded the configured node limit.";
            break;
    }
    return out;
}

}  // namespace dlx
