#pragma once

#include <sol/sol.hpp>
#include <string>
#include <memory>

namespace openitup {

class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();

    void init();
    bool run_file(const std::string& path);
    bool run_string(const std::string& code);
    sol::state& get_state();

private:
    void sandbox_dangerous_functions();

    std::unique_ptr<sol::state> lua_;
};

} // namespace openitup
