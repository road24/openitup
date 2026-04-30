#include <gtest/gtest.h>
#include <openitup/render/noteskin.h>

using namespace openitup;

// --- NoteSkin Accessor Tests (Pure Logic, No SDL) ---

TEST(NoteSkin, DefaultConstructedReturnsNull) {
    NoteSkin skin;
    EXPECT_EQ(skin.tap(0), nullptr);
    EXPECT_EQ(skin.faketap(0), nullptr);
    EXPECT_EQ(skin.hold(0, HoldPart::HEAD), nullptr);
    EXPECT_EQ(skin.hold(0, HoldPart::BODY), nullptr);
    EXPECT_EQ(skin.hold(0, HoldPart::TAIL), nullptr);
    EXPECT_EQ(skin.other_w(0), nullptr);
    EXPECT_EQ(skin.other_g(0), nullptr);
    EXPECT_EQ(skin.press(0), nullptr);
    EXPECT_EQ(skin.judge(0), nullptr);
    EXPECT_EQ(skin.receptor(PlayMode::SINGLE), nullptr);
    EXPECT_EQ(skin.receptor(PlayMode::DOUBLE), nullptr);
    EXPECT_EQ(skin.receptor(PlayMode::HALF), nullptr);
}

TEST(NoteSkin, TapOutOfBoundsReturnsNull) {
    NoteSkin skin;
    EXPECT_EQ(skin.tap(-1), nullptr);
    EXPECT_EQ(skin.tap(5), nullptr);
    EXPECT_EQ(skin.tap(100), nullptr);
}

TEST(NoteSkin, FaketapOutOfBoundsReturnsNull) {
    NoteSkin skin;
    EXPECT_EQ(skin.faketap(-1), nullptr);
    EXPECT_EQ(skin.faketap(5), nullptr);
}

TEST(NoteSkin, HoldPartsAreIndependent) {
    NoteSkin skin;
    // All hold parts for track 0 should be independent (all null by default)
    EXPECT_EQ(skin.hold(0, HoldPart::HEAD), nullptr);
    EXPECT_EQ(skin.hold(0, HoldPart::BODY), nullptr);
    EXPECT_EQ(skin.hold(0, HoldPart::TAIL), nullptr);

    // Out of bounds for hold
    EXPECT_EQ(skin.hold(-1, HoldPart::HEAD), nullptr);
    EXPECT_EQ(skin.hold(5, HoldPart::HEAD), nullptr);
}

TEST(NoteSkin, OtherWOutOfBoundsReturnsNull) {
    NoteSkin skin;
    EXPECT_EQ(skin.other_w(-1), nullptr);
    EXPECT_EQ(skin.other_w(5), nullptr);
}

TEST(NoteSkin, OtherGOutOfBoundsReturnsNull) {
    NoteSkin skin;
    EXPECT_EQ(skin.other_g(-1), nullptr);
    EXPECT_EQ(skin.other_g(5), nullptr);
}

TEST(NoteSkin, PressOutOfBoundsReturnsNull) {
    NoteSkin skin;
    EXPECT_EQ(skin.press(-1), nullptr);
    EXPECT_EQ(skin.press(5), nullptr);
}

TEST(NoteSkin, JudgeOutOfBoundsReturnsNull) {
    NoteSkin skin;
    EXPECT_EQ(skin.judge(-1), nullptr);
    EXPECT_EQ(skin.judge(5), nullptr);
}

TEST(NoteSkin, ReceptorInvalidModeReturnsNull) {
    NoteSkin skin;
    // Test casting beyond valid enum range
    EXPECT_EQ(skin.receptor(static_cast<PlayMode>(3)), nullptr);
    EXPECT_EQ(skin.receptor(static_cast<PlayMode>(-1)), nullptr);
}

TEST(NoteSkin, LoadedCountInitiallyZero) {
    NoteSkin skin;
    EXPECT_EQ(skin.loaded_count(), 0);
}

TEST(NoteSkin, IsCompleteInitiallyFalse) {
    NoteSkin skin;
    EXPECT_FALSE(skin.is_complete());
}

TEST(NoteSkin, ExpectedCountIs48) {
    EXPECT_EQ(NoteSkin::EXPECTED_COUNT, 48);
}

TEST(NoteSkin, TapTrackRange) {
    NoteSkin skin;
    // Verify all valid track indices 0-4 don't crash
    for (int i = 0; i < NUM_TRACKS; ++i) {
        EXPECT_EQ(skin.tap(i), nullptr);
    }
}

TEST(NoteSkin, FaketapTrackRange) {
    NoteSkin skin;
    for (int i = 0; i < NUM_TRACKS; ++i) {
        EXPECT_EQ(skin.faketap(i), nullptr);
    }
}

TEST(NoteSkin, HoldTrackRange) {
    NoteSkin skin;
    for (int i = 0; i < NUM_TRACKS; ++i) {
        EXPECT_EQ(skin.hold(i, HoldPart::HEAD), nullptr);
        EXPECT_EQ(skin.hold(i, HoldPart::BODY), nullptr);
        EXPECT_EQ(skin.hold(i, HoldPart::TAIL), nullptr);
    }
}

TEST(NoteSkin, OtherWTrackRange) {
    NoteSkin skin;
    for (int i = 0; i < NUM_TRACKS; ++i) {
        EXPECT_EQ(skin.other_w(i), nullptr);
    }
}

TEST(NoteSkin, OtherGTrackRange) {
    NoteSkin skin;
    for (int i = 0; i < NUM_TRACKS; ++i) {
        EXPECT_EQ(skin.other_g(i), nullptr);
    }
}

TEST(NoteSkin, PressTrackRange) {
    NoteSkin skin;
    for (int i = 0; i < NUM_TRACKS; ++i) {
        EXPECT_EQ(skin.press(i), nullptr);
    }
}

TEST(NoteSkin, JudgeTrackRange) {
    NoteSkin skin;
    for (int i = 0; i < NUM_TRACKS; ++i) {
        EXPECT_EQ(skin.judge(i), nullptr);
    }
}

TEST(NoteSkin, NameAccessor) {
    NoteSkin skin;
    EXPECT_TRUE(skin.name.empty());
}

TEST(NoteSkin, DirectoryAccessor) {
    NoteSkin skin;
    EXPECT_TRUE(skin.directory.empty());
}
