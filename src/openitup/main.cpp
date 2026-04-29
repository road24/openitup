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

    // Find chart file
    std::filesystem::path chart_file;
    if (!chart_arg.empty()) {
        chart_file = chart_arg;
    } else {
        auto found = data_dir->find_file_by_extension(".ksf");
        if (!found) {
            spdlog::error("No .ksf chart found in '{}'", data_dir->path().string());
            return 1;
        }
        chart_file = *found;
    }

    spdlog::info("Chart: {}", chart_file.string());
    spdlog::info("Data dir: {}", data_dir->path().string());

    try {
        openitup::EngineConfig config;
        openitup::Engine engine(config);
        return engine.run_gameplay(chart_file, data_dir->path());
    } catch (const std::exception& e) {
        spdlog::critical("fatal startup error: {}", e.what());
        return 1;
    }
}
