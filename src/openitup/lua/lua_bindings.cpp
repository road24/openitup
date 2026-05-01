#include <openitup/lua/lua_bindings.h>

#include <cmath>
#include <spdlog/spdlog.h>

#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/audio/audio_system.h>
#include <openitup/gfx/renderer.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/scene/scene_stack.h>
#include <openitup/scene/scene.h>
#include <openitup/core/clock.h>
#include <openitup/data/profile.h>
#include <openitup/sprite/sprite.h>
#include <openitup/sprite/sprite_loader.h>
#include <openitup/bga/bga_loader.h>
#include <openitup/gfx/blend_modes.h>
#include <openitup/math/types.h>
#include <openitup/sprite/sprite_loader.h>
#include <openitup/bga/animation.h>
#include <openitup/bga/bga_loader.h>

namespace openitup {

namespace {

// Global instruction counter for budget enforcement
int g_instruction_count = 0;
int g_max_instructions = DEFAULT_LUA_INSTRUCTION_BUDGET;

// Debug hook for instruction counting (US-LUA-008)
void instruction_count_hook(lua_State* L, lua_Debug* ar) {
    g_instruction_count++;
    if (g_instruction_count > g_max_instructions) {
        luaL_error(L, "Lua instruction budget exceeded (%d instructions)", g_max_instructions);
    }
}

} // anonymous namespace

void set_instruction_budget(sol::state& lua, int max_instructions) {
    g_max_instructions = max_instructions;
    lua_sethook(lua.lua_state(), instruction_count_hook, LUA_MASKCOUNT, 1000);
    spdlog::debug("Lua instruction budget set to {}", max_instructions);
}

void reset_instruction_count(sol::state& lua) {
    g_instruction_count = 0;
}

bool safe_call(sol::state& lua, const sol::protected_function& func) {
    sol::protected_function_result result = func();
    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("Lua error: {}", err.what());

        // Get Lua stack trace
        lua_Debug ar;
        int level = 0;
        while (lua_getstack(lua.lua_state(), level, &ar)) {
            lua_getinfo(lua.lua_state(), "Sln", &ar);
            if (ar.name) {
                spdlog::error("  [{}] {}:{} in function '{}'",
                    level, ar.short_src, ar.currentline, ar.name);
            } else {
                spdlog::error("  [{}] {}:{}",
                    level, ar.short_src, ar.currentline);
            }
            level++;
        }
        return false;
    }
    return true;
}

bool safe_call_with_args(sol::state& lua, const sol::protected_function& func, sol::variadic_args args) {
    sol::protected_function_result result = func(args);
    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("Lua error: {}", err.what());

        // Get Lua stack trace
        lua_Debug ar;
        int level = 0;
        while (lua_getstack(lua.lua_state(), level, &ar)) {
            lua_getinfo(lua.lua_state(), "Sln", &ar);
            if (ar.name) {
                spdlog::error("  [{}] {}:{} in function '{}'",
                    level, ar.short_src, ar.currentline, ar.name);
            } else {
                spdlog::error("  [{}] {}:{}",
                    level, ar.short_src, ar.currentline);
            }
            level++;
        }
        return false;
    }
    return true;
}

// US-LUA-002: Input query API
void register_input_bindings(sol::state& lua, const InputSnapshot* snapshot) {
    auto lua_input = lua["lua_input"].get_or_create<sol::table>();

    lua_input["is_pressed"] = [snapshot](const std::string& pad_name) -> bool {
        if (!snapshot) return false;

        // Map string to PadInput enum
        if (pad_name == "P1_DOWN_LEFT") return snapshot->is_pressed(PadInput::P1_DOWN_LEFT);
        if (pad_name == "P1_UP_LEFT") return snapshot->is_pressed(PadInput::P1_UP_LEFT);
        if (pad_name == "P1_CENTER") return snapshot->is_pressed(PadInput::P1_CENTER);
        if (pad_name == "P1_UP_RIGHT") return snapshot->is_pressed(PadInput::P1_UP_RIGHT);
        if (pad_name == "P1_DOWN_RIGHT") return snapshot->is_pressed(PadInput::P1_DOWN_RIGHT);
        if (pad_name == "P2_DOWN_LEFT") return snapshot->is_pressed(PadInput::P2_DOWN_LEFT);
        if (pad_name == "P2_UP_LEFT") return snapshot->is_pressed(PadInput::P2_UP_LEFT);
        if (pad_name == "P2_CENTER") return snapshot->is_pressed(PadInput::P2_CENTER);
        if (pad_name == "P2_UP_RIGHT") return snapshot->is_pressed(PadInput::P2_UP_RIGHT);
        if (pad_name == "P2_DOWN_RIGHT") return snapshot->is_pressed(PadInput::P2_DOWN_RIGHT);
        if (pad_name == "START") return snapshot->is_pressed(PadInput::START);
        if (pad_name == "BACK") return snapshot->is_pressed(PadInput::BACK);
        if (pad_name == "SELECT") return snapshot->is_pressed(PadInput::SELECT);
        if (pad_name == "COIN") return snapshot->is_pressed(PadInput::COIN);

        spdlog::warn("Unknown pad input: {}", pad_name);
        return false;
    };

    lua_input["is_held"] = [snapshot](const std::string& pad_name) -> bool {
        if (!snapshot) return false;

        if (pad_name == "P1_DOWN_LEFT") return snapshot->is_held(PadInput::P1_DOWN_LEFT);
        if (pad_name == "P1_UP_LEFT") return snapshot->is_held(PadInput::P1_UP_LEFT);
        if (pad_name == "P1_CENTER") return snapshot->is_held(PadInput::P1_CENTER);
        if (pad_name == "P1_UP_RIGHT") return snapshot->is_held(PadInput::P1_UP_RIGHT);
        if (pad_name == "P1_DOWN_RIGHT") return snapshot->is_held(PadInput::P1_DOWN_RIGHT);
        if (pad_name == "P2_DOWN_LEFT") return snapshot->is_held(PadInput::P2_DOWN_LEFT);
        if (pad_name == "P2_UP_LEFT") return snapshot->is_held(PadInput::P2_UP_LEFT);
        if (pad_name == "P2_CENTER") return snapshot->is_held(PadInput::P2_CENTER);
        if (pad_name == "P2_UP_RIGHT") return snapshot->is_held(PadInput::P2_UP_RIGHT);
        if (pad_name == "P2_DOWN_RIGHT") return snapshot->is_held(PadInput::P2_DOWN_RIGHT);
        if (pad_name == "START") return snapshot->is_held(PadInput::START);
        if (pad_name == "BACK") return snapshot->is_held(PadInput::BACK);
        if (pad_name == "SELECT") return snapshot->is_held(PadInput::SELECT);
        if (pad_name == "COIN") return snapshot->is_held(PadInput::COIN);

        spdlog::warn("Unknown pad input: {}", pad_name);
        return false;
    };

    lua_input["pressed_mask"] = [snapshot]() -> uint32_t {
        if (!snapshot) return 0;
        return snapshot->pressed_mask();
    };

    spdlog::debug("Lua input bindings registered");
}

// US-LUA-003: Audio control API
void register_audio_bindings(sol::state& lua, AudioSystem* audio) {
    auto lua_audio = lua["lua_audio"].get_or_create<sol::table>();

    lua_audio["play"] = [audio]() {
        if (audio) audio->play();
    };

    lua_audio["stop"] = [audio]() {
        if (audio) audio->stop();
    };

    lua_audio["pause"] = [audio]() {
        if (audio) audio->pause();
    };

    lua_audio["resume"] = [audio]() {
        if (audio) audio->resume();
    };

    lua_audio["set_volume"] = [audio](float volume) {
        if (audio) audio->set_music_volume(volume);
    };

    lua_audio["get_volume"] = [audio]() -> float {
        if (!audio) return 0.0f;
        return audio->get_music_volume();
    };

    lua_audio["get_position"] = [audio]() -> double {
        if (!audio) return 0.0;
        return audio->get_position_ms();
    };

    lua_audio["get_duration"] = [audio]() -> double {
        if (!audio) return 0.0;
        return audio->get_duration_ms();
    };

    spdlog::debug("Lua audio bindings registered");
}

// US-LUA-004: Sprite/BGA rendering
void register_render_bindings(sol::state& lua, Renderer* renderer, TextureCache* cache) {
    auto lua_render = lua["lua_render"].get_or_create<sol::table>();

    lua_render["draw_sprite"] = [renderer, cache](const std::string& sprj_path, float x, float y, float t) {
        if (!renderer || !cache) {
            spdlog::warn("Renderer or cache not available for draw_sprite");
            return;
        }

        // Load sprite from path
        auto sprite = load_sprj(sprj_path, *cache);
        if (!sprite) {
            spdlog::error("Failed to load sprite: {}", sprj_path);
            return;
        }

        LayerTransform transform;
        transform.translate_x = x;
        transform.translate_y = y;

        ColorMod color{1.0f, 1.0f, 1.0f, 1.0f};

        sprite->draw(renderer->get(), *cache, t, transform, color, SDL_BLENDMODE_BLEND);
    };

    lua_render["draw_bga"] = [renderer, cache](const std::string& bgaj_path, float tick) {
        if (!renderer || !cache) {
            spdlog::warn("Renderer or cache not available for draw_bga");
            return;
        }

        auto bga = load_bga_auto(bgaj_path, *cache);
        if (!bga) {
            spdlog::error("Failed to load BGA: {}", bgaj_path);
            return;
        }

        bga->render(renderer->get(), *cache, tick, resolve_blend_mode);
    };

    spdlog::debug("Lua render bindings registered");
}

// US-LUA-005: Scene stack navigation API
void register_scene_bindings(sol::state& lua, SceneStack* scene_stack) {
    auto lua_scene = lua["lua_scene"].get_or_create<sol::table>();

    lua_scene["push"] = [scene_stack](const std::string& name) {
        if (!scene_stack) {
            spdlog::warn("SceneStack not available for push");
            return;
        }
        spdlog::info("Lua requested scene push: {}", name);
        // Note: Actual scene creation would require a factory or registry
        // For now, just log the request
    };

    lua_scene["pop"] = [scene_stack]() {
        if (!scene_stack) {
            spdlog::warn("SceneStack not available for pop");
            return;
        }
        scene_stack->pop();
    };

    lua_scene["replace"] = [scene_stack](const std::string& name) {
        if (!scene_stack) {
            spdlog::warn("SceneStack not available for replace");
            return;
        }
        spdlog::info("Lua requested scene replace: {}", name);
        // Note: Actual scene creation would require a factory or registry
    };

    spdlog::debug("Lua scene bindings registered");
}

// US-LUA-006: Profile/score access
void register_profile_bindings(sol::state& lua, const data::ProfileData* profile) {
    auto lua_profile = lua["lua_profile"].get_or_create<sol::table>();

    lua_profile["name"] = [profile]() -> std::string {
        if (!profile) return "";
        return profile->display_name;
    };

    lua_profile["high_scores"] = [profile](const std::string& chart_hash) -> sol::object {
        if (!profile) return sol::nil;

        auto it = profile->high_scores.find(chart_hash);
        if (it == profile->high_scores.end()) {
            return sol::nil;
        }

        // Return a simple table representation of scores
        // In a full implementation, this would convert the vector properly
        return sol::nil;  // Placeholder
    };

    lua_profile["total_plays"] = [profile]() -> int {
        if (!profile) return 0;
        return profile->total_plays;
    };

    spdlog::debug("Lua profile bindings registered");
}

// US-LUA-007: Timer utilities
void register_timer_bindings(sol::state& lua, const Clock* clock) {
    auto lua_timer = lua["lua_timer"].get_or_create<sol::table>();

    lua_timer["now_ms"] = [clock]() -> double {
        if (!clock) return 0.0;
        return clock->elapsed() * 1000.0;
    };

    lua_timer["dt"] = []() -> double {
        // Return fixed timestep for now
        return FIXED_STEP;
    };

    lua_timer["fps"] = []() -> int {
        // Return target FPS
        return static_cast<int>(1.0 / FIXED_STEP);
    };

    spdlog::debug("Lua timer bindings registered");
}

// US-LUA-022: Primitive shape drawing
void register_shape_bindings(sol::state& lua, Renderer* renderer) {
    auto lua_draw = lua["lua_draw"].get_or_create<sol::table>();

    lua_draw["rect"] = [renderer](float x, float y, float w, float h,
                                   int r, int g, int b, int a) {
        if (!renderer) return;

        SDL_FRect rect{x, y, w, h};
        SDL_SetRenderDrawColor(renderer->get(), r, g, b, a);
        SDL_RenderFillRect(renderer->get(), &rect);
    };

    lua_draw["line"] = [renderer](float x1, float y1, float x2, float y2,
                                   int r, int g, int b, int a) {
        if (!renderer) return;

        SDL_SetRenderDrawColor(renderer->get(), r, g, b, a);
        SDL_RenderLine(renderer->get(), x1, y1, x2, y2);
    };

    lua_draw["circle"] = [renderer](float x, float y, float radius,
                                     int r, int g, int b, int a) {
        if (!renderer) return;

        SDL_SetRenderDrawColor(renderer->get(), r, g, b, a);

        // Simple circle drawing using SDL (approximation with lines)
        const int segments = 32;
        for (int i = 0; i < segments; ++i) {
            float angle1 = (2.0f * M_PI * i) / segments;
            float angle2 = (2.0f * M_PI * (i + 1)) / segments;

            float x1 = x + radius * cosf(angle1);
            float y1 = y + radius * sinf(angle1);
            float x2 = x + radius * cosf(angle2);
            float y2 = y + radius * sinf(angle2);

            SDL_RenderLine(renderer->get(), x1, y1, x2, y2);
        }
    };

    spdlog::debug("Lua shape drawing bindings registered");
}

void register_all_bindings(sol::state& lua) {
    // Register with null pointers initially
    // The actual engine should call individual register functions
    // with proper context pointers when available
    register_input_bindings(lua, nullptr);
    register_audio_bindings(lua, nullptr);
    register_render_bindings(lua, static_cast<Renderer*>(nullptr), static_cast<TextureCache*>(nullptr));
    register_scene_bindings(lua, nullptr);
    register_profile_bindings(lua, nullptr);
    register_timer_bindings(lua, nullptr);
    register_shape_bindings(lua, nullptr);

    // Set default instruction budget
    set_instruction_budget(lua, DEFAULT_LUA_INSTRUCTION_BUDGET);

    spdlog::info("All Lua bindings registered");
}

} // namespace openitup
