#include <gtest/gtest.h>
#include <openitup/chart/chart_builder.h>
#include <openitup/chart/stx_parser.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

#include <fstream>
#include <filesystem>
#include <cstring>
#include <zlib.h>

using namespace openitup;

namespace {

// Helper: write little-endian uint32
void write_u32(std::vector<uint8_t>& data, uint32_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
}

// Helper: write little-endian float
void write_float(std::vector<uint8_t>& data, float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    write_u32(data, bits);
}

// Helper: write fixed-size string (null-padded)
void write_string(std::vector<uint8_t>& data, const std::string& str, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data.push_back(i < str.length() ? str[i] : 0);
    }
}

// Helper: compress data with zlib
std::vector<uint8_t> compress_data(const std::vector<uint8_t>& input) {
    uLongf compressed_size = compressBound(input.size());
    std::vector<uint8_t> output(compressed_size);

    int result = compress(output.data(), &compressed_size, input.data(), input.size());
    if (result != Z_OK) {
        throw std::runtime_error("zlib compression failed");
    }

    output.resize(compressed_size);
    return output;
}

// Helper: create a minimal STX file with one track containing one block
std::vector<uint8_t> create_minimal_stx() {
    std::vector<uint8_t> stx;

    // Header
    stx.push_back('S');
    stx.push_back('T');
    stx.push_back('F');
    stx.push_back('4');

    // Padding (14 uint32s = 56 bytes)
    for (int i = 0; i < 14; ++i) {
        write_u32(stx, 0);
    }

    // Title, Artist, StepAuthor (64 bytes each)
    write_string(stx, "Test Song", 64);
    write_string(stx, "Test Artist", 64);
    write_string(stx, "Test Charter", 64);

    // Track offsets (9 uint32s)
    // Only Normal track (index 1) has data
    write_u32(stx, 0); // Practice
    write_u32(stx, 256); // Normal - will be at offset 256
    write_u32(stx, 0); // Hard
    write_u32(stx, 0); // Nightmare
    write_u32(stx, 0); // Crazy
    write_u32(stx, 0); // FullDouble
    write_u32(stx, 0); // HalfDouble
    write_u32(stx, 0); // Division
    write_u32(stx, 0); // Lights

    // Pad to offset 256
    while (stx.size() < 256) {
        stx.push_back(0);
    }

    // Track header at offset 256
    write_u32(stx, 5); // difficulty = 5

    // mBlocks[50] - only first block has 1 sub-block
    write_u32(stx, 1); // mBlocks[0] = 1
    for (int i = 1; i < 50; ++i) {
        write_u32(stx, 0);
    }

    // Create block data (uncompressed)
    std::vector<uint8_t> block_data;

    // StxBlock header
    write_float(block_data, 140.0f); // BPM
    write_u32(block_data, 4); // beat_per_measure
    write_u32(block_data, 4); // beat_split (tickcount)
    write_u32(block_data, 0); // delay

    // DivisionSet (80 bytes)
    for (int i = 0; i < 20; ++i) {
        write_u32(block_data, 0);
    }

    write_u32(block_data, 1000); // speed = 1.0 * 1000

    // Pad[7]
    for (int i = 0; i < 7; ++i) {
        write_u32(block_data, 0);
    }

    write_u32(block_data, 4); // block_length = 4 rows

    // Row data (4 rows, 13 bytes each)
    // Row 0: tap on column 0
    uint8_t row0[13] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    block_data.insert(block_data.end(), row0, row0 + 13);

    // Row 1: empty
    uint8_t row1[13] = {0};
    block_data.insert(block_data.end(), row1, row1 + 13);

    // Row 2: tap on column 2
    uint8_t row2[13] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    block_data.insert(block_data.end(), row2, row2 + 13);

    // Row 3: empty
    uint8_t row3[13] = {0};
    block_data.insert(block_data.end(), row3, row3 + 13);

    // Compress block
    std::vector<uint8_t> compressed = compress_data(block_data);

    // Write packed_block_t (size + data)
    write_u32(stx, compressed.size());
    stx.insert(stx.end(), compressed.begin(), compressed.end());

    return stx;
}

// Helper: create STX with hold notes
std::vector<uint8_t> create_stx_with_holds() {
    std::vector<uint8_t> stx;

    // Header
    stx.push_back('S');
    stx.push_back('T');
    stx.push_back('F');
    stx.push_back('4');

    // Padding
    for (int i = 0; i < 14; ++i) {
        write_u32(stx, 0);
    }

    // Title, Artist, StepAuthor
    write_string(stx, "Hold Test", 64);
    write_string(stx, "Artist", 64);
    write_string(stx, "Charter", 64);

    // Track offsets
    write_u32(stx, 0); // Practice
    write_u32(stx, 256); // Normal
    for (int i = 2; i < 9; ++i) {
        write_u32(stx, 0);
    }

    // Pad to offset 256
    while (stx.size() < 256) {
        stx.push_back(0);
    }

    // Track header
    write_u32(stx, 3); // difficulty
    write_u32(stx, 1); // mBlocks[0] = 1
    for (int i = 1; i < 50; ++i) {
        write_u32(stx, 0);
    }

    // Create block with hold notes
    std::vector<uint8_t> block_data;

    write_float(block_data, 120.0f); // BPM
    write_u32(block_data, 4);
    write_u32(block_data, 4);
    write_u32(block_data, 0);

    // DivisionSet + speed + pad
    for (int i = 0; i < 20; ++i) {
        write_u32(block_data, 0);
    }
    write_u32(block_data, 1000);
    for (int i = 0; i < 7; ++i) {
        write_u32(block_data, 0);
    }

    write_u32(block_data, 5); // 5 rows

    // Row 0: hold start on column 0
    uint8_t row0[13] = {10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    block_data.insert(block_data.end(), row0, row0 + 13);

    // Row 1: hold body
    uint8_t row1[13] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    block_data.insert(block_data.end(), row1, row1 + 13);

    // Row 2: hold body
    uint8_t row2[13] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    block_data.insert(block_data.end(), row2, row2 + 13);

    // Row 3: hold end (implicit - empty cell ends hold)
    uint8_t row3[13] = {0};
    block_data.insert(block_data.end(), row3, row3 + 13);

    // Row 4: regular tap on column 1
    uint8_t row4[13] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    block_data.insert(block_data.end(), row4, row4 + 13);

    std::vector<uint8_t> compressed = compress_data(block_data);
    write_u32(stx, compressed.size());
    stx.insert(stx.end(), compressed.begin(), compressed.end());

    return stx;
}

// Helper: write STX data to a temporary file and return path
std::filesystem::path write_temp_stx(const std::vector<uint8_t>& data) {
    std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "test_stx_parser_temp.stx";
    std::ofstream file(temp_path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    return temp_path;
}

} // anonymous namespace

TEST(StxParser, ParseMinimalStx) {
    auto stx_data = create_minimal_stx();
    auto temp_path = write_temp_stx(stx_data);

    StxParser parser;
    auto charts = parser.parse(temp_path);

    // Should have one chart (Normal track)
    ASSERT_EQ(charts.size(), 1);

    auto& chart = charts[0];
    EXPECT_EQ(chart.metadata().title, "Test Song");
    EXPECT_EQ(chart.metadata().artist, "Test Artist");
    EXPECT_EQ(chart.metadata().charter_name, "Test Charter");
    EXPECT_EQ(chart.metadata().difficulty_name, "Normal");
    EXPECT_EQ(chart.metadata().difficulty_rating, 5);
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);

    std::filesystem::remove(temp_path);
}

TEST(StxParser, BpmExtracted) {
    auto stx_data = create_minimal_stx();
    auto temp_path = write_temp_stx(stx_data);

    StxParser parser;
    auto charts = parser.parse(temp_path);

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 140.0);
    EXPECT_DOUBLE_EQ(chart.metadata().display_bpm, 140.0);

    std::filesystem::remove(temp_path);
}

TEST(StxParser, NotesExtracted) {
    auto stx_data = create_minimal_stx();
    auto temp_path = write_temp_stx(stx_data);

    StxParser parser;
    auto charts = parser.parse(temp_path);

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    auto& notes = chart.note_data().events();
    ASSERT_EQ(notes.size(), 2);

    // First note: column 0, beat 0.0
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);
    EXPECT_EQ(notes[0].column, 0);
    EXPECT_EQ(notes[0].type, NoteType::TAP);

    // Second note: column 2, beat 0.5 (row 2, tickcount 4)
    EXPECT_DOUBLE_EQ(notes[1].beat, 0.5);
    EXPECT_EQ(notes[1].column, 2);
    EXPECT_EQ(notes[1].type, NoteType::TAP);

    std::filesystem::remove(temp_path);
}

TEST(StxParser, HoldNotesExtracted) {
    auto stx_data = create_stx_with_holds();
    auto temp_path = write_temp_stx(stx_data);

    StxParser parser;
    auto charts = parser.parse(temp_path);

    ASSERT_EQ(charts.size(), 1);
    auto& chart = charts[0];

    auto& notes = chart.note_data().events();
    ASSERT_EQ(notes.size(), 4);

    // Hold head at row 0
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);
    EXPECT_EQ(notes[0].column, 0);
    EXPECT_EQ(notes[0].type, NoteType::HOLD_HEAD);

    // Hold tail at row 3 (implicit end)
    EXPECT_DOUBLE_EQ(notes[1].beat, 0.75);
    EXPECT_EQ(notes[1].column, 0);
    EXPECT_EQ(notes[1].type, NoteType::HOLD_TAIL);

    // Regular tap at row 4
    EXPECT_DOUBLE_EQ(notes[2].beat, 1.0);
    EXPECT_EQ(notes[2].column, 1);
    EXPECT_EQ(notes[2].type, NoteType::TAP);

    std::filesystem::remove(temp_path);
}

TEST(StxParser, InvalidMagicThrows) {
    std::vector<uint8_t> bad_stx;

    // Wrong magic
    bad_stx.push_back('X');
    bad_stx.push_back('X');
    bad_stx.push_back('X');
    bad_stx.push_back('X');

    // Pad to minimum size
    while (bad_stx.size() < 256) {
        bad_stx.push_back(0);
    }

    auto temp_path = write_temp_stx(bad_stx);

    StxParser parser;
    EXPECT_THROW(parser.parse(temp_path), ChartLoadException);

    std::filesystem::remove(temp_path);
}

TEST(StxParser, EmptyTracksSkipped) {
    std::vector<uint8_t> stx;

    // Header
    stx.push_back('S');
    stx.push_back('T');
    stx.push_back('F');
    stx.push_back('4');

    // Padding
    for (int i = 0; i < 14; ++i) {
        write_u32(stx, 0);
    }

    // Title, Artist, StepAuthor
    write_string(stx, "Empty", 64);
    write_string(stx, "Artist", 64);
    write_string(stx, "Charter", 64);

    // All track offsets are 0 (no tracks)
    for (int i = 0; i < 9; ++i) {
        write_u32(stx, 0);
    }

    auto temp_path = write_temp_stx(stx);

    StxParser parser;
    auto charts = parser.parse(temp_path);

    // Should return empty vector
    EXPECT_EQ(charts.size(), 0);

    std::filesystem::remove(temp_path);
}
