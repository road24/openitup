#include <openitup/scene/transition_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

TransitionScene::TransitionScene(Renderer* renderer,
                                 TextRenderer* text_renderer,
                                 SceneStack* scene_stack,
                                 std::unique_ptr<Scene> target_scene,
                                 Type transition_type,
                                 double duration)
    : renderer_(renderer),
      text_(text_renderer),
      stack_(scene_stack),
      target_scene_(std::move(target_scene)),
      type_(transition_type),
      duration_(duration) {}

void TransitionScene::on_enter() {
    spdlog::info("TransitionScene entered (duration: {}s)", duration_);
    elapsed_ = 0.0;
}

void TransitionScene::on_exit() {
    spdlog::info("TransitionScene exited");
}

void TransitionScene::on_pause() {}
void TransitionScene::on_resume() {}

void TransitionScene::update(double dt) {
    elapsed_ += dt;
    if (elapsed_ >= duration_) {
        spdlog::info("TransitionScene: transition complete, replacing with target scene");
        stack_->replace(std::move(target_scene_));
    }
}

void TransitionScene::handle_input(const InputSnapshot& /*input*/) {
    // Transitions ignore input (non-skippable in this implementation)
    // Future: add skip_on_input flag
}

void TransitionScene::render() {
    if (!renderer_ || !text_) return;

    // Calculate fade factor (0.0 = fully visible, 1.0 = fully faded)
    double progress = elapsed_ / duration_;
    if (progress > 1.0) progress = 1.0;

    switch (type_) {
        case Type::FADE:
            // Simple fade to black and back
            // In a real implementation, this would:
            // 1. First half (0.0-0.5): render outgoing scene with alpha going from 1.0 to 0.0
            // 2. Second half (0.5-1.0): render incoming scene with alpha going from 0.0 to 1.0
            // For Phase 5, just display a fade indicator
            {
                int alpha = static_cast<int>(progress * 255);
                if (progress > 0.5) {
                    alpha = static_cast<int>((1.0 - progress) * 255);
                }
                SDL_Color fade_color = {0, 0, 0, static_cast<Uint8>(alpha)};
                text_->draw_text("Transitioning...", 240, 240, fade_color);
            }
            break;
    }
}

} // namespace openitup
