#include <gtest/gtest.h>

#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>

using namespace openitup;

// --- NoteType Tests ---

TEST(Chart, AllNoteTypesExist) {
    // Verify all 6 note types are defined and distinct
    EXPECT_EQ(static_cast<uint8_t>(NoteType::TAP), 0);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::HOLD_HEAD), 1);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::HOLD_TAIL), 2);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::MINE), 3);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::FAKE), 4);
    EXPECT_EQ(static_cast<uint8_t>(NoteType::LIFT), 5);
}

TEST(Chart, NoteTypeStringRoundTrip) {
    // Verify string conversions are inverse for all types
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::TAP)), NoteType::TAP);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::HOLD_HEAD)), NoteType::HOLD_HEAD);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::HOLD_TAIL)), NoteType::HOLD_TAIL);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::MINE)), NoteType::MINE);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::FAKE)), NoteType::FAKE);
    EXPECT_EQ(note_type_from_string(note_type_to_string(NoteType::LIFT)), NoteType::LIFT);
}

// --- PlayMode Tests ---

TEST(Chart, SingleModeHas5Columns) {
    EXPECT_EQ(max_columns(PlayMode::SINGLE), 5);
}

TEST(Chart, DoubleModeHas10Columns) {
    EXPECT_EQ(max_columns(PlayMode::DOUBLE), 10);
}

TEST(Chart, PlayModeStringRoundTrip) {
    // Verify string conversions are inverse for both modes
    EXPECT_EQ(play_mode_from_string(play_mode_to_string(PlayMode::SINGLE)), PlayMode::SINGLE);
    EXPECT_EQ(play_mode_from_string(play_mode_to_string(PlayMode::DOUBLE)), PlayMode::DOUBLE);
}
