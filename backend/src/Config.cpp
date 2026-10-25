#include "Config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>

namespace dlx {

namespace {

std::string trim(const std::string& s) {
    auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    auto begin = std::find_if_not(s.begin(), s.end(), isSpace);
    auto end = std::find_if_not(s.rbegin(), s.rend(), isSpace).base();
    return begin < end ? std::string(begin, end) : std::string();
}

std::uint64_t parseUnsigned(const std::string& name, const std::string& raw, std::uint64_t min,
                            std::uint64_t max) {
    const std::string text = trim(raw);
    std::uint64_t value = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || ec != std::errc{} || ptr != text.data() + text.size() || value < min ||
        value > max) {
        throw ConfigError(name + " must be an integer between " + std::to_string(min) + " and " +
                          std::to_string(max) + " (got \"" + raw + "\").");
    }
    return value;
}

std::vector<std::string> splitList(const std::string& raw) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= raw.size()) {
        const std::size_t comma = raw.find(',', start);
        const std::string item =
            trim(raw.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (!item.empty()) out.push_back(item);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return out;
}

}  // namespace

Config Config::fromLookup(const Lookup& lookup) {
    Config cfg;

    if (auto v = lookup("DLX_HOST"); v && !trim(*v).empty()) cfg.host = trim(*v);
    const auto dlxPort = lookup("DLX_PORT");
    const auto platformPort = lookup("PORT");

    if (dlxPort && !trim(*dlxPort).empty()) {
        cfg.port = static_cast<std::uint16_t>(
            parseUnsigned("DLX_PORT", *dlxPort, 1, 65535));
    } else if (platformPort && !trim(*platformPort).empty()) {
        cfg.port = static_cast<std::uint16_t>(
            parseUnsigned("PORT", *platformPort, 1, 65535));
    }

    // An explicitly empty DLX_CORS_ORIGINS means "allow no cross-origin callers".
    if (auto v = lookup("DLX_CORS_ORIGINS")) cfg.corsOrigins = splitList(*v);
    if (auto v = lookup("DLX_STATIC_DIR")) cfg.staticDir = trim(*v);
    if (auto v = lookup("DLX_MAX_SOLUTIONS"); v && !trim(*v).empty()) {
        cfg.maxSolutions = static_cast<int>(parseUnsigned("DLX_MAX_SOLUTIONS", *v, 1, 1000));
    }
    if (auto v = lookup("DLX_MAX_NODES"); v && !trim(*v).empty()) {
        cfg.maxNodes = parseUnsigned("DLX_MAX_NODES", *v, 1, 1'000'000'000'000ULL);
    }
    if (auto v = lookup("DLX_THREADS"); v && !trim(*v).empty()) {
        cfg.threads = static_cast<int>(parseUnsigned("DLX_THREADS", *v, 0, 256));
    }
    if (auto v = lookup("DLX_LOG_LEVEL"); v && !trim(*v).empty()) {
        std::string level = trim(*v);
        std::transform(level.begin(), level.end(), level.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        static const char* const allowed[] = {"trace", "debug", "info", "warn", "error"};
        if (std::none_of(std::begin(allowed), std::end(allowed),
                         [&](const char* a) { return level == a; })) {
            throw ConfigError("DLX_LOG_LEVEL must be one of trace, debug, info, warn, error (got \"" +
                              *v + "\").");
        }
        cfg.logLevel = level;
    }
    return cfg;
}

Config Config::fromEnvironment() {
    return fromLookup([](const std::string& name) -> std::optional<std::string> {
        const char* value = std::getenv(name.c_str());
        if (value == nullptr) return std::nullopt;
        return std::string(value);
    });
}

}  // namespace dlx
