#pragma once

#include <memory>
#include <utility>

#include "Config.h"
#include "http/CorsPolicy.h"
#include "service/SudokuService.h"

namespace dlx {

/** Process-wide read-only state, created once in main() before the server starts. */
class AppContext {
public:
    static void init(Config config) {
        slot() = std::unique_ptr<AppContext>(new AppContext(std::move(config)));
    }
    static const AppContext& get() { return *slot(); }

    const Config config;
    const SudokuService service;
    const CorsPolicy cors;

private:
    explicit AppContext(Config c)
        : config(std::move(c)),
          service(ServiceOptions{config.maxSolutions, config.maxNodes}),
          cors(config.corsOrigins) {}

    static std::unique_ptr<AppContext>& slot() {
        static std::unique_ptr<AppContext> instance;
        return instance;
    }
};

}  // namespace dlx
