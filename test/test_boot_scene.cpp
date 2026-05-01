#include <gtest/gtest.h>

#include <openitup/scene/boot_scene.h>
#include <openitup/scene/scene_stack.h>

using namespace openitup;

// BootScene tests: lifecycle and timer logic without rendering

TEST(BootSceneTest, TimerReachesThreeSeconds) {
    SceneStack stack;
    auto boot = std::make_unique<BootScene>(nullptr, nullptr, &stack);
    boot->on_enter();

    // Simulate 3.5 seconds of updates (60 Hz)
    double dt = 1.0 / 60.0;
    for (int i = 0; i < 210; ++i) {  // 210 ticks = 3.5 seconds
        boot->update(dt);
    }

    // No assertion — just verify no crash. Transition stub logs but doesn't push TitleScene yet.
}

TEST(BootSceneTest, LifecycleMethodsDoNotCrash) {
    SceneStack stack;
    auto boot = std::make_unique<BootScene>(nullptr, nullptr, &stack);

    boot->on_enter();
    boot->on_pause();
    boot->on_resume();
    boot->render();  // Should handle nullptr gracefully
    boot->on_exit();

    // No assertion — just verify no crash
}
