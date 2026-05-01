#include <openitup/scene/boot_scene.h>

#include <spdlog/spdlog.h>

#include <openitup/gfx/renderer.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/scene_stack.h>

namespace openitup {

BootScene::BootScene(Renderer* renderer, TextRenderer* text_renderer, SceneStack* scene_stack)
    : renderer_(renderer), text_(text_renderer), stack_(scene_stack) {}

void BootScene::on_enter() {
    spdlog::info("BootScene entered");
    elapsed_ = 0.0;
}

void BootScene::on_exit() {
    spdlog::info("BootScene exited");
}

void BootScene::on_pause() {
    // BootScene is never paused (it's always the bottom scene)
}

void BootScene::on_resume() {
    // BootScene is never resumed
}

void BootScene::update(double dt) {
    elapsed_ += dt;
    if (elapsed_ >= DURATION) {
        spdlog::info("BootScene: transitioning to TitleScene (stub — TitleScene not yet implemented)");
        // Transition will be wired in US-SCN-004
        // stack_->replace(std::make_unique<TitleScene>(...));
    }
}

void BootScene::handle_input(const InputSnapshot& /*input*/) {
    // BootScene ignores input
}

void BootScene::render() {
    if (!renderer_ || !text_) return;

    // Text is drawn by Engine after all scenes render
    // For now, just log on first render
    static bool logged = false;
    if (!logged) {
        spdlog::debug("BootScene rendering");
        logged = true;
    }

    // Draw "openitup" centered at (320, 240)
    text_->draw_text("openitup", 320, 240, SDL_Color{255, 255, 255, 255});
}

} // namespace openitup
