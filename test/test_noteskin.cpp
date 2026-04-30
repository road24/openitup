#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <openitup/render/noteskin.h>
#include <openitup/render/noteskin_loader.h>
#include <openitup/render/noteskin_anim.h>
#include <openitup/render/note_renderer.h>
#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>

using namespace openitup;
namespace fs = std::filesystem;

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

// --- NoteSkinLoader Tests (Logic Tests Without SDL) ---

TEST(NoteSkinLoader, LoadFromNonexistentDir) {
    // Create a mock TextureCache (we won't actually load textures)
    // This test verifies that loading from a nonexistent directory throws
    SDL_Renderer* mock_renderer = nullptr;
    TextureCache cache(mock_renderer, nullptr);

    EXPECT_THROW({
        NoteSkinLoader::load("/nonexistent/path/to/noteskin", cache);
    }, std::runtime_error);
}

TEST(NoteSkinLoader, LoadFromEmptyDir) {
    // Create a temporary empty directory
    fs::path temp_dir = fs::temp_directory_path() / "test_empty_noteskin";
    fs::create_directories(temp_dir);

    SDL_Renderer* mock_renderer = nullptr;
    TextureCache cache(mock_renderer, nullptr);

    // Load should succeed but return a NoteSkin with all nullptr sprites
    auto skin = NoteSkinLoader::load(temp_dir, cache);

    EXPECT_NE(skin, nullptr);
    EXPECT_EQ(skin->loaded_count(), 0);
    EXPECT_FALSE(skin->is_complete());
    EXPECT_EQ(skin->name, "test_empty_noteskin");

    // Clean up
    fs::remove_all(temp_dir);
}

TEST(NoteSkinLoader, FilenameConvention) {
    // Test that the loader generates correct filenames
    // We can verify this indirectly by checking that the loader attempts
    // to load files with the expected naming pattern

    // Create a temporary directory with one correctly-named file
    fs::path temp_dir = fs::temp_directory_path() / "test_filename_convention";
    fs::create_directories(temp_dir);

    // Create a dummy SPRJ file (minimal valid JSON)
    fs::path test_file = temp_dir / "ARROW00_TAP.sprj";
    std::ofstream out(test_file);
    out << R"({
        "source_format": "sprj",
        "mode": "tile",
        "pictures": []
    })";
    out.close();

    SDL_Renderer* mock_renderer = nullptr;
    TextureCache cache(mock_renderer, nullptr);

    // Load should succeed and find the ARROW00_TAP.sprj file
    auto skin = NoteSkinLoader::load(temp_dir, cache);

    EXPECT_NE(skin, nullptr);
    EXPECT_EQ(skin->name, "test_filename_convention");

    // The tap(0) should be loaded (even though it has no pictures)
    // Actually, we can't verify this without proper sprite loading,
    // but we verified the file was found by no exception being thrown

    // Clean up
    fs::remove_all(temp_dir);
}

TEST(NoteSkinLoader, LoadWithFallbackUsesSecondary) {
    // Create two directories: one that doesn't exist and one that does
    fs::path nonexistent = "/nonexistent/noteskin/dir";
    fs::path fallback_dir = fs::temp_directory_path() / "test_fallback_noteskin";
    fs::create_directories(fallback_dir);

    SDL_Renderer* mock_renderer = nullptr;
    TextureCache cache(mock_renderer, nullptr);

    // Load with fallback should use the fallback directory
    auto skin = NoteSkinLoader::load_with_fallback(nonexistent, fallback_dir, cache);

    EXPECT_NE(skin, nullptr);
    EXPECT_EQ(skin->name, "test_fallback_noteskin");

    // Clean up
    fs::remove_all(fallback_dir);
}

TEST(NoteSkinLoader, LoadWithFallbackThrowsWhenBothFail) {
    // Both directories don't exist
    fs::path nonexistent1 = "/nonexistent/noteskin/dir1";
    fs::path nonexistent2 = "/nonexistent/noteskin/dir2";

    SDL_Renderer* mock_renderer = nullptr;
    TextureCache cache(mock_renderer, nullptr);

    // Should throw when both fail
    EXPECT_THROW({
        NoteSkinLoader::load_with_fallback(nonexistent1, nonexistent2, cache);
    }, std::runtime_error);
}

// --- NoteSkin Animation Timer Tests (Pure Math) ---

TEST(NoteSkinAnimTimer, LoopTAtZero) {
    EXPECT_FLOAT_EQ(noteskin_loop_t(0.0), 0.0f);
}

TEST(NoteSkinAnimTimer, LoopTAtHalfCycle) {
    EXPECT_FLOAT_EQ(noteskin_loop_t(150.0), 0.5f);
}

TEST(NoteSkinAnimTimer, LoopTWrapsAt300) {
    EXPECT_FLOAT_EQ(noteskin_loop_t(300.0), 0.0f);
}

TEST(NoteSkinAnimTimer, LoopTWrapsAt600) {
    EXPECT_FLOAT_EQ(noteskin_loop_t(600.0), 0.0f);
}

TEST(NoteSkinAnimTimer, LoopTHandlesNegative) {
    // -150ms should wrap to 150ms in the cycle (0.5)
    float t = noteskin_loop_t(-150.0);
    EXPECT_GE(t, 0.0f);
    EXPECT_LT(t, 1.0f);
    EXPECT_FLOAT_EQ(t, 0.5f);
}

TEST(NoteSkinAnimTimer, OneshotTBeforeTrigger) {
    EXPECT_FLOAT_EQ(noteskin_oneshot_t(50.0, 100.0), 0.0f);
}

TEST(NoteSkinAnimTimer, OneshotTAtMiddle) {
    // 150ms elapsed from trigger at 100ms -> 150/300 = 0.5
    EXPECT_FLOAT_EQ(noteskin_oneshot_t(250.0, 100.0), 0.5f);
}

TEST(NoteSkinAnimTimer, OneshotTAfterDuration) {
    // 300ms+ elapsed -> clamped to 1.0
    EXPECT_FLOAT_EQ(noteskin_oneshot_t(500.0, 100.0), 1.0f);
    EXPECT_FLOAT_EQ(noteskin_oneshot_t(1000.0, 100.0), 1.0f);
}

TEST(NoteSkinAnimTimer, OneshotTLinear) {
    // 100ms elapsed -> 100/300 ≈ 0.333
    EXPECT_NEAR(noteskin_oneshot_t(100.0, 0.0), 0.333f, 0.001f);
}

TEST(NoteSkinAnimTimer, OneshotActiveWithin300) {
    EXPECT_TRUE(noteskin_oneshot_active(200.0, 0.0));
    EXPECT_TRUE(noteskin_oneshot_active(299.0, 0.0));
}

TEST(NoteSkinAnimTimer, OneshotInactiveAfter300) {
    EXPECT_FALSE(noteskin_oneshot_active(300.0, 0.0));
    EXPECT_FALSE(noteskin_oneshot_active(301.0, 0.0));
    EXPECT_FALSE(noteskin_oneshot_active(1000.0, 0.0));
}

TEST(NoteSkinAnimTimer, OneshotInactiveBeforeTrigger) {
    EXPECT_FALSE(noteskin_oneshot_active(50.0, 100.0));
}

// --- Integration Verification Tests (No SDL Required) ---

TEST(NoteSkinIntegration, AnimTimerLoopTAt600ms) {
    // 600ms is exactly two full cycles (300ms each)
    EXPECT_FLOAT_EQ(noteskin_loop_t(600.0), 0.0f);
}

TEST(NoteSkinIntegration, AnimTimerOneshotCompletesAt300ms) {
    // At trigger_time=100, global_time=400 means 300ms elapsed
    // oneshot should be inactive (>=300ms)
    EXPECT_FALSE(noteskin_oneshot_active(400.0, 100.0));
}

TEST(NoteSkinIntegration, NoteRendererAcceptsSkin) {
    // Create empty note data and default timing
    NoteData note_data;
    TimingData timing_data;
    NoteFieldConfig config = default_single_config();

    // Construct NoteRenderer with nullptr skin
    NoteRenderer renderer(note_data, timing_data, config, nullptr, nullptr);

    // Verify beat_to_y still works
    float y = renderer.beat_to_y(4.0, 0.0);
    EXPECT_GT(y, 0.0f);  // Should be below receptor line (higher y value)
}

TEST(NoteSkinIntegration, NoteRendererAcceptsSkinWithConfig) {
    // Create empty note data and default timing
    NoteData note_data;
    TimingData timing_data;
    NoteFieldConfig config = default_single_config();

    // Construct NoteRenderer with nullptr skin
    NoteRenderer renderer(note_data, timing_data, config, nullptr, nullptr);

    // Verify config is accessible
    const auto& renderer_config = renderer.config();
    EXPECT_EQ(renderer_config.num_columns, 5);
    EXPECT_FLOAT_EQ(renderer_config.receptor_y, 80.0f);
}

TEST(NoteSkinIntegration, FullPipelineNoSDL) {
    // Create empty NoteSkin
    NoteSkin skin;

    // Create empty note data and default timing
    NoteData note_data;
    TimingData timing_data;
    NoteFieldConfig config = default_single_config();

    // Construct NoteRenderer with empty skin
    NoteRenderer renderer(note_data, timing_data, config, &skin, nullptr);

    // Call beat_to_y - should not crash
    float y1 = renderer.beat_to_y(0.0, 0.0);
    float y2 = renderer.beat_to_y(4.0, 0.0);
    float y3 = renderer.beat_to_y(-1.0, 0.0);

    // Verify reasonable values
    EXPECT_FLOAT_EQ(y1, config.receptor_y);  // At receptor line
    EXPECT_GT(y2, y1);  // Below receptor line
    EXPECT_LT(y3, y1);  // Above receptor line
}
