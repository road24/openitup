#pragma once

#include <openitup/input/input_snapshot.h>

namespace openitup {

// Abstract base class for all game screens.
// Lifecycle: on_enter → (update/handle_input/render loop) → on_exit
// Overlays: on_pause when covered, on_resume when uncovered
class Scene {
public:
    virtual ~Scene() = default;

    // Called when scene becomes active (pushed or revealed after pop)
    virtual void on_enter() = 0;

    // Called before scene is destroyed (popped or replaced)
    virtual void on_exit() = 0;

    // Called when another scene is pushed on top (this scene is covered)
    virtual void on_pause() = 0;

    // Called when the scene above is popped (this scene is uncovered)
    virtual void on_resume() = 0;

    // Called once per logic tick (60 Hz) — only on the topmost scene
    virtual void update(double dt) = 0;

    // Called with the current input snapshot — only on the topmost scene
    virtual void handle_input(const InputSnapshot& input) = 0;

    // Called every render frame — on ALL scenes, bottom-to-top
    virtual void render() = 0;

protected:
    Scene() = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
};

} // namespace openitup
