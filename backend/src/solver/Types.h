#pragma once

#include <array>

namespace dlx {

inline constexpr int kSize = 9;
inline constexpr int kBox = 3;

// 0 means "empty cell", 1..9 are digits. Same representation as the frontend's grid.
using Grid = std::array<std::array<int, kSize>, kSize>;

}  // namespace dlx
