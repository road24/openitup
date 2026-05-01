#include <openitup/lua/lua_engine.h>

#include <spdlog/spdlog.h>
#include <fstream>

namespace openitup {

LuaEngine::LuaEngine() : lua_(std::make_unique<sol::state>()) {}

LuaEngine::~LuaEngine() = default;

void LuaEngine::init() {
    lua_->open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::table,
        sol::lib::math,
        sol::lib::utf8,
        sol::lib::debug
    );

    sandbox_dangerous_functions();

    spdlog::info("Lua engine initialized (Lua {})", LUA_VERSION_MAJOR "." LUA_VERSION_MINOR);
}

bool LuaEngine::run_file(const std::string& path) {
    try {
        lua_->script_file(path);
        return true;
    } catch (const sol::error& e) {
        spdlog::error("Lua error loading file '{}': {}", path, e.what());
        return false;
    }
}

bool LuaEngine::run_string(const std::string& code) {
    try {
        lua_->script(code);
        return true;
    } catch (const sol::error& e) {
        spdlog::error("Lua error executing string: {}", e.what());
        return false;
    }
}

sol::state& LuaEngine::get_state() {
    return *lua_;
}

void LuaEngine::sandbox_dangerous_functions() {
    // Remove dangerous io operations
    (*lua_)["io"]["popen"] = sol::nil;

    // Remove dangerous os operations
    (*lua_)["os"]["execute"] = sol::nil;
    (*lua_)["os"]["exit"] = sol::nil;
    (*lua_)["os"]["remove"] = sol::nil;
    (*lua_)["os"]["rename"] = sol::nil;
    (*lua_)["os"]["tmpname"] = sol::nil;

    // Remove loadfile (use engine-provided run_file instead)
    (*lua_)["loadfile"] = sol::nil;
    (*lua_)["dofile"] = sol::nil;

    spdlog::debug("Lua sandbox: removed dangerous functions (io.popen, os.execute, loadfile, dofile)");
}

} // namespace openitup
