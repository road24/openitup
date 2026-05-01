#pragma once

#include <string>
#include <filesystem>

#include <openitup/chart/chart.h>

namespace openitup {

class OsfWriter {
public:
    // Write a Chart to an OSF JSON file.
    // chart: the Chart to serialize.
    // output_path: path where the .osf file will be written.
    // Throws std::runtime_error on write failure.
    void write(const Chart& chart, const std::filesystem::path& output_path) const;

    // Serialize a Chart to an OSF JSON string.
    // chart: the Chart to serialize.
    // Returns: pretty-printed JSON string with 2-space indentation.
    std::string to_json(const Chart& chart) const;
};

} // namespace openitup
