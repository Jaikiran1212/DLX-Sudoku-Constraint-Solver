#pragma once

#include <json/json.h>

#include "SudokuService.h"

namespace dlx {

// JSON body for a service result, in the shape the frontend's solver client expects.
Json::Value toJson(const ServiceResult& result);

// HTTP status code that goes with it (200 for solved / no-solution, 422 otherwise).
int httpStatusFor(SolveStatus status);

}  // namespace dlx
