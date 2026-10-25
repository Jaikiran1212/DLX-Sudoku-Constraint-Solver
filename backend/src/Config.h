#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace dlx {

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * Runtime configuration. Everything comes from environment variables (see .env.example);
 * nothing is read from files and nothing secret is compiled into the binary.
 */
struct Config {
    std::string host = "127.0.0.1";
    std::uint16_t port = 8080;
    // Origins allowed to call the API from a browser. "*" allows any, "null" allows file:// pages.
    std::vector<std::string> corsOrigins = {
        "http://127.0.0.1:8080", "http://localhost:8080",
        "http://127.0.0.1:3000", "http://localhost:3000",
    };
    std::string staticDir;  // empty: do not serve the frontend
    int maxSolutions = 2;
    std::uint64_t maxNodes = 5'000'000;
    int threads = 0;  // 0 = number of hardware threads
    std::string logLevel = "info";

    using Lookup = std::function<std::optional<std::string>(const std::string&)>;

    // Throws ConfigError with a message naming the offending variable.
    static Config fromLookup(const Lookup& lookup);
    static Config fromEnvironment();
};

}  // namespace dlx
