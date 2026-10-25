#pragma once

#include <optional>
#include <string>

#include <json/json.h>

#include "../solver/Types.h"

namespace dlx {

struct ParseResult {
    bool ok = false;
    Grid grid{};
    std::string message;  // set when !ok
};

/**
 * Turn a request body of the form {"grid": [[0,5,...], ... 9 rows]} into a Grid.
 * Rejects anything that is not exactly 9 rows of 9 integers in 0..9.
 */
ParseResult parseGrid(const Json::Value& body);

/**
 * Find the first pair of clues that repeat a digit in a row, column or box.
 * Returns a human-readable explanation, or std::nullopt if the clues are consistent.
 * Mirrors the frontend's findConflicts rule so both sides agree on what is a conflict.
 */
std::optional<std::string> findClueConflict(const Grid& grid);

}  // namespace dlx
