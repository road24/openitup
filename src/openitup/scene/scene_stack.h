#pragma once

#include <memory>
#include <vector>

#include <openitup/input/input_snapshot.h>
#include <openitup/scene/scene.h>

namespace openitup {

// Manages a stack of scenes with push/pop/replace operations.
// Renders all scenes bottom-to-top. Updates and routes input only to the top scene.
class SceneStack {
public:
    SceneStack();
    ~SceneStack();

    SceneStack(const SceneStack&) = delete;
    SceneStack& operator=(const SceneStack&) = delete;

    // Stack operations

    // push: new scene goes on top, previous top (if any) is paused
    void push(std::unique_ptr<Scene> scene);

    // pop: top scene is destroyed, new top (if any) is resumed
    // No-op if stack is empty.
    void pop();

    // replace: top scene is destroyed, new scene takes its place
    // If stack is empty, this is equivalent to push.
    void replace(std::unique_ptr<Scene> scene);

    // Frame operations

    // update: calls update(dt) on the topmost scene only
    void update(double dt);

    // handle_input: calls handle_input() on the topmost scene only
    void handle_input(const InputSnapshot& input);

    // render: calls render() on ALL scenes, bottom-to-top (painter's algorithm)
    void render();

    // State queries
    bool empty() const { return scenes_.empty(); }
    size_t size() const { return scenes_.size(); }

private:
    std::vector<std::unique_ptr<Scene>> scenes_;
};

} // namespace openitup
