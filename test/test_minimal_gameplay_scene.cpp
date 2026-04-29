#include <gtest/gtest.h>

#include <openitup/scene/minimal_gameplay_scene.h>
#include <openitup/audio/audio_system.h>
#include <openitup/input/input_driver.h>
#include <openitup/input/input_system.h>
#include <openitup/chart/chart_builder.h>
#include <openitup/chart/note_type.h>
#include <openitup/chart/play_mode.h>
#include <openitup/judge/judgment_tier.h>

#include <memory>

using namespace openitup;

// --- Mock Audio System ---

class MockAudioSystem : public AudioSystem {
public:
    bool init() override { return true; }
    void shutdown() override {}

    bool load_music(const std::filesystem::path&) override {
        music_loaded_ = true;
        return true;
    }

    void play() override {
        state_ = AudioState::PLAYING;
    }

    void pause() override {
        if (state_ == AudioState::PLAYING) {
            state_ = AudioState::PAUSED;
        }
    }

    void resume() override {
        if (state_ == AudioState::PAUSED) {
            state_ = AudioState::PLAYING;
        }
    }

    void stop() override {
        state_ = AudioState::STOPPED;
        position_ms_ = 0.0;
    }

    void seek(double position_ms) override {
        position_ms_ = position_ms;
    }

    double get_position_ms() const override {
        return position_ms_;
    }

    double get_duration_ms() const override {
        return 10000.0;  // 10 seconds
    }

    AudioState get_state() const override {
        return state_;
    }

    bool is_music_loaded() const override {
        return music_loaded_;
    }

    void set_music_volume(float) override {}
    float get_music_volume() const override { return 1.0f; }

    uint32_t load_sfx(const std::filesystem::path&) override { return 0; }
    void play_sfx(uint32_t) override {}
    void set_sfx_volume(float) override {}
    float get_sfx_volume() const override { return 1.0f; }

    // Test control methods
    void set_position(double ms) {
        position_ms_ = ms;
    }

    void set_state(AudioState state) {
        state_ = state;
    }

private:
    AudioState state_ = AudioState::STOPPED;
    double position_ms_ = 0.0;
    bool music_loaded_ = false;
};

// --- Mock Input Driver ---

class MockDriver : public InputDriver {
public:
    uint32_t poll_held() override {
        return held_mask;
    }

    std::string device_name() const override {
        return "Mock";
    }

    // Test control: publicly writable
    uint32_t held_mask = 0;
};

// --- Test Helpers ---

Chart make_test_chart() {
    ChartBuilder builder;
    builder.set_title("Test");
    builder.set_mode(PlayMode::SINGLE);
    builder.add_bpm_change(0.0, 120.0);
    builder.set_display_bpm(120.0);

    // 4 quarter notes at beats 1, 2, 3, 4 on columns 0-3
    // At 120 BPM: beat 1.0 = 500ms, beat 2.0 = 1000ms, beat 3.0 = 1500ms, beat 4.0 = 2000ms
    builder.add_note(1.0, 0, NoteType::TAP);
    builder.add_note(2.0, 1, NoteType::TAP);
    builder.add_note(3.0, 2, NoteType::TAP);
    builder.add_note(4.0, 3, NoteType::TAP);

    return builder.build();
}

// --- Tests ---

TEST(MinimalGameplayScene, ConstructWithChart) {
    Chart chart = make_test_chart();
    MinimalGameplayScene scene(std::move(chart), nullptr, nullptr, nullptr);

    // Verify judge was initialized with correct note count
    EXPECT_EQ(scene.judge().total_judgable(), 4u);
}

TEST(MinimalGameplayScene, NullAudioNoExceptions) {
    Chart chart = make_test_chart();
    MinimalGameplayScene scene(std::move(chart), nullptr, nullptr, nullptr);

    // Update 60 times - should not crash
    for (int i = 0; i < 60; i++) {
        ASSERT_NO_THROW(scene.update(1.0 / 60.0));
    }
}

TEST(MinimalGameplayScene, NullInputNoExceptions) {
    Chart chart = make_test_chart();
    auto audio = std::make_unique<MockAudioSystem>();

    MinimalGameplayScene scene(std::move(chart), audio.get(), nullptr, nullptr);

    // Update should not crash with null input
    ASSERT_NO_THROW(scene.update(1.0 / 60.0));
}

TEST(MinimalGameplayScene, SongCompletionWhenAudioStops) {
    Chart chart = make_test_chart();
    auto audio = std::make_unique<MockAudioSystem>();
    MockAudioSystem* audio_ptr = audio.get();

    MinimalGameplayScene scene(std::move(chart), audio.get(), nullptr, nullptr);

    // First update starts audio (scene.update calls audio->play())
    scene.update(1.0 / 60.0);
    EXPECT_EQ(audio_ptr->get_state(), AudioState::PLAYING);

    // Update a few more times while playing
    scene.update(1.0 / 60.0);
    scene.update(1.0 / 60.0);

    // Scene should not be complete yet
    EXPECT_FALSE(scene.is_complete());

    // Now stop the audio
    audio_ptr->set_state(AudioState::STOPPED);
    scene.update(1.0 / 60.0);

    // Scene should now be complete
    EXPECT_TRUE(scene.is_complete());
}

TEST(MinimalGameplayScene, GameplayStateAccumulates) {
    Chart chart = make_test_chart();
    auto audio = std::make_unique<MockAudioSystem>();
    MockAudioSystem* audio_ptr = audio.get();

    auto driver = std::make_unique<MockDriver>();
    MockDriver* driver_ptr = driver.get();
    auto input = std::make_unique<InputSystem>(std::move(driver));

    MinimalGameplayScene scene(std::move(chart), audio.get(), input.get(), nullptr);

    // Start audio (first update calls play, which sets state to PLAYING)
    scene.update(1.0 / 60.0);
    EXPECT_EQ(audio_ptr->get_state(), AudioState::PLAYING);

    // Hit note 1 at beat 1.0 (500ms) on column 0
    // Set position and press column 0
    audio_ptr->set_position(500.0);
    driver_ptr->held_mask = 0;  // Release all first
    input->poll(1);
    driver_ptr->held_mask = 1;  // Column 0
    input->poll(2);
    scene.update(1.0 / 60.0);

    // Hit note 2 at beat 2.0 (1000ms) on column 1
    audio_ptr->set_position(1000.0);
    driver_ptr->held_mask = 0;  // Release
    input->poll(3);
    driver_ptr->held_mask = 2;  // Column 1
    input->poll(4);
    scene.update(1.0 / 60.0);

    // Hit note 3 at beat 3.0 (1500ms) on column 2
    audio_ptr->set_position(1500.0);
    driver_ptr->held_mask = 0;  // Release
    input->poll(5);
    driver_ptr->held_mask = 4;  // Column 2
    input->poll(6);
    scene.update(1.0 / 60.0);

    // Verify score and combo
    const auto& state = scene.gameplay_state();
    EXPECT_EQ(state.score(), 3000);  // 3 PERFECTs * 1000
    EXPECT_EQ(state.current_combo(), 3);
    EXPECT_EQ(state.judgment_count(JudgmentTier::PERFECT), 3);
}

TEST(MinimalGameplayScene, FlushOnComplete) {
    Chart chart = make_test_chart();
    auto audio = std::make_unique<MockAudioSystem>();
    MockAudioSystem* audio_ptr = audio.get();

    auto driver = std::make_unique<MockDriver>();
    MockDriver* driver_ptr = driver.get();
    auto input = std::make_unique<InputSystem>(std::move(driver));

    MinimalGameplayScene scene(std::move(chart), audio.get(), input.get(), nullptr);

    // Start audio
    scene.update(1.0 / 60.0);
    EXPECT_EQ(audio_ptr->get_state(), AudioState::PLAYING);

    // Hit only 2 notes
    audio_ptr->set_position(500.0);
    driver_ptr->held_mask = 0;  // Release
    input->poll(1);
    driver_ptr->held_mask = 1;  // Column 0
    input->poll(2);
    scene.update(1.0 / 60.0);

    audio_ptr->set_position(1000.0);
    driver_ptr->held_mask = 0;  // Release
    input->poll(3);
    driver_ptr->held_mask = 2;  // Column 1
    input->poll(4);
    scene.update(1.0 / 60.0);

    // Stop audio (triggering completion)
    audio_ptr->set_state(AudioState::STOPPED);
    scene.update(1.0 / 60.0);

    // All 4 notes should now be judged (2 hit + 2 auto-miss)
    EXPECT_EQ(scene.judge().judged_count(), 4u);
    EXPECT_TRUE(scene.is_complete());

    // Verify 2 misses were added
    const auto& state = scene.gameplay_state();
    EXPECT_EQ(state.judgment_count(JudgmentTier::MISS), 2);
}
