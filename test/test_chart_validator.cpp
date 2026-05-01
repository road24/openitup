#include <gtest/gtest.h>

#include <openitup/chart/chart_validator.h>
#include <openitup/chart/chart_builder.h>

using namespace openitup;

// --- US-CHT-016: Validate Charts for Common Errors ---

TEST(ChartValidator, HoldNoteWithoutTailIsDetected) {
    // Scenario 1: Hold note without tail is detected
    // Given a chart with a hold_head at beat 4.0 column 2 but no hold_tail
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(4.0, 2, NoteType::HOLD_HEAD);
    builder.add_note(8.0, 0, NoteType::TAP);
    Chart chart = builder.build();

    // When the validator runs
    auto warnings = ChartValidator::validate(chart);

    // Then a warning is logged: "Hold note at beat 4.0 column 2 has no tail"
    ASSERT_FALSE(warnings.empty());
    bool found_orphan = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("no tail") != std::string::npos &&
            warning.beat == 4.0 && warning.column == 2) {
            found_orphan = true;
            break;
        }
    }
    EXPECT_TRUE(found_orphan);
}

TEST(ChartValidator, OverlappingNotesInSameColumnAreDetected) {
    // Scenario 2: Overlapping notes in same column are detected
    // Given a chart with two tap notes at beat 4.0 column 1
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(4.0, 1, NoteType::TAP);
    builder.add_note(4.0, 1, NoteType::TAP);
    Chart chart = builder.build();

    // When the validator runs
    auto warnings = ChartValidator::validate(chart);

    // Then a warning is logged: "Overlapping notes at beat 4.0 column 1"
    ASSERT_FALSE(warnings.empty());
    bool found_overlap = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("Overlapping") != std::string::npos &&
            warning.beat == 4.0 && warning.column == 1) {
            found_overlap = true;
            break;
        }
    }
    EXPECT_TRUE(found_overlap);
}

TEST(ChartValidator, NegativeBeatPositionIsDetected) {
    // Scenario 3: Negative beat position is detected
    // Given a chart with a note at beat -1.0
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(-1.0, 0, NoteType::TAP);
    builder.add_note(4.0, 1, NoteType::TAP);
    Chart chart = builder.build();

    // When the validator runs
    auto warnings = ChartValidator::validate(chart);

    // Then a warning is logged: "Invalid negative beat position: -1.0"
    ASSERT_FALSE(warnings.empty());
    bool found_negative = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("negative beat") != std::string::npos &&
            warning.beat == -1.0) {
            found_negative = true;
            EXPECT_EQ(warning.severity, ValidationSeverity::ERROR);
            break;
        }
    }
    EXPECT_TRUE(found_negative);
}

TEST(ChartValidator, BpmIsZeroOrNegativeDetected) {
    // Scenario 4: BPM is zero or negative
    // Given a chart with a BPM event setting BPM to 0
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 0.0);  // Invalid BPM
    builder.add_note(4.0, 1, NoteType::TAP);

    // When the validator runs - this throws during build
    EXPECT_THROW(builder.build(), ChartLoadException);
}

TEST(ChartValidator, ValidationWarningsDoNotPreventLoading) {
    // Scenario 5: Validation warnings do not prevent loading
    // Given a chart with one validation warning (orphan hold)
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(4.0, 2, NoteType::HOLD_HEAD);  // No tail
    Chart chart = builder.build();

    // When the chart is loaded - it should not throw
    // Then the Chart struct is still returned and the warning is logged at WARN level
    auto warnings = ChartValidator::validate(chart);
    ASSERT_FALSE(warnings.empty());

    // Chart is still valid and usable
    EXPECT_EQ(chart.note_count(), 2);
    EXPECT_FALSE(chart.note_data().empty());
}

TEST(ChartValidator, ValidChartHasNoWarnings) {
    // A completely valid chart should have no warnings
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(2.0, 1, NoteType::HOLD_HEAD);
    builder.add_note(4.0, 1, NoteType::HOLD_TAIL);
    builder.add_note(6.0, 2, NoteType::TAP);
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    EXPECT_TRUE(warnings.empty());
}

TEST(ChartValidator, EmptyChartDetected) {
    // A chart with no notes should be flagged
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    // No notes added
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    ASSERT_FALSE(warnings.empty());

    bool found_empty = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("no notes") != std::string::npos) {
            found_empty = true;
            EXPECT_EQ(warning.severity, ValidationSeverity::ERROR);
            break;
        }
    }
    EXPECT_TRUE(found_empty);
}

TEST(ChartValidator, ColumnOutOfRangeForSingleMode) {
    // Column index >= 5 in SINGLE mode should be flagged
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(2.0, 7, NoteType::TAP);  // Out of range for SINGLE
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    ASSERT_FALSE(warnings.empty());

    bool found_range = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("out of range") != std::string::npos &&
            warning.column == 7) {
            found_range = true;
            break;
        }
    }
    EXPECT_TRUE(found_range);
}

TEST(ChartValidator, ColumnOutOfRangeForDoubleMode) {
    // Column index >= 10 in DOUBLE mode should be flagged
    ChartBuilder builder;
    builder.set_mode(PlayMode::DOUBLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(2.0, 9, NoteType::TAP);   // Valid for DOUBLE
    builder.add_note(4.0, 10, NoteType::TAP);  // Out of range for DOUBLE
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    ASSERT_FALSE(warnings.empty());

    bool found_range = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("out of range") != std::string::npos &&
            warning.column == 10) {
            found_range = true;
            break;
        }
    }
    EXPECT_TRUE(found_range);
}

TEST(ChartValidator, UnreasonablyHighBpmWarning) {
    // BPM > 999 should generate a warning
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 1200.0);  // Unreasonable BPM
    builder.add_note(0.0, 0, NoteType::TAP);
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    ASSERT_FALSE(warnings.empty());

    bool found_unreasonable = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("Unreasonable BPM") != std::string::npos) {
            found_unreasonable = true;
            EXPECT_EQ(warning.severity, ValidationSeverity::WARNING);
            break;
        }
    }
    EXPECT_TRUE(found_unreasonable);
}

TEST(ChartValidator, HoldTailWithoutHeadDetected) {
    // Hold tail without a preceding hold head should be flagged
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(4.0, 2, NoteType::HOLD_TAIL);  // No preceding head
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    ASSERT_FALSE(warnings.empty());

    bool found_orphan_tail = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("without preceding hold head") != std::string::npos &&
            warning.beat == 4.0 && warning.column == 2) {
            found_orphan_tail = true;
            break;
        }
    }
    EXPECT_TRUE(found_orphan_tail);
}

TEST(ChartValidator, MultipleHoldsInSameColumnValidated) {
    // Multiple hold notes in the same column should be validated correctly
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 2, NoteType::HOLD_HEAD);
    builder.add_note(2.0, 2, NoteType::HOLD_TAIL);
    builder.add_note(4.0, 2, NoteType::HOLD_HEAD);
    builder.add_note(6.0, 2, NoteType::HOLD_TAIL);
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    EXPECT_TRUE(warnings.empty());
}

TEST(ChartValidator, OverlappingHoldHeadsDetected) {
    // Two hold heads in the same column without a tail between them
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 2, NoteType::HOLD_HEAD);
    builder.add_note(4.0, 2, NoteType::HOLD_HEAD);  // Second head before first tail
    builder.add_note(8.0, 2, NoteType::HOLD_TAIL);
    Chart chart = builder.build();

    auto warnings = ChartValidator::validate(chart);
    ASSERT_FALSE(warnings.empty());

    // Should detect orphan hold (first one has no tail before second head)
    bool found_orphan = false;
    for (const auto& warning : warnings) {
        if (warning.message.find("no tail") != std::string::npos &&
            warning.beat == 0.0 && warning.column == 2) {
            found_orphan = true;
            break;
        }
    }
    EXPECT_TRUE(found_orphan);
}
