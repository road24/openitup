#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include <openitup/chart/chart.h>

namespace openitup {

class StxParser {
public:
    StxParser();

    // Parse an STX file and return a vector of Charts (one per non-empty track).
    // stx_path: path to the .stx file.
    // Throws ChartLoadException on fatal parse errors (invalid magic, corrupt data).
    // Logs warnings for non-fatal issues (empty tracks, invalid note values).
    // Returns empty vector if no valid tracks found.
    std::vector<Chart> parse(const std::filesystem::path& stx_path) const;

private:
    struct StxHeader;
    struct StxTrack;
    struct StxBlock;

    // Helper: decompress a zlib-compressed block.
    // Returns decompressed data or throws ChartLoadException on failure.
    std::vector<uint8_t> decompress_block(const uint8_t* compressed_data, uint32_t compressed_size) const;

    // Helper: parse a single track at the given offset.
    // Returns a Chart or empty optional if track is empty.
    std::optional<Chart> parse_track(
        const std::vector<uint8_t>& file_data,
        uint32_t track_offset,
        int track_index,
        const std::string& title,
        const std::string& artist,
        const std::string& step_author) const;
};

} // namespace openitup
