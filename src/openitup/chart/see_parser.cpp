#include <openitup/chart/see_parser.h>

#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>
#include <openitup/chart/chart_builder.h>

namespace openitup {

namespace {

// Default filesystem reader.
std::string read_file_from_disk(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw ChartLoadException("Could not open file: " + path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

} // anonymous namespace

SeeParser::SeeParser() : file_reader_(read_file_from_disk) {}

SeeParser::SeeParser(FileReaderFn file_reader) : file_reader_(std::move(file_reader)) {}

std::vector<Chart> SeeParser::parse(const std::filesystem::path& chart_path) const {
    // SEE format is encrypted and not yet supported.
    // Log error and return empty vector.
    spdlog::error("SEE format not yet supported — requires decryption key: {}",
                  chart_path.string());
    spdlog::error("SEE files are encrypted. Decryption support is not implemented.");

    // Return empty vector rather than throwing, to allow graceful degradation
    return {};
}

} // namespace openitup
