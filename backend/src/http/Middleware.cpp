#include "Middleware.h"

#include <drogon/drogon.h>

#include "../AppContext.h"
#include "Responses.h"

namespace dlx {

namespace {

void addCorsHeaders(const drogon::HttpResponsePtr& resp, const std::string& allowOrigin) {
    resp->addHeader("Access-Control-Allow-Origin", allowOrigin);
    resp->addHeader("Vary", "Origin");
}

}  // namespace

void installMiddleware() {
    auto& app = drogon::app();

    // Answer CORS preflights for every /api/ path before routing.
    app.registerPreRoutingAdvice([](const drogon::HttpRequestPtr& req,
                                    drogon::AdviceCallback&& callback,
                                    drogon::AdviceChainCallback&& next) {
        const std::string& path = req->path();
        if (path.rfind("/api/", 0) != 0) {
            next();
            return;
        }
        // Known endpoints called with the wrong method get a JSON 405 instead of Drogon's HTML page.
        const char* allowed = path == "/api/solve" ? "POST" : (path == "/api/status" || path == "/api/health") ? "GET" : nullptr;
        if (allowed != nullptr && req->method() != drogon::Options &&
            req->getMethodString() != std::string(allowed)) {
            auto resp = errorResponse(405, "METHOD_NOT_ALLOWED",
                                      std::string("Use ") + allowed + " for this endpoint.");
            resp->addHeader("Allow", std::string(allowed) + ", OPTIONS");
            callback(resp);
            return;
        }
        if (req->method() != drogon::Options) {
            next();
            return;
        }
        const std::string origin = req->getHeader("origin");
        const std::string allow = AppContext::get().cors.allowOriginValue(origin);
        if (!origin.empty() && allow.empty()) {
            callback(errorResponse(403, "CORS_ORIGIN_NOT_ALLOWED",
                                   "This origin is not allowed to call the API."));
            return;
        }
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        resp->addHeader("Access-Control-Max-Age", "600");
        resp->addHeader("X-Content-Type-Options", "nosniff");
        if (!allow.empty()) addCorsHeaders(resp, allow);
        callback(resp);
    });

    // Headers on every normal response.
    app.registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
            resp->addHeader("X-Content-Type-Options", "nosniff");
            const std::string allow =
                AppContext::get().cors.allowOriginValue(req->getHeader("origin"));
            if (!allow.empty()) addCorsHeaders(resp, allow);
        });

    app.setCustom404Page(errorResponse(404, "NOT_FOUND", "No such resource."), false);
}

}  // namespace dlx
