#include "DlxSolver.h"

#include <algorithm>
#include <chrono>

namespace dlx {

namespace {

// Column layout (0-based, headers are stored at index + 1):
//   [  0,  81)  cell (r, c) holds some digit
//   [ 81, 162)  row r contains digit d
//   [162, 243)  column c contains digit d
//   [243, 324)  box b contains digit d
constexpr int kCellBase = 0;
constexpr int kRowBase = kSize * kSize;
constexpr int kColBase = 2 * kSize * kSize;
constexpr int kBoxBase = 3 * kSize * kSize;

constexpr int rowIdFor(int r, int c, int digitIndex) {
    return (r * kSize + c) * kSize + digitIndex;
}

}  // namespace

DlxSolver::DlxSolver(SolveOptions options) : options_(options) {
    options_.maxSolutions = std::max(1, options_.maxSolutions);
    partial_.reserve(kSize * kSize);
}

void DlxSolver::buildMatrix() {
    constexpr int root = 0;

    // Header row: a circular list root <-> 1 <-> 2 <-> ... <-> 324 <-> root.
    for (int i = 0; i <= kColumns; ++i) {
        left_[i] = (i == 0) ? kColumns : i - 1;
        right_[i] = (i == kColumns) ? root : i + 1;
    }
    for (int col = 1; col <= kColumns; ++col) {
        up_[col] = down_[col] = col;
        column_[col] = col;
        size_[col] = 0;
    }

    int next = kColumns + 1;
    for (int r = 0; r < kSize; ++r) {
        for (int c = 0; c < kSize; ++c) {
            const int box = (r / kBox) * kBox + c / kBox;
            for (int d = 0; d < kSize; ++d) {
                const int id = rowIdFor(r, c, d);
                const std::array<int, 4> cols = {
                    1 + kCellBase + r * kSize + c,
                    1 + kRowBase + r * kSize + d,
                    1 + kColBase + c * kSize + d,
                    1 + kBoxBase + box * kSize + d,
                };
                const int first = next;
                rowFirstNode_[id] = first;
                for (int k = 0; k < 4; ++k) {
                    const int n = next++;
                    const int head = cols[k];
                    column_[n] = head;
                    rowId_[n] = id;
                    // Append to the bottom of the column.
                    up_[n] = up_[head];
                    down_[n] = head;
                    down_[up_[head]] = n;
                    up_[head] = n;
                    ++size_[head];
                    // The four nodes of one row form their own circular list.
                    left_[n] = first + (k + 3) % 4;
                    right_[n] = first + (k + 1) % 4;
                }
            }
        }
    }
}

void DlxSolver::cover(int c) {
    right_[left_[c]] = right_[c];
    left_[right_[c]] = left_[c];
    for (int i = down_[c]; i != c; i = down_[i]) {
        for (int j = right_[i]; j != i; j = right_[j]) {
            down_[up_[j]] = down_[j];
            up_[down_[j]] = up_[j];
            --size_[column_[j]];
        }
    }
}

void DlxSolver::uncover(int c) {
    for (int i = up_[c]; i != c; i = up_[i]) {
        for (int j = left_[i]; j != i; j = left_[j]) {
            ++size_[column_[j]];
            down_[up_[j]] = j;
            up_[down_[j]] = j;
        }
    }
    right_[left_[c]] = c;
    left_[right_[c]] = c;
}

// Knuth's "S heuristic": the column with the fewest remaining candidates.
int DlxSolver::chooseColumn() const {
    int best = right_[0];
    for (int c = right_[0]; c != 0; c = right_[c]) {
        if (size_[c] < size_[best]) {
            best = c;
            if (size_[c] == 0) break;
        }
    }
    return best;
}

void DlxSolver::recordSolution() {
    if (result_.solutionCount != 0) return;  // only the first solution is kept
    Grid grid = *puzzle_;
    for (int id : partial_) {
        const int r = id / (kSize * kSize);
        const int c = (id / kSize) % kSize;
        grid[r][c] = id % kSize + 1;
    }
    result_.solution = grid;
}

// Returns the number of solutions found below this node.
int DlxSolver::search(int /*depth*/) {
    ++result_.stats.recursiveCalls;

    if (right_[0] == 0) {  // every constraint satisfied
        recordSolution();
        ++result_.solutionCount;
        if (result_.solutionCount >= options_.maxSolutions) {
            stop_ = true;
            result_.solutionLimitReached = true;
        }
        return 1;
    }

    const int c = chooseColumn();
    if (size_[c] == 0) return 0;  // dead end

    cover(c);
    int found = 0;
    for (int r = down_[c]; r != c && !stop_ && !aborted_; r = down_[r]) {
        if (++result_.stats.nodesVisited > options_.maxNodes) {
            aborted_ = true;
            break;
        }
        partial_.push_back(rowId_[r]);
        for (int j = right_[r]; j != r; j = right_[j]) cover(column_[j]);

        const int sub = search(0);

        for (int j = left_[r]; j != r; j = left_[j]) uncover(column_[j]);
        partial_.pop_back();

        if (sub == 0 && !aborted_) ++result_.stats.backtracks;
        found += sub;
    }
    uncover(c);
    return found;
}

SolveResult DlxSolver::solve(const Grid& puzzle) {
    const auto started = std::chrono::steady_clock::now();

    result_ = SolveResult{};
    aborted_ = false;
    stop_ = false;
    partial_.clear();
    covered_.fill(false);
    puzzle_ = &puzzle;
    buildMatrix();

    // Apply the clues by selecting their rows up front. If two clues claim the same
    // constraint (a duplicate in a row, column or box), the puzzle cannot be solved.
    bool consistent = true;
    for (int r = 0; r < kSize && consistent; ++r) {
        for (int c = 0; c < kSize && consistent; ++c) {
            const int digit = puzzle[r][c];
            if (digit == 0) continue;
            if (digit < 1 || digit > kSize) {
                consistent = false;
                break;
            }
            const int first = rowFirstNode_[rowIdFor(r, c, digit - 1)];
            for (int k = 0; k < 4; ++k) {
                if (covered_[column_[first + k]]) consistent = false;
            }
            if (!consistent) break;
            for (int k = 0; k < 4; ++k) {
                covered_[column_[first + k]] = true;
                cover(column_[first + k]);
            }
        }
    }

    if (consistent) search(0);

    if (aborted_) {
        result_.outcome = Outcome::LimitExceeded;
        result_.solutionLimitReached = false;
    } else if (result_.solutionCount > 0) {
        result_.outcome = Outcome::Solved;
    } else {
        result_.outcome = Outcome::NoSolution;
    }

    puzzle_ = nullptr;
    result_.elapsedMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    return result_;
}

}  // namespace dlx
