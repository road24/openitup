#include <gtest/gtest.h>

#include <openitup/lua/lua_engine.h>
#include <openitup/lua/lua_bindings.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/audio/null_audio_system.h>
#include <openitup/gfx/renderer.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/core/clock.h>
#include <openitup/data/profile.h>

using namespace openitup;

class LuaBindingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<LuaEngine>();
        engine_->init();
        register_all_bindings(engine_->get_state());
    }

    void TearDown() override {
        engine_.reset();
    }

    std::unique_ptr<LuaEngine> engine_;
};

// US-LUA-002: Input query API tests
TEST_F(LuaBindingsTest, InputIsPressedAvailable) {
    bool result = engine_->run_string("result = lua_input.is_pressed ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, InputIsHeldAvailable) {
    bool result = engine_->run_string("result = lua_input.is_held ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, InputPressedMaskAvailable) {
    bool result = engine_->run_string("result = lua_input.pressed_mask ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, InputIsPressedWithNullSnapshot) {
    // Should return false with null snapshot
    bool result = engine_->run_string("result = lua_input.is_pressed('START')");
    EXPECT_TRUE(result);
    bool pressed = engine_->get_state()["result"];
    EXPECT_FALSE(pressed);
}

TEST_F(LuaBindingsTest, InputIsPressedWithValidSnapshot) {
    // Create a snapshot with START pressed
    InputSnapshot snapshot(0, static_cast<uint32_t>(PadInput::START), 0, 1);
    register_input_bindings(engine_->get_state(), &snapshot);

    bool result = engine_->run_string("result = lua_input.is_pressed('START')");
    EXPECT_TRUE(result);
    bool pressed = engine_->get_state()["result"];
    EXPECT_TRUE(pressed);
}

TEST_F(LuaBindingsTest, InputIsHeldWithValidSnapshot) {
    // Create a snapshot with P1_CENTER held
    InputSnapshot snapshot(static_cast<uint32_t>(PadInput::P1_CENTER), 0, 0, 1);
    register_input_bindings(engine_->get_state(), &snapshot);

    bool result = engine_->run_string("result = lua_input.is_held('P1_CENTER')");
    EXPECT_TRUE(result);
    bool held = engine_->get_state()["result"];
    EXPECT_TRUE(held);
}

TEST_F(LuaBindingsTest, InputPressedMaskReturnsCorrectValue) {
    uint32_t mask = static_cast<uint32_t>(PadInput::START) | static_cast<uint32_t>(PadInput::P1_CENTER);
    InputSnapshot snapshot(0, mask, 0, 1);
    register_input_bindings(engine_->get_state(), &snapshot);

    bool result = engine_->run_string("result = lua_input.pressed_mask()");
    EXPECT_TRUE(result);
    uint32_t lua_mask = engine_->get_state()["result"];
    EXPECT_EQ(lua_mask, mask);
}

TEST_F(LuaBindingsTest, InputUnknownPadReturnsFlase) {
    InputSnapshot snapshot(0, 0, 0, 1);
    register_input_bindings(engine_->get_state(), &snapshot);

    bool result = engine_->run_string("result = lua_input.is_pressed('INVALID_PAD')");
    EXPECT_TRUE(result);
    bool pressed = engine_->get_state()["result"];
    EXPECT_FALSE(pressed);
}

// US-LUA-003: Audio control API tests
TEST_F(LuaBindingsTest, AudioPlayAvailable) {
    bool result = engine_->run_string("result = lua_audio.play ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, AudioStopAvailable) {
    bool result = engine_->run_string("result = lua_audio.stop ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, AudioSetVolumeAvailable) {
    bool result = engine_->run_string("result = lua_audio.set_volume ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, AudioGetVolumeAvailable) {
    bool result = engine_->run_string("result = lua_audio.get_volume ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, AudioGetPositionAvailable) {
    bool result = engine_->run_string("result = lua_audio.get_position ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, AudioPlayWithNullSystem) {
    // Should not crash with null audio system
    bool result = engine_->run_string("lua_audio.play()");
    EXPECT_TRUE(result);
}

TEST_F(LuaBindingsTest, AudioGetVolumeReturnsZeroWithNull) {
    bool result = engine_->run_string("result = lua_audio.get_volume()");
    EXPECT_TRUE(result);
    float volume = engine_->get_state()["result"];
    EXPECT_FLOAT_EQ(volume, 0.0f);
}

// US-LUA-004: Sprite/BGA rendering tests
TEST_F(LuaBindingsTest, RenderDrawSpriteAvailable) {
    bool result = engine_->run_string("result = lua_render.draw_sprite ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, RenderDrawBgaAvailable) {
    bool result = engine_->run_string("result = lua_render.draw_bga ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

// US-LUA-005: Scene stack navigation tests
TEST_F(LuaBindingsTest, ScenePushAvailable) {
    bool result = engine_->run_string("result = lua_scene.push ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, ScenePopAvailable) {
    bool result = engine_->run_string("result = lua_scene.pop ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, SceneReplaceAvailable) {
    bool result = engine_->run_string("result = lua_scene.replace ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, ScenePopWithNullStack) {
    // Should not crash with null scene stack
    bool result = engine_->run_string("lua_scene.pop()");
    EXPECT_TRUE(result);
}

// US-LUA-006: Profile/score access tests
TEST_F(LuaBindingsTest, ProfileNameAvailable) {
    bool result = engine_->run_string("result = lua_profile.name ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, ProfileHighScoresAvailable) {
    bool result = engine_->run_string("result = lua_profile.high_scores ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, ProfileNameReturnsEmptyWithNull) {
    bool result = engine_->run_string("result = lua_profile.name()");
    EXPECT_TRUE(result);
    std::string name = engine_->get_state()["result"];
    EXPECT_EQ(name, "");
}

TEST_F(LuaBindingsTest, ProfileNameReturnsCorrectValue) {
    auto profile = data::ProfileData::make_default("TestPlayer");
    register_profile_bindings(engine_->get_state(), &profile);

    bool result = engine_->run_string("result = lua_profile.name()");
    EXPECT_TRUE(result);
    std::string name = engine_->get_state()["result"];
    EXPECT_EQ(name, "TestPlayer");
}

// US-LUA-007: Timer utilities tests
TEST_F(LuaBindingsTest, TimerNowMsAvailable) {
    bool result = engine_->run_string("result = lua_timer.now_ms ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, TimerDtAvailable) {
    bool result = engine_->run_string("result = lua_timer.dt ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, TimerFpsAvailable) {
    bool result = engine_->run_string("result = lua_timer.fps ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, TimerDtReturnsFixedStep) {
    bool result = engine_->run_string("result = lua_timer.dt()");
    EXPECT_TRUE(result);
    double dt = engine_->get_state()["result"];
    EXPECT_DOUBLE_EQ(dt, FIXED_STEP);
}

TEST_F(LuaBindingsTest, TimerFpsReturns60) {
    bool result = engine_->run_string("result = lua_timer.fps()");
    EXPECT_TRUE(result);
    int fps = engine_->get_state()["result"];
    EXPECT_EQ(fps, 60);
}

// US-LUA-022: Primitive shape drawing tests
TEST_F(LuaBindingsTest, DrawRectAvailable) {
    bool result = engine_->run_string("result = lua_draw.rect ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, DrawLineAvailable) {
    bool result = engine_->run_string("result = lua_draw.line ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, DrawCircleAvailable) {
    bool result = engine_->run_string("result = lua_draw.circle ~= nil");
    EXPECT_TRUE(result);
    bool has_function = engine_->get_state()["result"];
    EXPECT_TRUE(has_function);
}

TEST_F(LuaBindingsTest, DrawRectWithNullRenderer) {
    // Should not crash with null renderer
    bool result = engine_->run_string("lua_draw.rect(0, 0, 100, 100, 255, 255, 255, 255)");
    EXPECT_TRUE(result);
}

// US-LUA-008: Instruction budget tests
TEST_F(LuaBindingsTest, InstructionBudgetEnforced) {
    // Set a very low budget
    set_instruction_budget(engine_->get_state(), 100);
    reset_instruction_count(engine_->get_state());

    // Try to run an infinite loop
    bool result = engine_->run_string(R"(
        while true do
            x = x + 1
        end
    )");

    // Should fail due to budget exceeded
    EXPECT_FALSE(result);
}

TEST_F(LuaBindingsTest, InstructionBudgetAllowsNormalCode) {
    set_instruction_budget(engine_->get_state(), 10000);
    reset_instruction_count(engine_->get_state());

    // Normal code should work fine
    bool result = engine_->run_string(R"(
        sum = 0
        for i = 1, 100 do
            sum = sum + i
        end
    )");

    EXPECT_TRUE(result);
    int sum = engine_->get_state()["sum"];
    EXPECT_EQ(sum, 5050);
}

// US-LUA-009: Error logging tests
TEST_F(LuaBindingsTest, SafeCallCatchesErrors) {
    engine_->run_string(R"(
        function test_error()
            error("test error message")
        end
    )");

    sol::protected_function func = engine_->get_state()["test_error"];
    bool success = safe_call(engine_->get_state(), func);
    EXPECT_FALSE(success);
}

TEST_F(LuaBindingsTest, SafeCallSucceedsWithValidFunction) {
    engine_->run_string(R"(
        function test_success()
            return 42
        end
    )");

    sol::protected_function func = engine_->get_state()["test_success"];
    bool success = safe_call(engine_->get_state(), func);
    EXPECT_TRUE(success);
}

// US-LUA-010: Sandbox tests (already in test_lua_engine.cpp, but verify bindings don't break it)
TEST_F(LuaBindingsTest, SandboxStillEnforcedAfterBindings) {
    // os.execute should still be nil after bindings
    engine_->run_string("result = os.execute");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaBindingsTest, LoadfileStillRemovedAfterBindings) {
    // loadfile should still be nil after bindings
    engine_->run_string("result = loadfile");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

// Integration test: All APIs available
TEST_F(LuaBindingsTest, AllApisAvailable) {
    bool result = engine_->run_string(R"(
        apis_available =
            lua_input ~= nil and
            lua_audio ~= nil and
            lua_render ~= nil and
            lua_scene ~= nil and
            lua_profile ~= nil and
            lua_timer ~= nil and
            lua_draw ~= nil
    )");

    EXPECT_TRUE(result);
    bool all_available = engine_->get_state()["apis_available"];
    EXPECT_TRUE(all_available);
}

// Test that Lua can call multiple APIs in sequence
TEST_F(LuaBindingsTest, MultipleApiCallsInSequence) {
    bool result = engine_->run_string(R"(
        -- Check input
        local pressed = lua_input.is_pressed('START')

        -- Check audio
        local volume = lua_audio.get_volume()

        -- Check timer
        local fps = lua_timer.fps()

        -- Check profile
        local name = lua_profile.name()

        all_calls_succeeded = true
    )");

    EXPECT_TRUE(result);
    bool succeeded = engine_->get_state()["all_calls_succeeded"];
    EXPECT_TRUE(succeeded);
}
