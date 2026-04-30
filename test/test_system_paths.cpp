#include <gtest/gtest.h>
#include <openitup/core/system_paths.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>

using namespace openitup::core;
namespace fs = std::filesystem;

class SystemPaths : public ::testing::Test {
protected:
    fs::path tmp_dir_;
    fs::path system_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "openitup_test_syspaths";
        fs::create_directories(tmp_dir_);
        system_dir_ = tmp_dir_ / "data" / "system";
        fs::create_directories(system_dir_);

        // Create a marker file to verify it's a valid system directory
        std::ofstream(system_dir_ / ".marker").put('\0');
    }

    void TearDown() override {
        // Clear environment variable
        unsetenv("OPENITUP_SYSTEM_DIR");
        fs::remove_all(tmp_dir_);
    }
};

TEST_F(SystemPaths, CliOverrideTakesPrecedence) {
    auto result = find_system_dir(system_dir_, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(fs::canonical(*result), fs::canonical(system_dir_));
}

TEST_F(SystemPaths, EnvironmentVariableFallback) {
    setenv("OPENITUP_SYSTEM_DIR", system_dir_.string().c_str(), 1);
    auto result = find_system_dir({}, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(fs::canonical(*result), fs::canonical(system_dir_));
}

TEST_F(SystemPaths, CwdRelativeSearch) {
    // Save current directory
    auto original_cwd = fs::current_path();

    // Change to tmp_dir so ./data/system exists
    fs::current_path(tmp_dir_);

    auto result = find_system_dir({}, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(fs::canonical(*result), fs::canonical(system_dir_));

    // Restore original directory
    fs::current_path(original_cwd);
}

TEST_F(SystemPaths, BinaryRelativeSearch) {
    // Create a fake binary directory structure:
    // tmp_dir/bin/openitup
    // tmp_dir/data/system/
    auto bin_dir = tmp_dir_ / "bin";
    fs::create_directories(bin_dir);
    auto binary_path = bin_dir / "openitup";

    // The system directory should be at ../data/system relative to bin/
    auto result = find_system_dir({}, binary_path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(fs::canonical(*result), fs::canonical(system_dir_));
}

TEST_F(SystemPaths, ReturnsNulloptWhenNotFound) {
    auto nonexistent = tmp_dir_ / "nonexistent";
    auto result = find_system_dir(nonexistent, nonexistent / "bin" / "openitup");
    EXPECT_FALSE(result.has_value());
}

TEST_F(SystemPaths, EmptyCliOverrideDoesNotBlock) {
    setenv("OPENITUP_SYSTEM_DIR", system_dir_.string().c_str(), 1);
    auto result = find_system_dir({}, {});  // Empty CLI override
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(fs::canonical(*result), fs::canonical(system_dir_));
}

TEST_F(SystemPaths, NonexistentCliOverrideFallsBackToEnv) {
    setenv("OPENITUP_SYSTEM_DIR", system_dir_.string().c_str(), 1);
    auto nonexistent = tmp_dir_ / "nonexistent";
    auto result = find_system_dir(nonexistent, {});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(fs::canonical(*result), fs::canonical(system_dir_));
}

TEST_F(SystemPaths, PathIsCanonicalized) {
    // Create a symlink to system_dir
    auto symlink_path = tmp_dir_ / "symlink_to_system";
    if (fs::exists(symlink_path)) {
        fs::remove(symlink_path);
    }
    fs::create_directory_symlink(system_dir_, symlink_path);

    auto result = find_system_dir(symlink_path, {});
    ASSERT_TRUE(result.has_value());
    // Result should be the canonical path, not the symlink
    EXPECT_EQ(*result, fs::canonical(system_dir_));
}
