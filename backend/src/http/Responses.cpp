#include "Responses.h"

namespace dlx {

drogon::HttpResponsePtr jsonResponse(int httpStatus, const Json::Value& body) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(httpStatus));
    resp->addHeader("Cache-Control", "no-store");
    return resp;
}

drogon::HttpResponsePtr errorResponse(int httpStatus, const std::string& code,
                                      const std::string& message) {
    Json::Value body(Json::objectValue);
    body["status"] = "error";
    body["code"] = code;
    body["message"] = message;
    return jsonResponse(httpStatus, body);
}

}  // namespace dlx
