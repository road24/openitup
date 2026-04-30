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
    // Implemented in Step 2
}

void SceneStack::pop() {
    // Implemented in Step 2
}

void SceneStack::replace(std::unique_ptr<Scene> scene) {
    // Implemented in Step 2
}

void SceneStack::update(double dt) {
    // Implemented in Step 2
}

void SceneStack::handle_input(const InputSnapshot& input) {
    // Implemented in Step 2
}

void SceneStack::render() {
    // Implemented in Step 2
}

} // namespace openitup
