#include <sys/stat.h>

#include <iostream>
#include <thread>

#include <drogon/drogon.h>

#include "AppContext.h"
#include "Config.h"
#include "http/Middleware.h"

namespace {

trantor::Logger::LogLevel toLogLevel(const std::string& level) {
    if (level == "trace") return trantor::Logger::kTrace;
    if (level == "debug") return trantor::Logger::kDebug;
    if (level == "warn") return trantor::Logger::kWarn;
    if (level == "error") return trantor::Logger::kError;
    return trantor::Logger::kInfo;
}

bool isDirectory(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

}  // namespace

int main() {
    dlx::Config config;
    try {
        config = dlx::Config::fromEnvironment();
    } catch (const dlx::ConfigError& e) {
        std::cerr << "Configuration error: " << e.what() << "\n";
        return 2;
    }
    if (!config.staticDir.empty() && !isDirectory(config.staticDir)) {
        std::cerr << "Configuration error: DLX_STATIC_DIR \"" << config.staticDir
                  << "\" is not a directory.\n";
        return 2;
    }

    dlx::AppContext::init(config);

    auto& app = drogon::app();
    app.setLogLevel(toLogLevel(config.logLevel));
    app.addListener(config.host, config.port);
    const unsigned hw = std::thread::hardware_concurrency();
    app.setThreadNum(config.threads > 0 ? config.threads : static_cast<int>(hw ? hw : 1));
    app.setClientMaxBodySize(64 * 1024);  // a puzzle is ~200 bytes

    if (config.staticDir.empty()) {
        // Drogon would otherwise serve files from the working directory.
        app.setFileTypes({});
    } else {
        app.setDocumentRoot(config.staticDir);
        app.setHomePage("index.html");
        app.setFileTypes({"html", "js", "css", "svg", "png", "ico", "json", "txt", "woff2"});
    }

    dlx::installMiddleware();

    LOG_INFO << "DLX solver backend listening on http://" << config.host << ":" << config.port
             << (config.staticDir.empty() ? "" : " (serving frontend from " + config.staticDir + ")");
    app.run();
    return 0;
}
