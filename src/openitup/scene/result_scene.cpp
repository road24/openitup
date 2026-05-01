#include <openitup/scene/result_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/judge/judgment_tier.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/scene/title_scene.h>

namespace openitup {

ResultScene::ResultScene(Renderer* renderer,
                         TextRenderer* text_renderer,
                         SceneStack* scene_stack,
                         Engine* engine,
                         const GameplayState& gameplay_state)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(scene_stack),
      engine_(engine),
      gameplay_state_(gameplay_state)
{
}

void ResultScene::on_enter() {
    spdlog::info("ResultScene entered - Score: {}, Grade: {}",
                 gameplay_state_.score(),
                 calculate_grade());
    elapsed_ = 0.0;
}

void ResultScene::on_exit() {
    spdlog::info("ResultScene exited");
}

void ResultScene::on_pause() {}
void ResultScene::on_resume() {}

void ResultScene::update(double dt) {
    elapsed_ += dt;
    if (elapsed_ >= AUTO_TRANSITION_TIME) {
        spdlog::info("ResultScene: auto-transition timeout, returning to TitleScene");
        // For Phase 3, we don't have the test_chart_path in ResultScene,
        // so we'll transition to TitleScene with an empty path.
        // TitleScene can handle this gracefully.
        stack_->replace(std::make_unique<TitleScene>(
            renderer_, text_, stack_, engine_, std::filesystem::path()));
    }
}

void ResultScene::handle_input(const InputSnapshot& input) {
    // Any input transitions to TitleScene
    if (input.pressed_mask() != 0) {
        spdlog::info("ResultScene: input detected, transitioning to TitleScene");
        stack_->replace(std::make_unique<TitleScene>(
            renderer_, text_, stack_, engine_, std::filesystem::path()));
    }
}

void ResultScene::render() {
    if (!renderer_ || !text_) return;

    // Display grade at the top
    std::string grade = calculate_grade();
    text_->draw_text("Grade: " + grade, 260, 80, SDL_Color{255, 255, 0, 255});

    // Display score and max combo
    text_->draw_text("Score: " + std::to_string(gameplay_state_.score()),
                     220, 140, SDL_Color{255, 255, 255, 255});
    text_->draw_text("Max Combo: " + std::to_string(gameplay_state_.max_combo()),
                     220, 180, SDL_Color{255, 255, 255, 255});

    // Display judgment breakdown
    text_->draw_text("Judgment Breakdown:", 220, 240, SDL_Color{200, 200, 200, 255});

    int perfect = gameplay_state_.judgment_count(JudgmentTier::PERFECT);
    int great = gameplay_state_.judgment_count(JudgmentTier::GREAT);
    int good = gameplay_state_.judgment_count(JudgmentTier::GOOD);
    int bad = gameplay_state_.judgment_count(JudgmentTier::BAD);
    int miss = gameplay_state_.judgment_count(JudgmentTier::MISS);

    text_->draw_text("Perfect: " + std::to_string(perfect),
                     240, 280, SDL_Color{255, 255, 0, 255});
    text_->draw_text("Great: " + std::to_string(great),
                     240, 310, SDL_Color{0, 255, 0, 255});
    text_->draw_text("Good: " + std::to_string(good),
                     240, 340, SDL_Color{0, 150, 255, 255});
    text_->draw_text("Bad: " + std::to_string(bad),
                     240, 370, SDL_Color{255, 150, 0, 255});
    text_->draw_text("Miss: " + std::to_string(miss),
                     240, 400, SDL_Color{255, 0, 0, 255});

    // Display prompt
    text_->draw_text("Press any button to continue", 180, 440, SDL_Color{200, 200, 200, 255});
}

std::string ResultScene::calculate_grade() const {
    // SS grade: all Perfect (100% Perfect rate)
    int total = gameplay_state_.total_judged();
    if (total == 0) return "F";

    int perfect = gameplay_state_.judgment_count(JudgmentTier::PERFECT);
    if (perfect == total) {
        return "SS";
    }

    // Other grades based on score percentage
    double percentage = gameplay_state_.score_percentage();

    if (percentage >= 95.0) return "S";
    if (percentage >= 90.0) return "A";
    if (percentage >= 80.0) return "B";
    if (percentage >= 70.0) return "C";
    if (percentage >= 60.0) return "D";
    return "F";
}

} // namespace openitup
