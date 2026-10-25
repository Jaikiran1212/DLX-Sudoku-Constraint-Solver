#include "SolverController.h"

#include <exception>

#include "../AppContext.h"
#include "../http/Responses.h"
#include "../service/GridValidation.h"
#include "../service/ResultMapper.h"

#ifndef DLX_VERSION
#define DLX_VERSION "0.0.0"
#endif

namespace dlx {

void SolverController::solve(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    try {
        if (req->contentType() != drogon::CT_APPLICATION_JSON) {
            callback(errorResponse(415, "UNSUPPORTED_MEDIA_TYPE",
                                   "Send the request with Content-Type: application/json."));
            return;
        }
        const auto json = req->getJsonObject();
        if (!json) {
            callback(errorResponse(400, "INVALID_JSON", "The request body is not valid JSON."));
            return;
        }
        const ParseResult parsed = parseGrid(*json);
        if (!parsed.ok) {
            callback(errorResponse(400, "INVALID_REQUEST", parsed.message));
            return;
        }
        const ServiceResult result = AppContext::get().service.solve(parsed.grid);
        callback(jsonResponse(httpStatusFor(result.status), toJson(result)));
    } catch (const std::exception& e) {
        LOG_ERROR << "solve failed: " << e.what();
        callback(errorResponse(500, "INTERNAL_ERROR", "The solver failed unexpectedly."));
    }
}

void SolverController::status(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto& opts = AppContext::get().service.options();
    Json::Value body(Json::objectValue);
    body["connected"] = true;
    body["backend"] = "cpp-drogon";
    body["solver"] = "dlx";
    body["version"] = DLX_VERSION;
    body["limits"]["maxSolutions"] = opts.maxSolutions;
    body["limits"]["maxNodes"] = static_cast<Json::UInt64>(opts.maxNodes);
    callback(jsonResponse(200, body));
}

void SolverController::health(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    Json::Value body(Json::objectValue);
    body["status"] = "ok";
    callback(jsonResponse(200, body));
}

}  // namespace dlx
