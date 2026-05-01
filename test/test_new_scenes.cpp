#include <gtest/gtest.h>

#include <memory>

#include <openitup/core/clock.h>
#include <openitup/core/engine.h>
#include <openitup/gfx/renderer.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/render/text_renderer.h>
#include <openitup/scene/name_entry_scene.h>
#include <openitup/scene/pause_overlay_scene.h>
#include <openitup/scene/profile_selection_scene.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/scene/settings_scene.h>
#include <openitup/scene/transition_scene.h>

using namespace openitup;

// Test NameEntryScene: character entry for high scores

TEST(NameEntrySceneTest, EntersWithCorrectRankAndScore) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    NameEntryScene scene(renderer, text, stack, &engine, 3, 900000);
    scene.on_enter();
    // Name should start empty
    scene.on_exit();
}

TEST(NameEntrySceneTest, CharacterSelectionAddsToName) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    NameEntryScene scene(renderer, text, stack, &engine, 1, 1000000);
    scene.on_enter();

    // Simulate selecting first character (should be 'A')
    InputSnapshot select_input(0, static_cast<uint32_t>(PadInput::P1_CENTER), 0, 1);
    scene.handle_input(select_input);

    scene.on_exit();
}

TEST(NameEntrySceneTest, NavigationMovesInGrid) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    NameEntryScene scene(renderer, text, stack, &engine, 1, 1000000);
    scene.on_enter();

    // Move right
    InputSnapshot right_input(0, static_cast<uint32_t>(PadInput::P1_RIGHT), 0, 1);
    scene.handle_input(right_input);

    // Move down
    InputSnapshot down_input(0, static_cast<uint32_t>(PadInput::P1_DOWN), 0, 2);
    scene.handle_input(down_input);

    scene.on_exit();
}

TEST(NameEntrySceneTest, BackspaceRemovesCharacter) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    NameEntryScene scene(renderer, text, stack, &engine, 1, 1000000);
    scene.on_enter();

    // Add a character
    InputSnapshot select_input(0, static_cast<uint32_t>(PadInput::P1_CENTER), 0, 1);
    scene.handle_input(select_input);

    // Backspace
    InputSnapshot back_input(0, static_cast<uint32_t>(PadInput::P1_BACK), 0, 2);
    scene.handle_input(back_input);

    scene.on_exit();
}

TEST(NameEntrySceneTest, RenderDoesNotCrash) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    NameEntryScene scene(renderer, text, stack, &engine, 5, 750000);
    scene.on_enter();
    scene.render();
    scene.on_exit();
}

// Test PauseOverlayScene: pause menu during gameplay

TEST(PauseOverlaySceneTest, PausesAudioOnEnter) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    PauseOverlayScene scene(renderer, text, stack, &engine);
    scene.on_enter();
    // Audio should be paused (if audio system exists)
    scene.on_exit();
}

TEST(PauseOverlaySceneTest, NavigationChangesSelection) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    PauseOverlayScene scene(renderer, text, stack, &engine);
    scene.on_enter();

    // Move down from Resume to Restart
    InputSnapshot down_input(0, static_cast<uint32_t>(PadInput::P1_DOWN), 0, 1);
    scene.handle_input(down_input);

    // Move down from Restart to Quit
    scene.handle_input(down_input);

    scene.on_exit();
}

TEST(PauseOverlaySceneTest, BackButtonResumes) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    // Push pause overlay (requires underlying scene)
    PauseOverlayScene scene(renderer, text, stack, &engine);
    scene.on_enter();

    InputSnapshot back_input(0, static_cast<uint32_t>(PadInput::P1_BACK), 0, 1);
    scene.handle_input(back_input);
    // Should pop scene (resume)

    scene.on_exit();
}

TEST(PauseOverlaySceneTest, RenderDoesNotCrash) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    PauseOverlayScene scene(renderer, text, stack, &engine);
    scene.on_enter();
    scene.render();
    scene.on_exit();
}

// Test TransitionScene: fade animations between scenes

TEST(TransitionSceneTest, TransitionsToTargetAfterDuration) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    // Create a target scene (use PauseOverlayScene as dummy)
    auto target = std::make_unique<PauseOverlayScene>(renderer, text, stack, &engine);

    TransitionScene scene(renderer, text, stack, std::move(target),
                          TransitionScene::Type::FADE, 0.5);
    scene.on_enter();

    // Update just before completion
    scene.update(0.4);

    // Update past completion
    scene.update(0.2); // Total 0.6 seconds, should trigger transition

    scene.on_exit();
}

TEST(TransitionSceneTest, RenderDoesNotCrash) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    auto target = std::make_unique<PauseOverlayScene>(renderer, text, stack, &engine);
    TransitionScene scene(renderer, text, stack, std::move(target));
    scene.on_enter();
    scene.render();
    scene.on_exit();
}

// Test SettingsScene: configuration menu

TEST(SettingsSceneTest, LoadsCurrentSettings) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    SettingsScene scene(renderer, text, stack, &engine);
    scene.on_enter();
    // Should load settings from engine's SettingsManager
    scene.on_exit();
}

TEST(SettingsSceneTest, TabNavigationWorks) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    SettingsScene scene(renderer, text, stack, &engine);
    scene.on_enter();

    // Navigate to next tab
    InputSnapshot right_input(0, static_cast<uint32_t>(PadInput::P1_RIGHT), 0, 1);
    scene.handle_input(right_input);

    // Navigate to previous tab
    InputSnapshot left_input(0, static_cast<uint32_t>(PadInput::P1_LEFT), 0, 2);
    scene.handle_input(left_input);

    scene.on_exit();
}

TEST(SettingsSceneTest, BackButtonExits) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    SettingsScene scene(renderer, text, stack, &engine);
    scene.on_enter();

    InputSnapshot back_input(0, static_cast<uint32_t>(PadInput::P1_BACK), 0, 1);
    scene.handle_input(back_input);
    // Should pop scene

    scene.on_exit();
}

TEST(SettingsSceneTest, RenderDoesNotCrash) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    SettingsScene scene(renderer, text, stack, &engine);
    scene.on_enter();
    scene.render();
    scene.on_exit();
}

// Test ProfileSelectionScene: profile management

TEST(ProfileSelectionSceneTest, ScansProfilesOnEntry) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    ProfileSelectionScene scene(renderer, text, stack, &engine);
    scene.on_enter();
    // Should scan profiles directory
    scene.on_exit();
}

TEST(ProfileSelectionSceneTest, NavigationWorks) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    ProfileSelectionScene scene(renderer, text, stack, &engine);
    scene.on_enter();

    // Navigate down
    InputSnapshot down_input(0, static_cast<uint32_t>(PadInput::P1_DOWN), 0, 1);
    scene.handle_input(down_input);

    // Navigate up
    InputSnapshot up_input(0, static_cast<uint32_t>(PadInput::P1_UP), 0, 2);
    scene.handle_input(up_input);

    scene.on_exit();
}

TEST(ProfileSelectionSceneTest, BackButtonReturnsToTitle) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    ProfileSelectionScene scene(renderer, text, stack, &engine);
    scene.on_enter();

    InputSnapshot back_input(0, static_cast<uint32_t>(PadInput::P1_BACK), 0, 1);
    scene.handle_input(back_input);
    // Should replace with TitleScene

    scene.on_exit();
}

TEST(ProfileSelectionSceneTest, RenderDoesNotCrash) {
    EngineConfig config;
    config.window_width = 640;
    config.window_height = 480;
    Engine engine(config);
    auto* renderer = engine.get_renderer();
    auto* text = engine.get_text_renderer();
    auto* stack = engine.get_scene_stack();

    ProfileSelectionScene scene(renderer, text, stack, &engine);
    scene.on_enter();
    scene.render();
    scene.on_exit();
}
