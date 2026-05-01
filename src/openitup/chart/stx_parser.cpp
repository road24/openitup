#include <openitup/chart/stx_parser.h>

#include <fstream>
#include <cstring>
#include <array>
#include <zlib.h>

#include <spdlog/spdlog.h>

#include <openitup/chart/chart_builder.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

namespace openitup {

namespace {

// STX file format structures (based on AMXSTX/STX.h)
constexpr uint32_t STX_MAGIC = 0x34465453; // "STF4" little-endian
constexpr size_t MAX_BLOCKS = 50;
constexpr size_t UNCOMPRESSED_BUFFER_SIZE = 1024 * 1024; // 1MB buffer

enum class TrackType : int {
    PRACTICE = 0,
    NORMAL,
    HARD,
    NIGHTMARE,
    CRAZY,
    FULLDOUBLE,
    HALFDOUBLE,
    DIVISION,
    LIGHTS,
    COUNT
};

// Track names for logging
const char* track_name(int track_index) {
    static const char* names[] = {
        "Practice", "Normal", "Hard", "Nightmare", "Crazy",
        "FullDouble", "HalfDouble", "Division", "Lights"
    };
    if (track_index >= 0 && track_index < static_cast<int>(TrackType::COUNT)) {
        return names[track_index];
    }
    return "Unknown";
}

// Determine play mode from track type
PlayMode track_play_mode(int track_index) {
    switch (static_cast<TrackType>(track_index)) {
        case TrackType::PRACTICE:
        case TrackType::NORMAL:
        case TrackType::HARD:
        case TrackType::CRAZY:
        case TrackType::DIVISION:
            return PlayMode::SINGLE;
        case TrackType::NIGHTMARE:
        case TrackType::FULLDOUBLE:
        case TrackType::HALFDOUBLE:
            return PlayMode::DOUBLE;
        case TrackType::LIGHTS:
            // Lights are special, treat as SINGLE for now
            return PlayMode::SINGLE;
        default:
            return PlayMode::SINGLE;
    }
}

// Determine number of columns from track type
int track_column_count(int track_index) {
    switch (static_cast<TrackType>(track_index)) {
        case TrackType::PRACTICE:
        case TrackType::NORMAL:
        case TrackType::HARD:
        case TrackType::CRAZY:
        case TrackType::DIVISION:
            return 5; // Single mode: columns 0-4
        case TrackType::NIGHTMARE:
        case TrackType::FULLDOUBLE:
            return 10; // Double mode: columns 0-9
        case TrackType::HALFDOUBLE:
            return 6; // HalfDouble: columns 2-7 (we'll remap to 0-5)
        case TrackType::LIGHTS:
            return 3; // Lights: columns 10-12 (we'll remap to 0-2)
        default:
            return 5;
    }
}

// Map STX column to output column based on track type
int map_column(int stx_col, int track_index) {
    switch (static_cast<TrackType>(track_index)) {
        case TrackType::PRACTICE:
        case TrackType::NORMAL:
        case TrackType::HARD:
        case TrackType::CRAZY:
        case TrackType::DIVISION:
            // Single mode: columns 0-4 map directly
            if (stx_col >= 0 && stx_col < 5) return stx_col;
            return -1;
        case TrackType::NIGHTMARE:
        case TrackType::FULLDOUBLE:
            // Full double: columns 0-9 map directly
            if (stx_col >= 0 && stx_col < 10) return stx_col;
            return -1;
        case TrackType::HALFDOUBLE:
            // HalfDouble: columns 2-7 map to 0-5
            if (stx_col >= 2 && stx_col <= 7) return stx_col - 2;
            return -1;
        case TrackType::LIGHTS:
            // Lights: columns 10-12 map to 0-2
            if (stx_col >= 10 && stx_col <= 12) return stx_col - 10;
            return -1;
        default:
            return -1;
    }
}

// Read little-endian uint32
uint32_t read_u32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

// Read little-endian int32
int32_t read_i32(const uint8_t* data) {
    return static_cast<int32_t>(read_u32(data));
}

// Read little-endian float
float read_float(const uint8_t* data) {
    uint32_t bits = read_u32(data);
    float result;
    std::memcpy(&result, &bits, sizeof(float));
    return result;
}

// Read null-terminated string from fixed-size buffer
std::string read_string(const uint8_t* data, size_t max_length) {
    size_t len = 0;
    while (len < max_length && data[len] != 0) {
        len++;
    }
    return std::string(reinterpret_cast<const char*>(data), len);
}

} // anonymous namespace

#pragma pack(push, 1)
struct StxParser::StxHeader {
    uint8_t magic[4];        // "STF4"
    uint32_t padding[14];    // zeros
    char title[64];          // ANSI string
    char artist[64];         // ANSI string
    char step_author[64];    // ANSI string
    uint32_t offsets[9];     // track offsets
};

struct StxParser::StxTrack {
    uint32_t difficulty;
    uint32_t blocks[MAX_BLOCKS];
};

struct StxParser::StxBlock {
    float bpm;
    uint32_t beat_per_measure;
    uint32_t beat_split;
    int32_t delay; // N * 1s/100, can be negative
    uint8_t division_set[80]; // 10 * 2 uint32s
    uint32_t speed; // stored as int(Speed * 1000)
    uint32_t pad[7];
    uint32_t block_length; // In beat_split units
};
#pragma pack(pop)

StxParser::StxParser() = default;

std::vector<uint8_t> StxParser::decompress_block(const uint8_t* compressed_data, uint32_t compressed_size) const {
    std::vector<uint8_t> output(UNCOMPRESSED_BUFFER_SIZE);
    uLongf dest_len = output.size();

    int result = uncompress(output.data(), &dest_len, compressed_data, compressed_size);
    if (result != Z_OK) {
        throw ChartLoadException("STX: zlib decompression failed with error " + std::to_string(result));
    }

    output.resize(dest_len);
    return output;
}

std::optional<Chart> StxParser::parse_track(
    const std::vector<uint8_t>& file_data,
    uint32_t track_offset,
    int track_index,
    const std::string& title,
    const std::string& artist,
    const std::string& step_author) const {

    if (track_offset == 0 || track_offset >= file_data.size()) {
        spdlog::debug("STX parser: track {} has no data (offset={})", track_name(track_index), track_offset);
        return std::nullopt;
    }

    // Read track header
    if (track_offset + sizeof(StxTrack) > file_data.size()) {
        throw ChartLoadException("STX: track header out of bounds");
    }

    const auto* track = reinterpret_cast<const StxTrack*>(file_data.data() + track_offset);

    // Count total blocks
    uint32_t total_blocks = 0;
    for (size_t i = 0; i < MAX_BLOCKS && track->blocks[i] != 0; ++i) {
        total_blocks += track->blocks[i];
    }

    if (total_blocks == 0) {
        spdlog::debug("STX parser: track {} is empty (no blocks)", track_name(track_index));
        return std::nullopt;
    }

    spdlog::debug("STX parser: parsing track {} with {} blocks", track_name(track_index), total_blocks);

    ChartBuilder builder;
    builder.set_title(title);
    builder.set_artist(artist);
    builder.set_charter_name(step_author);
    builder.set_difficulty_name(track_name(track_index));
    builder.set_difficulty_rating(track->difficulty);
    builder.set_mode(track_play_mode(track_index));

    // Track state across blocks
    float current_bpm = 120.0f;
    uint32_t current_tick = 4;
    int current_beat_in_ticks = 0; // cumulative tick position

    // Pointer to first packed block
    const uint8_t* block_ptr = file_data.data() + track_offset + sizeof(StxTrack);

    // Hold state tracking (13 columns)
    std::array<bool, 13> hold_active = {};

    // Process each block
    uint32_t blocks_processed = 0;
    for (size_t block_idx = 0; block_idx < MAX_BLOCKS && track->blocks[block_idx] != 0; ++block_idx) {
        uint32_t blocks_in_group = track->blocks[block_idx];

        for (uint32_t sub_block = 0; sub_block < blocks_in_group; ++sub_block) {
            // Read packed_block_t: uint32_t size + compressed data
            if (block_ptr + 4 > file_data.data() + file_data.size()) {
                throw ChartLoadException("STX: packed block header out of bounds");
            }

            uint32_t compressed_size = read_u32(block_ptr);
            block_ptr += 4;

            if (block_ptr + compressed_size > file_data.data() + file_data.size()) {
                throw ChartLoadException("STX: packed block data out of bounds");
            }

            // Decompress block
            std::vector<uint8_t> decompressed;
            try {
                decompressed = decompress_block(block_ptr, compressed_size);
            } catch (const ChartLoadException& e) {
                spdlog::error("STX parser: failed to decompress block {} in track {}: {}",
                             blocks_processed, track_name(track_index), e.what());
                block_ptr += compressed_size;
                blocks_processed++;
                continue;
            }

            block_ptr += compressed_size;

            if (decompressed.size() < sizeof(StxBlock)) {
                spdlog::warn("STX parser: decompressed block too small");
                blocks_processed++;
                continue;
            }

            const auto* block = reinterpret_cast<const StxBlock*>(decompressed.data());

            // Add BPM change if different from current (except first block)
            if (blocks_processed > 0 && block->bpm != current_bpm) {
                double beat = static_cast<double>(current_beat_in_ticks) / current_tick;
                builder.add_bpm_change(beat, block->bpm);
            } else if (blocks_processed == 0) {
                // First block: set initial BPM
                builder.add_bpm_change(0.0, block->bpm);
                builder.set_display_bpm(block->bpm);
            }

            current_bpm = block->bpm;
            current_tick = block->beat_split;

            // Handle delay (only for first block, or if non-zero for subsequent)
            if (blocks_processed == 0 && block->delay > 0) {
                // delay is in centiseconds (1/100 second)
                builder.set_start_time_ms(block->delay * 10.0);
            }

            // Process rows
            const uint8_t* rows_ptr = decompressed.data() + sizeof(StxBlock);
            size_t row_size = 13; // 13 bytes per row (row_t.mRow[13])

            for (uint32_t row_idx = 0; row_idx < block->block_length; ++row_idx) {
                if (rows_ptr + row_size > decompressed.data() + decompressed.size()) {
                    spdlog::warn("STX parser: row data out of bounds");
                    break;
                }

                double beat = static_cast<double>(current_beat_in_ticks) / current_tick;

                // Process each column
                for (int stx_col = 0; stx_col < 13; ++stx_col) {
                    int output_col = map_column(stx_col, track_index);
                    if (output_col < 0) continue; // Column not used in this track type

                    uint8_t note_value = rows_ptr[stx_col];

                    // Map note values based on STXDlg.cpp:
                    // 0 = empty (or hold tail if hold active)
                    // 1 = tap (or hold body if hold active)
                    // 2 = G note (special)
                    // 3 = W note (special)
                    // 4 = stepA (special)
                    // 5 = stepB (special)
                    // 6 = stepC (special)
                    // 10 = hold start
                    // 11 = hold body (explicit)
                    // 12 = hold end

                    if (note_value == 1) {
                        // Tap (or hold body if hold active)
                        if (!hold_active[stx_col]) {
                            builder.add_note(beat, output_col, NoteType::TAP);
                        }
                        // If hold is active, this is a hold body - ignore
                    } else if (note_value == 10) {
                        // Hold start
                        builder.add_note(beat, output_col, NoteType::HOLD_HEAD);
                        hold_active[stx_col] = true;
                    } else if (note_value == 12) {
                        // Hold end
                        builder.add_note(beat, output_col, NoteType::HOLD_TAIL);
                        hold_active[stx_col] = false;
                    } else if (note_value == 0) {
                        // Empty, or hold tail if hold is active (implicit)
                        if (hold_active[stx_col]) {
                            builder.add_note(beat, output_col, NoteType::HOLD_TAIL);
                            hold_active[stx_col] = false;
                        }
                    } else if (note_value == 11) {
                        // Explicit hold body - ignore (implicit in our model)
                    } else if (note_value == 2 || note_value == 3 ||
                               note_value == 4 || note_value == 5 || note_value == 6) {
                        // Special note types (G, W, stepA, stepB, stepC)
                        // For now, treat as regular taps
                        if (!hold_active[stx_col]) {
                            builder.add_note(beat, output_col, NoteType::TAP);
                        }
                    }
                    // Other values: ignore
                }

                rows_ptr += row_size;
                current_beat_in_ticks++;
            }

            blocks_processed++;
        }
    }

    // Check if we actually have any notes
    try {
        return builder.build();
    } catch (const ChartLoadException& e) {
        spdlog::warn("STX parser: track {} failed validation: {}", track_name(track_index), e.what());
        return std::nullopt;
    }
}

std::vector<Chart> StxParser::parse(const std::filesystem::path& stx_path) const {
    // Read entire file into memory
    std::ifstream file(stx_path, std::ios::binary);
    if (!file) {
        throw ChartLoadException("Could not open STX file: " + stx_path.string());
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size < sizeof(StxHeader)) {
        throw ChartLoadException("STX file too small");
    }

    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    if (!file) {
        throw ChartLoadException("Failed to read STX file");
    }

    // Parse header
    const auto* header = reinterpret_cast<const StxHeader*>(file_data.data());

    // Verify magic
    uint32_t magic = read_u32(header->magic);
    if (magic != STX_MAGIC) {
        throw ChartLoadException("Invalid STX magic number (expected STF4)");
    }

    std::string title = read_string(reinterpret_cast<const uint8_t*>(header->title), 64);
    std::string artist = read_string(reinterpret_cast<const uint8_t*>(header->artist), 64);
    std::string step_author = read_string(reinterpret_cast<const uint8_t*>(header->step_author), 64);

    spdlog::debug("STX parser: Title='{}', Artist='{}', StepAuthor='{}'", title, artist, step_author);

    // Parse each track
    std::vector<Chart> charts;
    for (int i = 0; i < static_cast<int>(TrackType::COUNT); ++i) {
        uint32_t offset = header->offsets[i];
        if (offset == 0) continue;

        auto chart = parse_track(file_data, offset, i, title, artist, step_author);
        if (chart) {
            charts.push_back(std::move(*chart));
        }
    }

    if (charts.empty()) {
        spdlog::warn("STX parser: no valid charts found in {}", stx_path.string());
    } else {
        spdlog::info("STX parser: loaded {} chart(s) from {}", charts.size(), stx_path.string());
    }

    return charts;
}

} // namespace openitup
