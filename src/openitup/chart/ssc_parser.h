#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <vector>

#include <openitup/chart/chart.h>

namespace openitup {

// Type alias for file reading. Injectable for testing.
// Default: reads entire file to string from filesystem.
using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;

class SscParser {
public:
    // Default constructor: reads files from the filesystem.
    SscParser();

    // Injectable constructor for testing: provide custom file reader.
    explicit SscParser(FileReaderFn file_reader);

    // Parse a .ssc file and return a vector of Charts (multi-chart support).
    // chart_path: path to the .ssc file.
    // Returns: vector of charts (one per NOTEDATA section).
    // Throws ChartLoadException on fatal parse errors.
    // Logs warnings for non-fatal issues (missing optional fields).
    std::vector<Chart> parse(const std::filesystem::path& chart_path) const;

private:
    FileReaderFn file_reader_;
};

} // namespace openitup
