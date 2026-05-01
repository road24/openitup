#include <gtest/gtest.h>
#include <openitup/lua/lua_engine.h>

#include <fstream>
#include <filesystem>
#include <cstdio>

using namespace openitup;

class LuaEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<LuaEngine>();
        engine_->init();
    }

    void TearDown() override {
        engine_.reset();
        // Clean up any temp files
        if (!temp_file_path_.empty() && std::filesystem::exists(temp_file_path_)) {
            std::filesystem::remove(temp_file_path_);
        }
    }

    std::string create_temp_lua_file(const std::string& content) {
        temp_file_path_ = std::tmpnam(nullptr);
        temp_file_path_ += ".lua";
        std::ofstream out(temp_file_path_);
        out << content;
        out.close();
        return temp_file_path_;
    }

    std::unique_ptr<LuaEngine> engine_;
    std::string temp_file_path_;
};

TEST_F(LuaEngineTest, LuaStateInitializes) {
    EXPECT_NO_THROW(engine_->get_state());
}

TEST_F(LuaEngineTest, RunStringExecutesCode) {
    bool result = engine_->run_string("x = 42");
    EXPECT_TRUE(result);

    int x = engine_->get_state()["x"];
    EXPECT_EQ(x, 42);
}

TEST_F(LuaEngineTest, RunStringReturnsValues) {
    bool result = engine_->run_string("return 2 + 3");
    EXPECT_TRUE(result);
}

TEST_F(LuaEngineTest, RunStringWithSyntaxErrorReturnsFalse) {
    bool result = engine_->run_string("invalid lua syntax !@#");
    EXPECT_FALSE(result);
}

TEST_F(LuaEngineTest, RunStringWithRuntimeErrorReturnsFalse) {
    bool result = engine_->run_string("error('intentional error')");
    EXPECT_FALSE(result);
}

TEST_F(LuaEngineTest, RunFileLoadsAndExecutes) {
    std::string path = create_temp_lua_file("y = 100");

    bool result = engine_->run_file(path);
    EXPECT_TRUE(result);

    int y = engine_->get_state()["y"];
    EXPECT_EQ(y, 100);
}

TEST_F(LuaEngineTest, RunFileWithNonexistentFileReturnsFalse) {
    bool result = engine_->run_file("/nonexistent/path/to/script.lua");
    EXPECT_FALSE(result);
}

TEST_F(LuaEngineTest, RunFileWithSyntaxErrorReturnsFalse) {
    std::string path = create_temp_lua_file("local x = ");

    bool result = engine_->run_file(path);
    EXPECT_FALSE(result);
}

TEST_F(LuaEngineTest, StandardLibrariesAvailable) {
    // Test math library
    bool result = engine_->run_string("z = math.sqrt(16)");
    EXPECT_TRUE(result);
    double z = engine_->get_state()["z"];
    EXPECT_DOUBLE_EQ(z, 4.0);

    // Test string library
    result = engine_->run_string("s = string.upper('hello')");
    EXPECT_TRUE(result);
    std::string s = engine_->get_state()["s"];
    EXPECT_EQ(s, "HELLO");

    // Test table library
    result = engine_->run_string("t = {1, 2, 3}; table.insert(t, 4); count = #t");
    EXPECT_TRUE(result);
    int count = engine_->get_state()["count"];
    EXPECT_EQ(count, 4);
}

TEST_F(LuaEngineTest, DangerousOsExecuteRemoved) {
    // os.execute should be nil
    engine_->run_string("result = os.execute");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaEngineTest, DangerousOsExitRemoved) {
    // os.exit should be nil
    engine_->run_string("result = os.exit");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaEngineTest, DangerousOsRemoveRemoved) {
    // os.remove should be nil
    engine_->run_string("result = os.remove");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaEngineTest, DangerousOsRenameRemoved) {
    // os.rename should be nil
    engine_->run_string("result = os.rename");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaEngineTest, DangerousIoPopenRemoved) {
    // io.popen should be nil
    engine_->run_string("result = io.popen");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaEngineTest, LoadfileRemoved) {
    // loadfile should be nil (use run_file instead)
    engine_->run_string("result = loadfile");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaEngineTest, DofileRemoved) {
    // dofile should be nil (use run_file instead)
    engine_->run_string("result = dofile");
    sol::object result = engine_->get_state()["result"];
    EXPECT_FALSE(result.valid());
}

TEST_F(LuaEngineTest, SafeOsFunctionsStillAvailable) {
    // os.time should still work
    engine_->run_string("result = os.time()");
    sol::object result = engine_->get_state()["result"];
    EXPECT_TRUE(result.valid());
    EXPECT_TRUE(result.is<double>() || result.is<int>());

    // os.date should still work
    engine_->run_string("result = os.date('*t')");
    result = engine_->get_state()["result"];
    EXPECT_TRUE(result.valid());
    EXPECT_TRUE(result.is<sol::table>());
}

TEST_F(LuaEngineTest, SafeIoFunctionsStillAvailable) {
    // io.open should still work (for reading within allowed paths)
    // Note: actual file access control would be in engine-provided functions
    engine_->run_string("result = io.open");
    sol::object result = engine_->get_state()["result"];
    EXPECT_TRUE(result.valid());
    EXPECT_TRUE(result.is<sol::function>());
}

TEST_F(LuaEngineTest, MultipleScriptExecutions) {
    // First execution
    bool result1 = engine_->run_string("a = 10");
    EXPECT_TRUE(result1);

    // Second execution builds on first
    bool result2 = engine_->run_string("b = a + 5");
    EXPECT_TRUE(result2);

    int b = engine_->get_state()["b"];
    EXPECT_EQ(b, 15);
}

TEST_F(LuaEngineTest, ErrorDoesNotCrashEngine) {
    // First error
    bool result1 = engine_->run_string("error('test error 1')");
    EXPECT_FALSE(result1);

    // Engine should still work after error
    bool result2 = engine_->run_string("x = 42");
    EXPECT_TRUE(result2);

    int x = engine_->get_state()["x"];
    EXPECT_EQ(x, 42);
}

TEST_F(LuaEngineTest, FunctionDefinitionAndCall) {
    bool result = engine_->run_string(R"(
        function add(a, b)
            return a + b
        end
        result = add(10, 20)
    )");
    EXPECT_TRUE(result);

    int result_val = engine_->get_state()["result"];
    EXPECT_EQ(result_val, 30);
}

TEST_F(LuaEngineTest, TableCreationAndAccess) {
    bool result = engine_->run_string(R"(
        person = {
            name = "Test",
            age = 25
        }
    )");
    EXPECT_TRUE(result);

    sol::table person = engine_->get_state()["person"];
    std::string name = person["name"];
    int age = person["age"];

    EXPECT_EQ(name, "Test");
    EXPECT_EQ(age, 25);
}
