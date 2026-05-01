#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <vector>

#include <openitup/chart/chart.h>

namespace openitup {

// Type alias for file reading. Injectable for testing.
using FileReaderFn = std::function<std::string(const std::filesystem::path&)>;

class SeeParser {
public:
    // Default constructor: reads files from the filesystem.
    SeeParser();

    // Injectable constructor for testing: provide custom file reader.
    explicit SeeParser(FileReaderFn file_reader);

    // Attempt to parse a .see file.
    // SEE format is encrypted and requires a decryption key.
    // Currently returns an empty vector and logs an error.
    // chart_path: path to the .see file.
    // Returns: empty vector (SEE format not yet supported).
    std::vector<Chart> parse(const std::filesystem::path& chart_path) const;

private:
    FileReaderFn file_reader_;
};

} // namespace openitup
