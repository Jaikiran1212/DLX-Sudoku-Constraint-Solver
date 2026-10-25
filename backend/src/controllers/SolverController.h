#pragma once

#include <functional>

#include <drogon/HttpController.h>

namespace dlx {

class SolverController : public drogon::HttpController<SolverController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SolverController::solve, "/api/solve", drogon::Post);
    ADD_METHOD_TO(SolverController::status, "/api/status", drogon::Get);
    ADD_METHOD_TO(SolverController::health, "/api/health", drogon::Get);
    METHOD_LIST_END

    void solve(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void status(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void health(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

}  // namespace dlx
