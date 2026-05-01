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

    try {
        openitup::EngineConfig config;

        // If --chart is provided, use direct gameplay mode (Phase 1 compatibility)
        if (!chart_arg.empty()) {
            // Resolve data directory
            auto data_dir = openitup::resolve_data_directory(data_dir_arg);
            if (!data_dir.has_value()) {
                return 1;
            }
            if (!data_dir->validate()) {
                return 1;
            }

            std::filesystem::path chart_file = chart_arg;
            spdlog::info("Chart: {}", chart_file.string());
            spdlog::info("Data dir: {}", data_dir->path().string());

            openitup::Engine engine(config);
            return engine.run_gameplay(chart_file, data_dir->path());
        }

        // Otherwise, use full scene flow starting with BootScene
        // Resolve data directory if provided
        if (!data_dir_arg.empty()) {
            auto data_dir = openitup::resolve_data_directory(data_dir_arg);
            if (!data_dir.has_value()) {
                return 1;
            }
            if (!data_dir->validate()) {
                return 1;
            }

            config.data_dir_path = data_dir->path().string();

            // Find a chart file to use for testing
            auto found = data_dir->find_file_by_extension(".ksf");
            if (found) {
                config.chart_path = found->string();
                spdlog::info("Found test chart: {}", config.chart_path);
            }
        }

        spdlog::info("Starting with scene flow (Boot → Title → Mode Select → Gameplay)");
        openitup::Engine engine(config);
        return engine.run();

    } catch (const std::exception& e) {
        spdlog::critical("fatal startup error: {}", e.what());
        return 1;
    }
}
