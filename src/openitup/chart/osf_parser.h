#pragma once

#include <string>
#include <filesystem>
#include <functional>

#include <openitup/chart/chart.h>

namespace openitup {

// Type alias for file reading. Injectable for testing.
using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;

class OsfParser {
public:
    // Default constructor: reads files from the filesystem.
    OsfParser();

    // Injectable constructor for testing: provide custom file reader.
    explicit OsfParser(FileReaderFn file_reader);

    // Parse a .osf JSON file and return a Chart.
    // chart_path: path to the .osf file.
    // Throws ChartLoadException on parse errors or invalid JSON.
    // Logs warnings for non-fatal issues (missing optional fields).
    Chart parse(const std::filesystem::path& chart_path) const;

private:
    FileReaderFn file_reader_;
};

} // namespace openitup
