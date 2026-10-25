#pragma once

#include <string>

#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace dlx {

drogon::HttpResponsePtr jsonResponse(int httpStatus, const Json::Value& body);

// {"status":"error","code":...,"message":...}
drogon::HttpResponsePtr errorResponse(int httpStatus, const std::string& code,
                                      const std::string& message);

}  // namespace dlx
