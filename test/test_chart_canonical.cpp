#include <gtest/gtest.h>

#include <openitup/chart/chart_canonical.h>
#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>
#include <openitup/chart/note_type.h>

#include <algorithm>

using namespace openitup;

// --- Tests for canonical_bytes() ---

TEST(ChartCanonical, SameNotesInDifferentOrderProduceSameBytes) {
    // Create two NoteData with identical notes but added in different orders.
    // NoteData constructor sorts them, so canonical_bytes should produce identical output.

    std::vector<NoteEvent> events_order1 = {
        {4.0, 0, NoteType::TAP},
        {2.0, 1, NoteType::HOLD_HEAD},
        {8.0, 2, NoteType::TAP}
    };

    std::vector<NoteEvent> events_order2 = {
        {8.0, 2, NoteType::TAP},
        {4.0, 0, NoteType::TAP},
        {2.0, 1, NoteType::HOLD_HEAD}
    };

    NoteData notes1(events_order1);
    NoteData notes2(events_order2);

    // Use default timing (120 BPM at beat 0)
    TimingData timing;

    auto bytes1 = canonical_bytes(notes1, timing);
    auto bytes2 = canonical_bytes(notes2, timing);

    EXPECT_EQ(bytes1, bytes2);
}

TEST(ChartCanonical, DifferentNotesProduceDifferentBytes) {
    // Two charts with different notes should produce different bytes.

    std::vector<NoteEvent> events1 = {
        {4.0, 0, NoteType::TAP}
    };

    std::vector<NoteEvent> events2 = {
        {4.0, 0, NoteType::TAP},
        {5.0, 1, NoteType::TAP}  // Additional note
    };

    NoteData notes1(events1);
    NoteData notes2(events2);

    TimingData timing;

    auto bytes1 = canonical_bytes(notes1, timing);
    auto bytes2 = canonical_bytes(notes2, timing);

    EXPECT_NE(bytes1, bytes2);
}

TEST(ChartCanonical, DifferentNoteTypeProducesDifferentBytes) {
    // Same beat and column, but different note type.

    std::vector<NoteEvent> events1 = {
        {4.0, 0, NoteType::TAP}
    };

    std::vector<NoteEvent> events2 = {
        {4.0, 0, NoteType::HOLD_HEAD}  // Different type
    };

    NoteData notes1(events1);
    NoteData notes2(events2);

    TimingData timing;

    auto bytes1 = canonical_bytes(notes1, timing);
    auto bytes2 = canonical_bytes(notes2, timing);

    EXPECT_NE(bytes1, bytes2);
}

TEST(ChartCanonical, DifferentBeatProducesDifferentBytes) {
    // Same column and type, but different beat.

    std::vector<NoteEvent> events1 = {
        {4.0, 0, NoteType::TAP}
    };

    std::vector<NoteEvent> events2 = {
        {5.0, 0, NoteType::TAP}  // Different beat
    };

    NoteData notes1(events1);
    NoteData notes2(events2);

    TimingData timing;

    auto bytes1 = canonical_bytes(notes1, timing);
    auto bytes2 = canonical_bytes(notes2, timing);

    EXPECT_NE(bytes1, bytes2);
}

TEST(ChartCanonical, DifferentColumnProducesDifferentBytes) {
    // Same beat and type, but different column.

    std::vector<NoteEvent> events1 = {
        {4.0, 0, NoteType::TAP}
    };

    std::vector<NoteEvent> events2 = {
        {4.0, 1, NoteType::TAP}  // Different column
    };

    NoteData notes1(events1);
    NoteData notes2(events2);

    TimingData timing;

    auto bytes1 = canonical_bytes(notes1, timing);
    auto bytes2 = canonical_bytes(notes2, timing);

    EXPECT_NE(bytes1, bytes2);
}

TEST(ChartCanonical, DifferentTimingProducesDifferentBytes) {
    // Same notes, but different timing events.

    std::vector<NoteEvent> events = {
        {4.0, 0, NoteType::TAP}
    };

    NoteData notes(events);

    // Timing 1: default (120 BPM at beat 0)
    TimingData timing1;

    // Timing 2: BPM change at beat 8
    std::vector<TimingEvent> timing_events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing2(timing_events);

    auto bytes1 = canonical_bytes(notes, timing1);
    auto bytes2 = canonical_bytes(notes, timing2);

    EXPECT_NE(bytes1, bytes2);
}

TEST(ChartCanonical, DifferentStopProducesDifferentBytes) {
    // Same notes, but different stop events.

    std::vector<NoteEvent> events = {
        {4.0, 0, NoteType::TAP}
    };

    NoteData notes(events);

    // Timing 1: 120 BPM with 1-second stop at beat 4
    std::vector<TimingEvent> timing_events1 = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {4.0, TimingEventType::STOP, 0.0, 1.0}
    };
    TimingData timing1(timing_events1);

    // Timing 2: 120 BPM with 2-second stop at beat 4
    std::vector<TimingEvent> timing_events2 = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {4.0, TimingEventType::STOP, 0.0, 2.0}
    };
    TimingData timing2(timing_events2);

    auto bytes1 = canonical_bytes(notes, timing1);
    auto bytes2 = canonical_bytes(notes, timing2);

    EXPECT_NE(bytes1, bytes2);
}

TEST(ChartCanonical, RoundTripIsDeterministic) {
    // Serialize the same chart multiple times — should always produce identical bytes.

    std::vector<NoteEvent> events = {
        {4.0, 0, NoteType::TAP},
        {5.0, 1, NoteType::HOLD_HEAD},
        {6.0, 1, NoteType::HOLD_TAIL}
    };

    NoteData notes(events);

    std::vector<TimingEvent> timing_events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing(timing_events);

    auto bytes1 = canonical_bytes(notes, timing);
    auto bytes2 = canonical_bytes(notes, timing);
    auto bytes3 = canonical_bytes(notes, timing);

    EXPECT_EQ(bytes1, bytes2);
    EXPECT_EQ(bytes2, bytes3);
}

TEST(ChartCanonical, EmptyChartProducesValidBytes) {
    // Empty note data with default timing should produce valid (non-empty) bytes.

    NoteData notes;  // Empty
    TimingData timing;  // Default: 120 BPM at beat 0

    auto bytes = canonical_bytes(notes, timing);

    // Should have:
    // - 4 bytes for note count (0)
    // - 4 bytes for timing count (1, the default BPM event)
    // - 17 bytes for the one timing event (8 + 1 + 8)
    // Total: 4 + 4 + 17 = 25 bytes

    EXPECT_EQ(bytes.size(), 25u);
}

TEST(ChartCanonical, ByteFormatIncludesNoteCount) {
    // Verify the first 4 bytes contain the note count (little-endian uint32).

    std::vector<NoteEvent> events = {
        {4.0, 0, NoteType::TAP},
        {5.0, 1, NoteType::TAP}
    };

    NoteData notes(events);
    TimingData timing;

    auto bytes = canonical_bytes(notes, timing);

    // First 4 bytes should be note count = 2 (little-endian)
    ASSERT_GE(bytes.size(), 4u);

    uint32_t note_count = static_cast<uint32_t>(bytes[0])
                        | (static_cast<uint32_t>(bytes[1]) << 8)
                        | (static_cast<uint32_t>(bytes[2]) << 16)
                        | (static_cast<uint32_t>(bytes[3]) << 24);

    EXPECT_EQ(note_count, 2u);
}

TEST(ChartCanonical, ByteFormatIncludesTimingCount) {
    // Verify the timing count appears after the note data.

    std::vector<NoteEvent> events = {
        {4.0, 0, NoteType::TAP}
    };

    NoteData notes(events);

    std::vector<TimingEvent> timing_events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing(timing_events);

    auto bytes = canonical_bytes(notes, timing);

    // Byte layout:
    // - 4 bytes: note count (1)
    // - 10 bytes: one note (8 + 1 + 1)
    // - 4 bytes: timing count (2)
    // Total so far: 18 bytes

    ASSERT_GE(bytes.size(), 18u);

    uint32_t timing_count = static_cast<uint32_t>(bytes[14])
                          | (static_cast<uint32_t>(bytes[15]) << 8)
                          | (static_cast<uint32_t>(bytes[16]) << 16)
                          | (static_cast<uint32_t>(bytes[17]) << 24);

    EXPECT_EQ(timing_count, 2u);
}
