#include <openitup/audio/gameplay_sfx.h>

#include <algorithm>

#include <spdlog/spdlog.h>

#include <openitup/audio/sfx_player.h>
#include <openitup/judge/judgment_tier.h>

namespace openitup {

GameplaySfx::GameplaySfx(SfxPlayer* sfx_player, const std::filesystem::path& system_dir)
    : sfx_player_(sfx_player), system_dir_(system_dir) {
}

GameplaySfx::~GameplaySfx() = default;

bool GameplaySfx::load_sounds() {
    bool any_loaded = false;
    std::filesystem::path sfx_dir = system_dir_ / "sfx";

    // --- Load key sounds ---

    // Try to load per-column key sounds (col0.wav through col4.wav)
    bool all_columns_loaded = true;
    for (int col = 0; col < NUM_COLUMNS; ++col) {
        std::string filename = "col" + std::to_string(col) + ".wav";
        std::filesystem::path path = sfx_dir / filename;

        if (std::filesystem::exists(path)) {
            key_sounds_[col] = SoundSample::load(path);
            if (key_sounds_[col]) {
                spdlog::debug("Loaded key sound for column {}: {}", col, path.string());
                any_loaded = true;
            } else {
                spdlog::warn("Failed to load key sound for column {}: {}", col, path.string());
                all_columns_loaded = false;
            }
        } else {
            all_columns_loaded = false;
        }
    }

    has_per_column_sounds_ = all_columns_loaded;

    // If per-column sounds are incomplete, load fallback key sound
    if (!has_per_column_sounds_) {
        std::filesystem::path fallback_path = sfx_dir / "key.wav";
        if (std::filesystem::exists(fallback_path)) {
            fallback_key_sound_ = SoundSample::load(fallback_path);
            if (fallback_key_sound_) {
                spdlog::info("Loaded fallback key sound: {}", fallback_path.string());
                any_loaded = true;
            } else {
                spdlog::warn("Failed to load fallback key sound: {}", fallback_path.string());
            }
        } else {
            spdlog::warn("No per-column key sounds and no fallback key.wav found");
        }
    }

    // --- Load judgment sounds ---

    std::filesystem::path judgment_dir = sfx_dir / "judgment";
    const char* judgment_filenames[] = {
        "Perfect.wav",  // JudgmentTier::PERFECT = 0
        "Great.wav",    // JudgmentTier::GREAT = 1
        "Good.wav",     // JudgmentTier::GOOD = 2
        "Bad.wav",      // JudgmentTier::BAD = 3
        "Miss.wav"      // JudgmentTier::MISS = 4
    };

    for (int i = 0; i < 5; ++i) {
        std::filesystem::path path = judgment_dir / judgment_filenames[i];
        if (std::filesystem::exists(path)) {
            judgment_sounds_[i] = SoundSample::load(path);
            if (judgment_sounds_[i]) {
                spdlog::debug("Loaded judgment sound: {}", path.string());
                any_loaded = true;
            } else {
                spdlog::warn("Failed to load judgment sound: {}", path.string());
            }
        } else {
            spdlog::debug("Judgment sound not found (graceful): {}", path.string());
        }
    }

    if (!any_loaded) {
        spdlog::warn("No gameplay SFX loaded from {}", sfx_dir.string());
        return false;
    }

    spdlog::info("GameplaySfx initialized: per-column={}, fallback={}, judgment={} sounds",
                 has_per_column_sounds_ ? "yes" : "no",
                 fallback_key_sound_ != nullptr ? "yes" : "no",
                 std::count_if(judgment_sounds_.begin(), judgment_sounds_.end(),
                              [](const auto& s) { return s != nullptr; }));

    return true;
}

void GameplaySfx::on_panel_press(int column) {
    if (!sfx_player_) {
        return;
    }

    if (column < 0 || column >= NUM_COLUMNS) {
        spdlog::warn("Invalid column index for key sound: {}", column);
        return;
    }

    // Select sound: per-column if available, otherwise fallback
    const SoundSample* sample = nullptr;
    if (has_per_column_sounds_ && key_sounds_[column]) {
        sample = key_sounds_[column].get();
    } else if (fallback_key_sound_) {
        sample = fallback_key_sound_.get();
    }

    if (sample) {
        sfx_player_->play(*sample, key_sound_volume_);
    }
}

void GameplaySfx::on_judgment(JudgmentTier tier) {
    if (!sfx_player_) {
        return;
    }

    int tier_index = static_cast<int>(tier);
    if (tier_index < 0 || tier_index >= 5) {
        spdlog::warn("Invalid judgment tier index: {}", tier_index);
        return;
    }

    const SoundSample* sample = judgment_sounds_[tier_index].get();
    if (sample) {
        sfx_player_->play(*sample, judgment_volume_);
    }
}

void GameplaySfx::set_key_sound_volume(float volume) {
    key_sound_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void GameplaySfx::set_judgment_volume(float volume) {
    judgment_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

}  // namespace openitup
