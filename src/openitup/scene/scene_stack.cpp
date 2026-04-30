#include <openitup/scene/scene_stack.h>

#include <spdlog/spdlog.h>

namespace openitup {

SceneStack::SceneStack() {
    spdlog::debug("SceneStack created");
}

SceneStack::~SceneStack() {
    // Call on_exit() on all remaining scenes in reverse order
    while (!scenes_.empty()) {
        pop();
    }
    spdlog::debug("SceneStack destroyed");
}

void SceneStack::push(std::unique_ptr<Scene> scene) {
    if (!scene) {
        spdlog::warn("SceneStack::push called with nullptr, ignoring");
        return;
    }

    // Pause the current top scene (if any)
    if (!scenes_.empty()) {
        scenes_.back()->on_pause();
    }

    // Add new scene and call its on_enter
    scenes_.push_back(std::move(scene));
    scenes_.back()->on_enter();

    spdlog::debug("Scene pushed, stack size: {}", scenes_.size());
}

void SceneStack::pop() {
    if (scenes_.empty()) {
        return;  // No-op
    }

    // Call on_exit and destroy the top scene
    scenes_.back()->on_exit();
    scenes_.pop_back();

    spdlog::debug("Scene popped, stack size: {}", scenes_.size());

    // Resume the new top scene (if any)
    if (!scenes_.empty()) {
        scenes_.back()->on_resume();
    }
}

void SceneStack::replace(std::unique_ptr<Scene> scene) {
    if (!scene) {
        spdlog::warn("SceneStack::replace called with nullptr, ignoring");
        return;
    }

    if (scenes_.empty()) {
        // Empty stack: replace is equivalent to push
        push(std::move(scene));
        return;
    }

    // Call on_exit and destroy the current top scene
    scenes_.back()->on_exit();
    scenes_.pop_back();

    // Add new scene and call its on_enter
    scenes_.push_back(std::move(scene));
    scenes_.back()->on_enter();

    spdlog::debug("Scene replaced, stack size: {}", scenes_.size());
}

void SceneStack::update(double dt) {
    if (!scenes_.empty()) {
        scenes_.back()->update(dt);
    }
}

void SceneStack::handle_input(const InputSnapshot& input) {
    if (!scenes_.empty()) {
        scenes_.back()->handle_input(input);
    }
}

void SceneStack::render() {
    // Render all scenes bottom-to-top (painter's algorithm)
    for (auto& scene : scenes_) {
        scene->render();
    }
}

} // namespace openitup
