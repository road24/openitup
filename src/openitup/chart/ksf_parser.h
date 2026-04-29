#pragma once

#include <string>
#include <filesystem>
#include <functional>

#include <openitup/chart/chart.h>

namespace openitup {

// Type alias for file reading. Injectable for testing.
// Default: reads entire file to string from filesystem.
using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;

class KsfParser {
public:
    // Default constructor: reads files from the filesystem.
    KsfParser();

    // Injectable constructor for testing: provide custom file reader.
    explicit KsfParser(FileReaderFn file_reader);

    // Parse a .ksf file and return a Chart.
    // chart_path: path to the .ksf file.
    // Throws ChartLoadException on fatal parse errors.
    // Logs warnings for non-fatal issues (missing optional fields).
    Chart parse(const std::filesystem::path& chart_path) const;

private:
    FileReaderFn file_reader_;
};

} // namespace openitup
