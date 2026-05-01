#pragma once

#include <array>
#include <filesystem>
#include <memory>

#include <openitup/audio/sound_sample.h>

namespace openitup {

class SfxPlayer;
enum class JudgmentTier : uint8_t;

// Number of playable columns (5 for Pump It Up)
constexpr int NUM_COLUMNS = 5;

// GameplaySfx manages gameplay-related sound effects:
// - Key sounds (panel press feedback)
// - Judgment sounds (Perfect, Great, etc.)
// Provides independent volume control for key sounds and judgment sounds.
class GameplaySfx {
public:
    GameplaySfx(SfxPlayer* sfx_player, const std::filesystem::path& system_dir);
    ~GameplaySfx();

    GameplaySfx(const GameplaySfx&) = delete;
    GameplaySfx& operator=(const GameplaySfx&) = delete;

    // Load key sounds and judgment sounds from system directory.
    // Key sounds: attempts to load col0.wav through col4.wav from system_dir/sfx/
    //   Falls back to key.wav if per-column sounds not found.
    // Judgment sounds: loads Perfect.wav, Great.wav, Good.wav, Bad.wav, Miss.wav
    //   from system_dir/sfx/judgment/
    // Returns true if at least one sound loaded successfully.
    // Missing sounds are handled gracefully (no playback, no error).
    bool load_sounds();

    // Trigger a key sound for the given column (0-4).
    // Volume is affected by key_sound_volume_.
    void on_panel_press(int column);

    // Trigger a judgment sound for the given tier.
    // Volume is affected by judgment_volume_.
    void on_judgment(JudgmentTier tier);

    // Set volume for key sounds. Range: 0.0 to 1.0. Clamped.
    void set_key_sound_volume(float volume);

    // Get current key sound volume.
    float get_key_sound_volume() const { return key_sound_volume_; }

    // Set volume for judgment sounds. Range: 0.0 to 1.0. Clamped.
    void set_judgment_volume(float volume);

    // Get current judgment sound volume.
    float get_judgment_volume() const { return judgment_volume_; }

private:
    SfxPlayer* sfx_player_;  // Borrowed reference
    std::filesystem::path system_dir_;

    // Key sounds (per-column or single fallback)
    std::array<std::unique_ptr<SoundSample>, NUM_COLUMNS> key_sounds_;
    std::unique_ptr<SoundSample> fallback_key_sound_;  // Used if per-column sounds missing
    bool has_per_column_sounds_ = false;

    // Judgment sounds (indexed by JudgmentTier enum value)
    std::array<std::unique_ptr<SoundSample>, 5> judgment_sounds_;

    // Volume controls
    float key_sound_volume_ = 1.0f;
    float judgment_volume_ = 1.0f;
};

}  // namespace openitup
