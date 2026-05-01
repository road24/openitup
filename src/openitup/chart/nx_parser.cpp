#include <openitup/chart/nx_parser.h>

#include <fstream>
#include <cstring>
#include <algorithm>
#include <array>

#include <spdlog/spdlog.h>

#include <openitup/chart/chart_builder.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

namespace openitup {

namespace {

// Default filesystem reader for binary files.
std::vector<uint8_t> read_binary_file_from_disk(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw ChartLoadException("Could not open file: " + path.string());
    }
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw ChartLoadException("Could not read file: " + path.string());
    }
    return buffer;
}

// NX20 binary structures (little-endian)
struct NX20Header {
    uint32_t magic;           // "NX20" = 0x3032584E
    uint32_t half_double_flag;
    uint32_t tracks_per_row;  // 5 for single, 10 for double
    uint32_t lightmap_flag;
};

struct NX20DivisionHeader {
    float start;             // Start time in milliseconds
    float bpm;
    float split;             // Split/scroll value
    float delay;             // Delay in milliseconds
    float speed;             // Speed modifier
    uint8_t beat_split;      // Rows per beat
    uint8_t beats_per_measure;
    uint8_t smooth;          // Smooth speed change flag
    uint8_t unknown;
};

// Helper to read little-endian values from buffer
class BinaryReader {
public:
    explicit BinaryReader(const std::vector<uint8_t>& data)
        : data_(data), pos_(0) {}

    template<typename T>
    T read() {
        if (pos_ + sizeof(T) > data_.size()) {
            throw ChartLoadException("Unexpected end of file while reading binary data");
        }
        T value;
        std::memcpy(&value, &data_[pos_], sizeof(T));
        pos_ += sizeof(T);
        return value;
    }

    uint32_t read_u32() { return read<uint32_t>(); }
    int32_t read_i32() { return read<int32_t>(); }
    float read_float() { return read<float>(); }
    uint8_t read_u8() { return read<uint8_t>(); }

    void skip(size_t bytes) {
        if (pos_ + bytes > data_.size()) {
            throw ChartLoadException("Unexpected end of file while skipping bytes");
        }
        pos_ += bytes;
    }

    size_t position() const { return pos_; }
    size_t remaining() const { return data_.size() - pos_; }

private:
    const std::vector<uint8_t>& data_;
    size_t pos_;
};

// Read extra data section (ID/value pairs)
void read_extra_data(BinaryReader& reader, ChartBuilder* builder = nullptr) {
    int32_t count = reader.read_i32();

    for (int32_t i = 0; i < count; ++i) {
        int32_t id = reader.read_i32();
        int32_t value = reader.read_i32();

        // ID 1001 in block extra data = difficulty rating
        if (builder && id == 1001) {
            builder->set_difficulty_rating(value);
        }
    }
}

// Process a single note from the row data
void process_note(ChartBuilder& builder, int track, double beat, uint32_t data) {
    if (data == 0) {
        return;  // No note
    }

    uint8_t type = (data & 0x000000FF);
    uint8_t display = (data & 0x0000FF00) >> 8;
    // uint8_t seed = (data & 0x00FF0000) >> 16;      // Unused
    // uint8_t scoring = (data & 0xFF000000) >> 24;   // Unused (routine player indicator)

    // Invisible notes are skipped
    if (display == 0) {
        return;
    }

    // Map note types
    switch (type & 0x0F) {
        case 0x3:
            builder.add_note(beat, track, NoteType::TAP);
            break;
        case 0x7:
            builder.add_note(beat, track, NoteType::HOLD_HEAD);
            break;
        case 0xB:
            // Hold body - ignored (implicit between head and tail)
            break;
        case 0xF:
            builder.add_note(beat, track, NoteType::HOLD_TAIL);
            break;
        default:
            spdlog::debug("NX parser: unknown note type 0x{:X}", type);
            break;
    }
}

// Read a single row of notes
void read_row(BinaryReader& reader, ChartBuilder& builder, int tracks_per_row, double beat) {
    uint32_t first_data = reader.read_u32();

    // Check if this is a single-column row (bit 0x80 set)
    if (first_data & 0x80) {
        // Single column row - only process the first column
        process_note(builder, 0, beat, first_data);
        return;
    }

    // Multi-column row - process first column, then read remaining columns
    process_note(builder, 0, beat, first_data);

    for (int track = 1; track < tracks_per_row; ++track) {
        uint32_t data = reader.read_u32();
        process_note(builder, track, beat, data);
    }
}

// Process a division (timing section)
double process_division(BinaryReader& reader, ChartBuilder& builder,
                       int tracks_per_row, double current_offset) {
    NX20DivisionHeader header;
    header.start = reader.read_float();
    header.bpm = reader.read_float();
    header.split = reader.read_float();
    header.delay = reader.read_float();
    header.speed = reader.read_float();
    header.beat_split = reader.read_u8();
    header.beats_per_measure = reader.read_u8();
    header.smooth = reader.read_u8();
    header.unknown = reader.read_u8();

    // Read division extra data (unused for now)
    read_extra_data(reader);

    // Add BPM change
    double current_beat = current_offset;
    builder.add_bpm_change(current_beat, header.bpm);

    // Add delay as a stop if present
    if (header.delay > 0.0f) {
        builder.add_stop(current_beat, header.delay / 1000.0);
    }

    // Read row count
    int32_t row_count = reader.read_i32();

    // Process each row
    for (int32_t i = 0; i < row_count; ++i) {
        read_row(reader, builder, tracks_per_row, current_beat);
        // Advance beat by one subdivision
        current_beat += 1.0 / header.beat_split;
    }

    return current_beat;
}

// Process all blocks in the file
void process_blocks(BinaryReader& reader, ChartBuilder& builder, int tracks_per_row) {
    int32_t block_count = reader.read_i32();

    double current_offset = 0.0;

    for (int32_t block_idx = 0; block_idx < block_count; ++block_idx) {
        // Read random method (unused)
        /*int32_t random_method =*/ reader.read_i32();

        // Read block extra data (may contain difficulty rating)
        read_extra_data(reader, &builder);

        // Read division count
        int32_t division_count = reader.read_i32();

        // Process divisions (only use division 0 for now)
        for (int32_t div_idx = 0; div_idx < division_count; ++div_idx) {
            // Only process the first division of each block
            // (other divisions are for random/branch patterns)
            if (div_idx == 0) {
                current_offset = process_division(reader, builder, tracks_per_row, current_offset);
            } else {
                // Skip this division by reading and discarding
                process_division(reader, builder, tracks_per_row, current_offset);
            }
        }
    }
}

// Extract difficulty from filename
std::string difficulty_from_filename(const std::filesystem::path& path) {
    std::string filename = path.stem().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

    if (filename == "pr") return "Practice";
    if (filename == "no") return "Normal";
    if (filename == "hd") return "Hard";
    if (filename == "cr") return "Crazy";
    if (filename == "fr") return "Freestyle";
    if (filename == "nm") return "Nightmare";

    return "Unknown";
}

} // anonymous namespace

NxParser::NxParser() : file_reader_(read_binary_file_from_disk) {}

NxParser::NxParser(BinaryReaderFn file_reader) : file_reader_(std::move(file_reader)) {}

Chart NxParser::parse(const std::filesystem::path& chart_path) const {
    // Read file contents
    std::vector<uint8_t> content = file_reader_(chart_path);
    if (content.empty()) {
        throw ChartLoadException("NX file is empty");
    }

    BinaryReader reader(content);

    // Read and validate header
    NX20Header header;
    header.magic = reader.read_u32();
    header.half_double_flag = reader.read_u32();
    header.tracks_per_row = reader.read_u32();
    header.lightmap_flag = reader.read_u32();

    // Verify magic number
    if (header.magic != 0x3032584E) {  // "NX20" in little-endian
        throw ChartLoadException("Invalid NX20 magic number: expected 0x3032584E, got 0x" +
                                std::to_string(header.magic));
    }

    // Skip lightmap-only charts
    if (header.lightmap_flag != 0) {
        throw ChartLoadException("NX parser: lightmap-only charts are not supported");
    }

    // Determine play mode
    PlayMode mode;
    if (header.tracks_per_row == 5) {
        mode = PlayMode::SINGLE;
    } else if (header.tracks_per_row == 10) {
        mode = PlayMode::DOUBLE;
    } else {
        throw ChartLoadException("NX parser: unsupported tracks_per_row: " +
                                std::to_string(header.tracks_per_row));
    }

    ChartBuilder builder;
    builder.set_mode(mode);

    // Set difficulty from filename
    builder.set_difficulty_name(difficulty_from_filename(chart_path));

    // Use filename as title fallback (will be replaced by song metadata in Phase 4)
    builder.set_title(chart_path.stem().string());

    // Read chart-level extra data
    read_extra_data(reader, &builder);

    // Process all blocks and divisions
    process_blocks(reader, builder, header.tracks_per_row);

    // Build and return the chart
    return builder.build();
}

} // namespace openitup
