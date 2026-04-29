#include <CLI/CLI.hpp>
#include <openitup/asset/data_directory.h>
#include <openitup/core/engine.h>

#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    CLI::App app{"openitup - Pump It Up engine"};

    std::string data_dir_arg;
    std::string chart_arg;
    app.add_option("--data-dir", data_dir_arg, "Path to game data / song directory");
    app.add_option("--chart", chart_arg, "Path to chart file (.ksf)");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // Resolve data directory
    auto data_dir = openitup::resolve_data_directory(data_dir_arg);
    if (!data_dir.has_value()) {
        return 1;
    }
    if (!data_dir->validate()) {
        return 1;
    }

    try {
        openitup::EngineConfig config;
        config.data_dir_path = data_dir->path().string();
        config.chart_path = chart_arg;

        openitup::Engine engine(config);
        return engine.run();
    } catch (const std::exception& e) {
        spdlog::critical("fatal startup error: {}", e.what());
        return 1;
    }
}
