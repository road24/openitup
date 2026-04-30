#include <openitup/scene/minimal_gameplay_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/input/input_snapshot.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/timing_profile.h>
#include <openitup/render/note_renderer.h>

namespace openitup {

MinimalGameplayScene::MinimalGameplayScene(
    const std::filesystem::path& chart_path,
    const std::filesystem::path& data_dir,
    AudioSystem* audio_system,
    InputSystem* input_system,
    Renderer* renderer)
    : chart_(KsfParser().parse(chart_path)),
      judge_(chart_.note_data(), chart_.timing_data(), default_timing_profile()),
      gameplay_state_(static_cast<int>(judge_.total_judgable())),
      note_renderer_(chart_.note_data(), chart_.timing_data(), default_single_config()),
      judgment_display_(),
      audio_(audio_system),
      input_(input_system),
      renderer_(renderer)
{
    spdlog::info("Loaded chart '{}': {} notes, BPM {:.0f}",
                 chart_.metadata().title,
                 chart_.note_count(),
                 chart_.metadata().display_bpm);

    // Load audio
    if (audio_) {
        std::filesystem::path audio_path;

        std::string audio_file = chart_.metadata().audio_path;
        if (!audio_file.empty()) {
            audio_path = data_dir / std::filesystem::path(audio_file).filename();
        }

        // Fallback: probe for common filenames when chart has no audio reference
        if (audio_path.empty() || !std::filesystem::exists(audio_path)) {
            static const char* probes[] = {
                "song.ogg", "song.mp3", "Song.ogg", "Song.mp3",
                "SONG.OGG", "SONG.MP3",
            };
            for (const char* name : probes) {
                auto candidate = data_dir / name;
                if (std::filesystem::exists(candidate)) {
                    audio_path = candidate;
                    spdlog::info("Audio discovered by convention: {}", audio_path.string());
                    break;
                }
            }
        }

        if (!audio_path.empty() && std::filesystem::exists(audio_path)) {
            if (!audio_->load_music(audio_path)) {
                spdlog::warn("Failed to load audio '{}', proceeding without",
                             audio_path.string());
                audio_ = nullptr;
            }
        } else {
            spdlog::warn("No audio file found in '{}'", data_dir.string());
            audio_ = nullptr;
        }
    }
}

MinimalGameplayScene::MinimalGameplayScene(
    Chart chart,
    AudioSystem* audio_system,
    InputSystem* input_system,
    Renderer* renderer)
    : chart_(std::move(chart)),
      judge_(chart_.note_data(), chart_.timing_data(), default_timing_profile()),
      gameplay_state_(static_cast<int>(judge_.total_judgable())),
      note_renderer_(chart_.note_data(), chart_.timing_data(), default_single_config()),
      judgment_display_(),
      audio_(audio_system),
      input_(input_system),
      renderer_(renderer)
{
    // No logging or audio loading for test-friendly constructor
}

MinimalGameplayScene::~MinimalGameplayScene() = default;

void MinimalGameplayScene::update(double /*dt*/) {
    // Start audio on first tick
    if (!audio_started_ && audio_) {
        audio_->play();
        audio_started_ = true;
        spdlog::info("Audio playback started");
    }

    // Get song position from audio (authoritative time source)
    double song_ms = 0.0;
    if (audio_ && audio_->get_state() == AudioState::PLAYING) {
        song_ms = audio_->get_position_ms();
    }
    last_song_ms_ = song_ms;

    // Get input (InputSystem was already polled by Engine before this call)
    uint32_t pressed = 0;
    if (input_) {
        pressed = input_->snapshot().pressed_mask() & 0x03FF;
    }

    // Run judge
    auto events = judge_.update(song_ms, pressed);

    // Feed events to displays and state
    for (const auto& event : events) {
        judgment_display_.on_judgment(event.tier());

        if (spdlog::should_log(spdlog::level::debug)) {
            spdlog::debug("Judgment: {} col={} error={:.1f}ms {}",
                         judgment_tier_to_string(event.tier()),
                         event.column(),
                         event.timing_error_ms(),
                         event.is_auto_miss() ? "(auto-miss)" : "");
        }
    }
    gameplay_state_.apply(events);

    // Check song completion
    if (audio_started_ && audio_ && audio_->get_state() == AudioState::STOPPED) {
        if (!complete_) {
            auto remaining = judge_.flush_remaining();
            gameplay_state_.apply(remaining);
            complete_ = true;
            spdlog::info("Song complete. Score: {}, Max combo: {}, Perfects: {}",
                         gameplay_state_.score(),
                         gameplay_state_.max_combo(),
                         gameplay_state_.judgment_count(JudgmentTier::PERFECT));
        }
    }

    // Also complete if all notes judged (edge case: chart with no audio)
    if (!complete_ && judge_.is_complete()) {
        complete_ = true;
    }
}

void MinimalGameplayScene::render(double /*alpha*/) {
    if (!renderer_) return;

    SDL_Renderer* sdl_renderer = renderer_->get();

    // Render note field
    note_renderer_.render_receptors(sdl_renderer);
    note_renderer_.render(sdl_renderer, last_song_ms_);

    // Render judgment feedback
    // Use FIXED_STEP as dt approximation for the fade timer
    judgment_display_.render(sdl_renderer, 1.0 / 60.0);
}

bool MinimalGameplayScene::is_complete() const {
    return complete_;
}

const GameplayState& MinimalGameplayScene::gameplay_state() const {
    return gameplay_state_;
}

const Judge& MinimalGameplayScene::judge() const {
    return judge_;
}

} // namespace openitup
