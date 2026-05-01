#pragma once

#include <sol/sol.hpp>

namespace openitup {

// Forward declarations for binding context
class InputSnapshot;
class AudioSystem;
class Renderer;
class TextureCache;
class SceneStack;
class Clock;
class GamePackage;
namespace data { struct ProfileData; }

// Maximum Lua instructions per frame (US-LUA-008)
constexpr int DEFAULT_LUA_INSTRUCTION_BUDGET = 100000;

// Register all Lua API bindings to the given state.
// Must be called after lua_->open_libraries() in LuaEngine::init().
void register_all_bindings(sol::state& lua);

// Binding categories (each can be registered independently for testing)
void register_input_bindings(sol::state& lua, const InputSnapshot* snapshot);
void register_audio_bindings(sol::state& lua, AudioSystem* audio);
void register_render_bindings(sol::state& lua, Renderer* renderer, const TextureCache* cache);
void register_scene_bindings(sol::state& lua, SceneStack* scene_stack);
void register_profile_bindings(sol::state& lua, const data::ProfileData* profile);
void register_timer_bindings(sol::state& lua, const Clock* clock);
void register_shape_bindings(sol::state& lua, Renderer* renderer);
void register_game_bindings(sol::state& lua, const GamePackage* package);

// Budget enforcement (US-LUA-008)
void set_instruction_budget(sol::state& lua, int max_instructions);
void reset_instruction_count(sol::state& lua);

// Error logging with stack trace (US-LUA-009)
bool safe_call(sol::state& lua, const sol::protected_function& func);
bool safe_call_with_args(sol::state& lua, const sol::protected_function& func, sol::variadic_args args);

} // namespace openitup
