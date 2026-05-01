#include <gtest/gtest.h>
#include <openitup/chart/nx_parser.h>
#include <openitup/chart/chart_builder.h>
#include <cstring>

using namespace openitup;

namespace {

// Helper to create a minimal NX20 binary file in memory
std::vector<uint8_t> create_minimal_nx20(uint32_t tracks = 5) {
    std::vector<uint8_t> data;

    // Helper to append data
    auto append_u32 = [&](uint32_t value) {
        data.insert(data.end(),
                   reinterpret_cast<const uint8_t*>(&value),
                   reinterpret_cast<const uint8_t*>(&value) + sizeof(uint32_t));
    };

    auto append_i32 = [&](int32_t value) {
        data.insert(data.end(),
                   reinterpret_cast<const uint8_t*>(&value),
                   reinterpret_cast<const uint8_t*>(&value) + sizeof(int32_t));
    };

    auto append_float = [&](float value) {
        data.insert(data.end(),
                   reinterpret_cast<const uint8_t*>(&value),
                   reinterpret_cast<const uint8_t*>(&value) + sizeof(float));
    };

    auto append_u8 = [&](uint8_t value) {
        data.push_back(value);
    };

    // Header
    append_u32(0x3032584E);  // "NX20" magic
    append_u32(0);           // half_double_flag
    append_u32(tracks);      // tracks_per_row
    append_u32(0);           // lightmap_flag

    // Chart-level extra data (empty)
    append_i32(0);           // extra data count

    // Block count
    append_i32(1);           // 1 block

    // Block
    append_i32(0);           // random method
    append_i32(0);           // block extra data count
    append_i32(1);           // division count

    // Division header
    append_float(0.0f);      // start time
    append_float(120.0f);    // bpm
    append_float(1.0f);      // split
    append_float(0.0f);      // delay
    append_float(1.0f);      // speed
    append_u8(4);            // beat_split (4 rows per beat)
    append_u8(4);            // beats_per_measure
    append_u8(0);            // smooth
    append_u8(0);            // unknown

    // Division extra data (empty)
    append_i32(0);           // extra data count

    // Row count
    append_i32(4);           // 4 rows

    // Row 0: tap on column 0
    append_u32(0x00010003);  // type=0x3 (tap), display=1
    if (tracks > 1) {
        for (uint32_t i = 1; i < tracks; ++i) {
            append_u32(0);   // empty columns
        }
    }

    // Row 1: empty
    append_u32(0);
    if (tracks > 1) {
        for (uint32_t i = 1; i < tracks; ++i) {
            append_u32(0);
        }
    }

    // Row 2: tap on column 2
    append_u32(0);
    if (tracks > 1) {
        append_u32(0);
        append_u32(0x00010003);  // tap on column 2
        for (uint32_t i = 3; i < tracks; ++i) {
            append_u32(0);
        }
    }

    // Row 3: empty
    append_u32(0);
    if (tracks > 1) {
        for (uint32_t i = 1; i < tracks; ++i) {
            append_u32(0);
        }
    }

    return data;
}

// Helper to create NX20 with hold notes
std::vector<uint8_t> create_nx20_with_holds() {
    std::vector<uint8_t> data;

    auto append_u32 = [&](uint32_t value) {
        data.insert(data.end(),
                   reinterpret_cast<const uint8_t*>(&value),
                   reinterpret_cast<const uint8_t*>(&value) + sizeof(uint32_t));
    };

    auto append_i32 = [&](int32_t value) {
        data.insert(data.end(),
                   reinterpret_cast<const uint8_t*>(&value),
                   reinterpret_cast<const uint8_t*>(&value) + sizeof(int32_t));
    };

    auto append_float = [&](float value) {
        data.insert(data.end(),
                   reinterpret_cast<const uint8_t*>(&value),
                   reinterpret_cast<const uint8_t*>(&value) + sizeof(float));
    };

    auto append_u8 = [&](uint8_t value) {
        data.push_back(value);
    };

    // Header
    append_u32(0x3032584E);
    append_u32(0);
    append_u32(5);
    append_u32(0);

    // Chart extra data
    append_i32(0);

    // Blocks
    append_i32(1);
    append_i32(0);           // random method
    append_i32(0);           // block extra data
    append_i32(1);           // division count

    // Division
    append_float(0.0f);
    append_float(120.0f);
    append_float(1.0f);
    append_float(0.0f);
    append_float(1.0f);
    append_u8(4);
    append_u8(4);
    append_u8(0);
    append_u8(0);

    append_i32(0);           // division extra data

    // 4 rows: hold_head, hold_body, hold_body, hold_tail
    append_i32(4);

    // Row 0: hold head on column 0
    append_u32(0x00010007);  // type=0x7 (hold head)
    for (int i = 1; i < 5; ++i) append_u32(0);

    // Row 1: hold body
    append_u32(0x0001000B);  // type=0xB (hold body)
    for (int i = 1; i < 5; ++i) append_u32(0);

    // Row 2: hold body
    append_u32(0x0001000B);
    for (int i = 1; i < 5; ++i) append_u32(0);

    // Row 3: hold tail
    append_u32(0x0001000F);  // type=0xF (hold tail)
    for (int i = 1; i < 5; ++i) append_u32(0);

    return data;
}

} // anonymous namespace

TEST(NxParser, EmptyFileThrows) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        return {};
    };

    NxParser parser(reader);
    EXPECT_THROW(parser.parse("/fake/path.nx"), ChartLoadException);
}

TEST(NxParser, InvalidMagicThrows) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        std::vector<uint8_t> data(16, 0);
        uint32_t bad_magic = 0x12345678;
        std::memcpy(data.data(), &bad_magic, sizeof(bad_magic));
        return data;
    };

    NxParser parser(reader);
    EXPECT_THROW(parser.parse("/fake/path.nx"), ChartLoadException);
}

TEST(NxParser, ValidMinimalNx20Parsed) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        return create_minimal_nx20();
    };

    NxParser parser(reader);
    Chart chart = parser.parse("/fake/path.nx");

    // Should have 2 tap notes (at rows 0 and 2)
    EXPECT_EQ(chart.note_count(), 2);

    // Should have 120 BPM
    EXPECT_DOUBLE_EQ(chart.timing_data().bpm_at_beat(0.0), 120.0);

    // Should be single mode
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);
}

TEST(NxParser, DoubleModeDetected) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        return create_minimal_nx20(10);  // 10 tracks = double
    };

    NxParser parser(reader);
    Chart chart = parser.parse("/fake/path.nx");

    EXPECT_EQ(chart.metadata().mode, PlayMode::DOUBLE);
}

TEST(NxParser, NotePositionsCorrect) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        return create_minimal_nx20();
    };

    NxParser parser(reader);
    Chart chart = parser.parse("/fake/path.nx");

    const auto& notes = chart.note_data().events();
    ASSERT_EQ(notes.size(), 2);

    // beat_split = 4, so each row is 1/4 beat
    // Row 0 -> beat 0.0
    // Row 2 -> beat 0.5
    EXPECT_DOUBLE_EQ(notes[0].beat, 0.0);
    EXPECT_EQ(notes[0].column, 0);
    EXPECT_EQ(notes[0].type, NoteType::TAP);

    EXPECT_DOUBLE_EQ(notes[1].beat, 0.5);
    EXPECT_EQ(notes[1].column, 2);
    EXPECT_EQ(notes[1].type, NoteType::TAP);
}

TEST(NxParser, HoldNotesConverted) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        return create_nx20_with_holds();
    };

    NxParser parser(reader);
    Chart chart = parser.parse("/fake/path.nx");

    const auto& notes = chart.note_data().events();

    // Should have hold_head at beat 0 and hold_tail at beat 0.75
    // (4 rows at 4 rows per beat = 0, 0.25, 0.5, 0.75)
    bool found_head = false;
    bool found_tail = false;

    for (const auto& note : notes) {
        if (note.type == NoteType::HOLD_HEAD && note.beat == 0.0) {
            found_head = true;
        }
        if (note.type == NoteType::HOLD_TAIL && note.beat == 0.75) {
            found_tail = true;
        }
    }

    EXPECT_TRUE(found_head);
    EXPECT_TRUE(found_tail);
}

TEST(NxParser, DifficultyFromFilename) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        return create_minimal_nx20();
    };

    NxParser parser(reader);

    Chart chart_cr = parser.parse("/fake/path/cr.nx");
    EXPECT_EQ(chart_cr.metadata().difficulty_name, "Crazy");

    Chart chart_hd = parser.parse("/fake/path/hd.nx");
    EXPECT_EQ(chart_hd.metadata().difficulty_name, "Hard");

    Chart chart_no = parser.parse("/fake/path/no.nx");
    EXPECT_EQ(chart_no.metadata().difficulty_name, "Normal");
}

TEST(NxParser, LightmapOnlyThrows) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        std::vector<uint8_t> data = create_minimal_nx20();
        // Set lightmap_flag to 1 (byte offset 12-15)
        uint32_t lightmap = 1;
        std::memcpy(data.data() + 12, &lightmap, sizeof(lightmap));
        return data;
    };

    NxParser parser(reader);
    EXPECT_THROW(parser.parse("/fake/path.nx"), ChartLoadException);
}

TEST(NxParser, InvalidTracksPerRowThrows) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        return create_minimal_nx20(7);  // Invalid track count
    };

    NxParser parser(reader);
    EXPECT_THROW(parser.parse("/fake/path.nx"), ChartLoadException);
}

TEST(NxParser, InvisibleNotesSkipped) {
    auto reader = [](const std::filesystem::path&) -> std::vector<uint8_t> {
        std::vector<uint8_t> data;

        auto append_u32 = [&](uint32_t value) {
            data.insert(data.end(),
                       reinterpret_cast<const uint8_t*>(&value),
                       reinterpret_cast<const uint8_t*>(&value) + sizeof(uint32_t));
        };

        auto append_i32 = [&](int32_t value) {
            data.insert(data.end(),
                       reinterpret_cast<const uint8_t*>(&value),
                       reinterpret_cast<const uint8_t*>(&value) + sizeof(int32_t));
        };

        auto append_float = [&](float value) {
            data.insert(data.end(),
                       reinterpret_cast<const uint8_t*>(&value),
                       reinterpret_cast<const uint8_t*>(&value) + sizeof(float));
        };

        auto append_u8 = [&](uint8_t value) {
            data.push_back(value);
        };

        // Header
        append_u32(0x3032584E);
        append_u32(0);
        append_u32(5);
        append_u32(0);

        // Chart extra data
        append_i32(0);

        // Blocks
        append_i32(1);
        append_i32(0);
        append_i32(0);
        append_i32(1);

        // Division
        append_float(0.0f);
        append_float(120.0f);
        append_float(1.0f);
        append_float(0.0f);
        append_float(1.0f);
        append_u8(4);
        append_u8(4);
        append_u8(0);
        append_u8(0);

        append_i32(0);

        // 1 row with invisible note (display=0)
        append_i32(1);

        // Row 0: tap with display=0 (invisible)
        append_u32(0x00000003);  // type=0x3, display=0
        for (int i = 1; i < 5; ++i) append_u32(0);

        return data;
    };

    NxParser parser(reader);
    Chart chart = parser.parse("/fake/path.nx");

    // Should have 0 notes (invisible note skipped)
    EXPECT_EQ(chart.note_count(), 0);
}
