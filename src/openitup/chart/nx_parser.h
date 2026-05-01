#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <cstdint>

#include <openitup/chart/chart.h>

namespace openitup {

// Type alias for binary file reading. Injectable for testing.
// Default: reads entire file to a vector of bytes from filesystem.
using BinaryReaderFn = std::function<std::vector<uint8_t>(const std::filesystem::path&)>;

class NxParser {
public:
    // Default constructor: reads files from the filesystem.
    NxParser();

    // Injectable constructor for testing: provide custom file reader.
    explicit NxParser(BinaryReaderFn file_reader);

    // Parse a .nx file and return a Chart.
    // chart_path: path to the .nx file.
    // Throws ChartLoadException on fatal parse errors.
    // Logs warnings for non-fatal issues (missing optional fields, unsupported features).
    Chart parse(const std::filesystem::path& chart_path) const;

private:
    BinaryReaderFn file_reader_;
};

} // namespace openitup
