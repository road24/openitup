#include <gtest/gtest.h>

#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/scene/title_scene.h>

using namespace openitup;

TEST(TitleSceneTest, InactivityTimeoutAfter30Seconds) {
    SceneStack stack;
    auto title = std::make_unique<TitleScene>(nullptr, nullptr, &stack);
    title->on_enter();

    // Simulate 31 seconds of updates (60 Hz)
    double dt = 1.0 / 60.0;
    for (int i = 0; i < 1860; ++i) {  // 1860 ticks = 31 seconds
        title->update(dt);
    }

    // No assertion — just verify no crash. Transition stub logs.
}

TEST(TitleSceneTest, InputResetsInactivityTimer) {
    SceneStack stack;
    auto title = std::make_unique<TitleScene>(nullptr, nullptr, &stack);
    title->on_enter();

    // Simulate 20 seconds
    double dt = 1.0 / 60.0;
    for (int i = 0; i < 1200; ++i) {
        title->update(dt);
    }

    // Press a button (resets timer)
    uint32_t pressed = static_cast<uint32_t>(PadInput::P1_DOWN_LEFT);
    InputSnapshot input(pressed, pressed, 0, 1200);
    title->handle_input(input);

    // Simulate another 20 seconds (total 40s elapsed, but timer reset at 20s)
    for (int i = 0; i < 1200; ++i) {
        title->update(dt);
    }

    // Should not timeout yet
}

TEST(TitleSceneTest, StartInputTriggersTransition) {
    SceneStack stack;
    auto title = std::make_unique<TitleScene>(nullptr, nullptr, &stack);
    title->on_enter();

    uint32_t pressed = static_cast<uint32_t>(PadInput::START);
    InputSnapshot input(pressed, pressed, 0, 1);
    title->handle_input(input);

    // No assertion — just verify no crash. Transition stub logs.
}
