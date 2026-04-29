#include <openitup/core/engine.h>

#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    try {
        openitup::EngineConfig config;
        config.window_title = "openitup";
        config.window_width = 1280;
        config.window_height = 960;

        openitup::Engine engine(config);
        return engine.run();
    } catch (const std::exception& e) {
        spdlog::critical("fatal startup error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::critical("fatal startup error: unknown exception");
        return 1;
    }
}
