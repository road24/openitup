#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <vector>

#include <openitup/chart/chart.h>

namespace openitup {

// Type alias for file reading. Injectable for testing.
using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;

class SmaParser {
public:
    // Default constructor: reads files from the filesystem.
    SmaParser();

    // Injectable constructor for testing: provide custom file reader.
    explicit SmaParser(FileReaderFn file_reader);

    // Parse a .sma file and return a vector of Charts (one per difficulty).
    // chart_path: path to the .sma file.
    // Throws ChartLoadException on fatal parse errors.
    // Logs warnings for non-fatal issues (missing optional fields).
    // Returns empty vector if no pump-single5 or pump-double10 charts found.
    std::vector<Chart> parse(const std::filesystem::path& chart_path) const;

private:
    FileReaderFn file_reader_;
};

} // namespace openitup
