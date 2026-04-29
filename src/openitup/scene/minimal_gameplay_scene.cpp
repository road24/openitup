#include <openitup/scene/minimal_gameplay_scene.h>

#include <spdlog/spdlog.h>

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
        std::string audio_file = chart_.metadata().audio_path;
        if (!audio_file.empty()) {
            auto audio_path = data_dir / std::filesystem::path(audio_file).filename();
            if (!audio_->load_music(audio_path)) {
                spdlog::warn("Failed to load audio '{}', proceeding without",
                             audio_path.string());
                audio_ = nullptr;
            }
        } else {
            spdlog::warn("Chart has no audio file reference");
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
    // Implemented in Step 2
}

void MinimalGameplayScene::render(double /*alpha*/) {
    // Implemented in Step 3
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
