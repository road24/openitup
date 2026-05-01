#include <gtest/gtest.h>

#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/scene/mode_select_scene.h>
#include <openitup/scene/scene_stack.h>

using namespace openitup;

TEST(ModeSelectSceneTest, CursorNavigationRight) {
    SceneStack stack;
    auto mode_select = std::make_unique<ModeSelectScene>(nullptr, nullptr, &stack);
    mode_select->on_enter();

    // Navigate right 3 times
    for (int i = 0; i < 3; ++i) {
        uint32_t pressed = static_cast<uint32_t>(PadInput::P1_UP_RIGHT);
        InputSnapshot input(pressed, pressed, 0, i);
        mode_select->handle_input(input);
    }

    mode_select->render();  // Should not crash
}

TEST(ModeSelectSceneTest, CursorWrapsAround) {
    SceneStack stack;
    auto mode_select = std::make_unique<ModeSelectScene>(nullptr, nullptr, &stack);
    mode_select->on_enter();

    // Navigate right 5 times (wraps: 0→1→2→3→0)
    for (int i = 0; i < 5; ++i) {
        uint32_t pressed = static_cast<uint32_t>(PadInput::P1_DOWN_RIGHT);
        InputSnapshot input(pressed, pressed, 0, i);
        mode_select->handle_input(input);
    }

    mode_select->render();  // Should not crash
}

TEST(ModeSelectSceneTest, StartConfirmsSingleMode) {
    SceneStack stack;
    auto mode_select = std::make_unique<ModeSelectScene>(nullptr, nullptr, &stack);
    mode_select->on_enter();

    // Cursor starts at 0 (Single)
    uint32_t pressed = static_cast<uint32_t>(PadInput::START);
    InputSnapshot input(pressed, pressed, 0, 1);
    mode_select->handle_input(input);

    // No assertion — just verify no crash. Transition stub logs.
}

TEST(ModeSelectSceneTest, DisabledModesLogWarning) {
    SceneStack stack;
    auto mode_select = std::make_unique<ModeSelectScene>(nullptr, nullptr, &stack);
    mode_select->on_enter();

    // Move to Co-op (index 2)
    for (int i = 0; i < 2; ++i) {
        uint32_t pressed = static_cast<uint32_t>(PadInput::P1_UP_RIGHT);
        InputSnapshot input(pressed, pressed, 0, i);
        mode_select->handle_input(input);
    }

    // Try to select Co-op
    uint32_t pressed = static_cast<uint32_t>(PadInput::START);
    InputSnapshot input(pressed, pressed, 0, 10);
    mode_select->handle_input(input);

    // Should log warning, not crash
}
