#include <gtest/gtest.h>

#include <openitup/chart/chart_classifier.h>
#include <openitup/chart/chart_builder.h>

using namespace openitup;

// --- US-CHT-017: Classify Chart Difficulty and Mode ---

TEST(ChartClassifier, SingleModeIsIdentified) {
    // Scenario 1: Single mode is identified
    // Given a chart with 5 columns (indices 0-4)
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(1.0, 1, NoteType::TAP);
    builder.add_note(2.0, 2, NoteType::TAP);
    builder.add_note(3.0, 3, NoteType::TAP);
    builder.add_note(4.0, 4, NoteType::TAP);
    Chart chart = builder.build();

    // When the chart is loaded
    // Then the Chart metadata mode field is "Single"
    EXPECT_EQ(chart.metadata().mode, PlayMode::SINGLE);

    // Auto-classify should also detect SINGLE
    PlayMode classified = ChartClassifier::auto_classify_mode(chart.note_data());
    EXPECT_EQ(classified, PlayMode::SINGLE);
}

TEST(ChartClassifier, DoubleModeIsIdentified) {
    // Scenario 2: Double mode is identified
    // Given a chart with 10 columns (indices 0-9)
    ChartBuilder builder;
    builder.set_mode(PlayMode::DOUBLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_note(0.0, 0, NoteType::TAP);
    builder.add_note(1.0, 5, NoteType::TAP);  // Column 5 = DOUBLE
    builder.add_note(2.0, 9, NoteType::TAP);
    Chart chart = builder.build();

    // When the chart is loaded
    // Then the Chart metadata mode field is "Double"
    EXPECT_EQ(chart.metadata().mode, PlayMode::DOUBLE);

    // Auto-classify should detect DOUBLE
    PlayMode classified = ChartClassifier::auto_classify_mode(chart.note_data());
    EXPECT_EQ(classified, PlayMode::DOUBLE);
}

TEST(ChartClassifier, AutoClassifySingleMode) {
    // Auto-classify a chart that only uses columns 0-4
    NoteData note_data(std::vector<NoteEvent>{
        {0.0, 0, NoteType::TAP},
        {1.0, 1, NoteType::TAP},
        {2.0, 2, NoteType::TAP},
        {3.0, 3, NoteType::TAP},
        {4.0, 4, NoteType::TAP},
    });

    PlayMode mode = ChartClassifier::auto_classify_mode(note_data);
    EXPECT_EQ(mode, PlayMode::SINGLE);
}

TEST(ChartClassifier, AutoClassifyDoubleModeWithHighColumn) {
    // Auto-classify a chart that uses column >= 5
    NoteData note_data(std::vector<NoteEvent>{
        {0.0, 0, NoteType::TAP},
        {1.0, 5, NoteType::TAP},  // Column 5 triggers DOUBLE
    });

    PlayMode mode = ChartClassifier::auto_classify_mode(note_data);
    EXPECT_EQ(mode, PlayMode::DOUBLE);
}

TEST(ChartClassifier, AutoClassifyEmptyChartDefaultsSingle) {
    // Empty chart defaults to SINGLE
    NoteData note_data;

    PlayMode mode = ChartClassifier::auto_classify_mode(note_data);
    EXPECT_EQ(mode, PlayMode::SINGLE);
}

TEST(ChartClassifier, EstimateDifficultyEasyRange) {
    // Low NPS (<2) should map to Easy range (1-5)
    // 120 BPM = 2 beats per second
    // 10 notes over 8 seconds = 1.25 NPS -> Easy
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // Spread 10 notes over 16 beats (8 seconds at 120 BPM)
    for (int i = 0; i < 10; ++i) {
        builder.add_note(i * 1.6, 0, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should be in Easy range (1-5)
    EXPECT_GE(difficulty, 1);
    EXPECT_LE(difficulty, 5);
}

TEST(ChartClassifier, EstimateDifficultyNormalRange) {
    // Medium NPS (2-4) should map to Normal range (6-10)
    // 120 BPM = 2 beats per second
    // 12 notes over 4 seconds = 3 NPS -> Normal
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // Place 12 notes in a 4-second window (8 beats at 120 BPM)
    for (int i = 0; i < 12; ++i) {
        builder.add_note(i * 0.67, 0, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should be in Normal range (6-10)
    EXPECT_GE(difficulty, 6);
    EXPECT_LE(difficulty, 10);
}

TEST(ChartClassifier, EstimateDifficultyHardRange) {
    // Higher NPS (4-6) should map to Hard range (11-15)
    // 120 BPM = 2 beats per second
    // 20 notes over 4 seconds = 5 NPS -> Hard
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // Place 20 notes in a 4-second window (8 beats at 120 BPM)
    for (int i = 0; i < 20; ++i) {
        builder.add_note(i * 0.4, 0, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should be in Hard range (11-15)
    EXPECT_GE(difficulty, 11);
    EXPECT_LE(difficulty, 15);
}

TEST(ChartClassifier, EstimateDifficultyCrazyRange) {
    // High NPS (6-8) should map to Crazy range (16-20)
    // 120 BPM = 2 beats per second
    // 28 notes over 4 seconds = 7 NPS -> Crazy
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // Place 28 notes in a 4-second window (8 beats at 120 BPM)
    for (int i = 0; i < 28; ++i) {
        builder.add_note(i * 0.286, 0, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should be in Crazy range (16-20)
    EXPECT_GE(difficulty, 16);
    EXPECT_LE(difficulty, 20);
}

TEST(ChartClassifier, EstimateDifficultyFreestyleRange) {
    // Very high NPS (8+) should map to Freestyle range (21-28)
    // 120 BPM = 2 beats per second
    // 36 notes over 4 seconds = 9 NPS -> Freestyle
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // Place 36 notes in a 4-second window (8 beats at 120 BPM)
    for (int i = 0; i < 36; ++i) {
        builder.add_note(i * 0.222, 0, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should be in Freestyle range (21-28)
    EXPECT_GE(difficulty, 21);
    EXPECT_LE(difficulty, 28);
}

TEST(ChartClassifier, EstimateDifficultyEmptyChartMinimum) {
    // Empty chart should return minimum difficulty
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    Chart chart = builder.build();

    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());
    EXPECT_EQ(difficulty, 1);
}

TEST(ChartClassifier, EstimateDifficultyWithBpmChanges) {
    // Chart with BPM changes should still estimate correctly
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.add_bpm_change(8.0, 180.0);  // Speed up at beat 8

    // Add notes with varying density
    for (int i = 0; i < 10; ++i) {
        builder.add_note(i * 0.5, 0, NoteType::TAP);
    }
    for (int i = 0; i < 20; ++i) {
        builder.add_note(8.0 + i * 0.25, 1, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should pick up the denser section
    EXPECT_GE(difficulty, 11);  // At least Hard
}

TEST(ChartClassifier, EstimateDifficultyPeakDetection) {
    // Chart with sparse and dense sections should detect the peak
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // Sparse intro (1 NPS)
    for (int i = 0; i < 8; ++i) {
        builder.add_note(i * 2.0, 0, NoteType::TAP);
    }

    // Dense middle section (5 NPS over 4 seconds = 20 notes)
    for (int i = 0; i < 20; ++i) {
        builder.add_note(20.0 + i * 0.4, 1, NoteType::TAP);
    }

    // Sparse outro (1 NPS)
    for (int i = 0; i < 8; ++i) {
        builder.add_note(40.0 + i * 2.0, 2, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should detect the dense middle section (5 NPS = Hard range)
    EXPECT_GE(difficulty, 11);
    EXPECT_LE(difficulty, 15);
}

TEST(ChartClassifier, EstimateDifficultyShortChart) {
    // Chart shorter than the 4-second window
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // 2 seconds worth of notes (4 beats at 120 BPM)
    // 10 notes in 2 seconds = 5 NPS
    for (int i = 0; i < 10; ++i) {
        builder.add_note(i * 0.4, 0, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should calculate overall NPS (5 NPS = Hard range)
    EXPECT_GE(difficulty, 11);
    EXPECT_LE(difficulty, 15);
}

TEST(ChartClassifier, DifficultyRatingCapsAt28) {
    // Extremely high NPS should cap at 28
    ChartBuilder builder;
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);

    // 100 notes over 4 seconds = 25 NPS (absurdly high)
    for (int i = 0; i < 100; ++i) {
        builder.add_note(i * 0.08, 0, NoteType::TAP);
    }

    Chart chart = builder.build();
    int difficulty = ChartClassifier::estimate_difficulty(chart.note_data(), chart.timing_data());

    // Should cap at 28
    EXPECT_LE(difficulty, 28);
    EXPECT_GE(difficulty, 21);  // But still in Freestyle range
}
