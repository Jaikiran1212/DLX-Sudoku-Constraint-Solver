#include "GridValidation.h"

#include <sstream>

namespace dlx {

ParseResult parseGrid(const Json::Value& body) {
    ParseResult out;

    if (!body.isObject() || !body.isMember("grid")) {
        out.message = "The request body must be a JSON object with a \"grid\" property.";
        return out;
    }
    const Json::Value& rows = body["grid"];
    if (!rows.isArray() || rows.size() != static_cast<Json::ArrayIndex>(kSize)) {
        out.message = "\"grid\" must be an array of exactly 9 rows.";
        return out;
    }

    for (int r = 0; r < kSize; ++r) {
        const Json::Value& row = rows[r];
        if (!row.isArray() || row.size() != static_cast<Json::ArrayIndex>(kSize)) {
            std::ostringstream msg;
            msg << "Row " << (r + 1) << " must be an array of exactly 9 cells.";
            out.message = msg.str();
            return out;
        }
        for (int c = 0; c < kSize; ++c) {
            const Json::Value& cell = row[c];
            // isInt() is false for booleans, strings and null; non-integral numbers are rejected too.
            if (!cell.isInt() || cell.asInt() < 0 || cell.asInt() > 9 ||
                (cell.isDouble() && cell.asDouble() != static_cast<double>(cell.asInt()))) {
                std::ostringstream msg;
                msg << "Row " << (r + 1) << ", column " << (c + 1)
                    << " must be an integer from 0 (empty) to 9.";
                out.message = msg.str();
                return out;
            }
            out.grid[r][c] = cell.asInt();
        }
    }

    out.ok = true;
    return out;
}

std::optional<std::string> findClueConflict(const Grid& grid) {
    // Rows
    for (int r = 0; r < kSize; ++r) {
        int firstCol[kSize + 1];
        for (int& v : firstCol) v = -1;
        for (int c = 0; c < kSize; ++c) {
            const int d = grid[r][c];
            if (d == 0) continue;
            if (firstCol[d] >= 0) {
                std::ostringstream msg;
                msg << "The digit " << d << " appears twice in row " << (r + 1) << " (columns "
                    << (firstCol[d] + 1) << " and " << (c + 1) << ").";
                return msg.str();
            }
            firstCol[d] = c;
        }
    }
    // Columns
    for (int c = 0; c < kSize; ++c) {
        int firstRow[kSize + 1];
        for (int& v : firstRow) v = -1;
        for (int r = 0; r < kSize; ++r) {
            const int d = grid[r][c];
            if (d == 0) continue;
            if (firstRow[d] >= 0) {
                std::ostringstream msg;
                msg << "The digit " << d << " appears twice in column " << (c + 1) << " (rows "
                    << (firstRow[d] + 1) << " and " << (r + 1) << ").";
                return msg.str();
            }
            firstRow[d] = r;
        }
    }
    // Boxes
    for (int br = 0; br < kBox; ++br) {
        for (int bc = 0; bc < kBox; ++bc) {
            bool seen[kSize + 1] = {};
            for (int r = br * kBox; r < (br + 1) * kBox; ++r) {
                for (int c = bc * kBox; c < (bc + 1) * kBox; ++c) {
                    const int d = grid[r][c];
                    if (d == 0) continue;
                    if (seen[d]) {
                        std::ostringstream msg;
                        msg << "The digit " << d << " appears twice in the box covering rows "
                            << (br * kBox + 1) << "-" << (br * kBox + kBox) << " and columns "
                            << (bc * kBox + 1) << "-" << (bc * kBox + kBox) << ".";
                        return msg.str();
                    }
                    seen[d] = true;
                }
            }
        }
    }
    return std::nullopt;
}

}  // namespace dlx
