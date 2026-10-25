#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace dlx {

/** Decides which browser origins may call the API. Pure logic, no HTTP dependency. */
class CorsPolicy {
public:
    CorsPolicy() = default;
    explicit CorsPolicy(std::vector<std::string> origins) : origins_(std::move(origins)) {}

    bool allowsAny() const {
        return std::find(origins_.begin(), origins_.end(), "*") != origins_.end();
    }

    bool isAllowed(const std::string& origin) const {
        if (origin.empty()) return false;
        return allowsAny() || std::find(origins_.begin(), origins_.end(), origin) != origins_.end();
    }

    // Value for Access-Control-Allow-Origin, or empty if the origin is not allowed.
    std::string allowOriginValue(const std::string& origin) const {
        if (!isAllowed(origin)) return {};
        return allowsAny() ? "*" : origin;
    }

private:
    std::vector<std::string> origins_;
};

}  // namespace dlx
