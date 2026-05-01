#include <gtest/gtest.h>
#include <openitup/chart/chart_hasher.h>
#include <openitup/chart/chart_builder.h>

using namespace openitup;

TEST(ChartHasher, ComputesHashForSimpleChart) {
    ChartBuilder builder;
    builder.set_title("Test");
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 2, NoteType::TAP);
    builder.add_note(1.0, 1, NoteType::TAP);

    Chart chart = builder.build();

    std::string hash = compute_chart_hash(chart.note_data(), chart.timing_data());

    // Hash should be a hex string
    EXPECT_FALSE(hash.empty());
    // SHA-256 produces 64 hex characters (or fallback produces at least 16)
    EXPECT_GE(hash.length(), 16);

    // Hash should only contain hex characters
    for (char c : hash) {
        EXPECT_TRUE(std::isxdigit(c));
    }
}

TEST(ChartHasher, SameChartProducesSameHash) {
    ChartBuilder builder1;
    builder1.set_title("Test 1");
    builder1.set_mode(PlayMode::SINGLE);
    builder1.add_bpm_change(0.0, 140.0);
    builder1.add_note(0.0, 2, NoteType::TAP);
    Chart chart1 = builder1.build();

    ChartBuilder builder2;
    builder2.set_title("Test 2"); // Different title
    builder2.set_mode(PlayMode::SINGLE);
    builder2.add_bpm_change(0.0, 140.0);
    builder2.add_note(0.0, 2, NoteType::TAP);
    Chart chart2 = builder2.build();

    std::string hash1 = compute_chart_hash(chart1.note_data(), chart1.timing_data());
    std::string hash2 = compute_chart_hash(chart2.note_data(), chart2.timing_data());

    // Same notes and timing should produce same hash despite different metadata
    EXPECT_EQ(hash1, hash2);
}

TEST(ChartHasher, DifferentNotesProduceDifferentHash) {
    ChartBuilder builder1;
    builder1.set_title("Test");
    builder1.set_mode(PlayMode::SINGLE);
    builder1.add_bpm_change(0.0, 120.0);
    builder1.add_note(0.0, 2, NoteType::TAP);
    Chart chart1 = builder1.build();

    ChartBuilder builder2;
    builder2.set_title("Test");
    builder2.set_mode(PlayMode::SINGLE);
    builder2.add_bpm_change(0.0, 120.0);
    builder2.add_note(0.0, 3, NoteType::TAP); // Different column
    Chart chart2 = builder2.build();

    std::string hash1 = compute_chart_hash(chart1.note_data(), chart1.timing_data());
    std::string hash2 = compute_chart_hash(chart2.note_data(), chart2.timing_data());

    EXPECT_NE(hash1, hash2);
}

TEST(ChartHasher, DifferentTimingProducesDifferentHash) {
    ChartBuilder builder1;
    builder1.set_title("Test");
    builder1.set_mode(PlayMode::SINGLE);
    builder1.add_bpm_change(0.0, 120.0);
    builder1.add_note(0.0, 2, NoteType::TAP);
    Chart chart1 = builder1.build();

    ChartBuilder builder2;
    builder2.set_title("Test");
    builder2.set_mode(PlayMode::SINGLE);
    builder2.add_bpm_change(0.0, 140.0); // Different BPM
    builder2.add_note(0.0, 2, NoteType::TAP);
    Chart chart2 = builder2.build();

    std::string hash1 = compute_chart_hash(chart1.note_data(), chart1.timing_data());
    std::string hash2 = compute_chart_hash(chart2.note_data(), chart2.timing_data());

    EXPECT_NE(hash1, hash2);
}

TEST(ChartHasher, BytesToHex) {
    std::vector<uint8_t> bytes = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    std::string hex = bytes_to_hex(bytes);

    EXPECT_EQ(hex, "0123456789abcdef");
}
