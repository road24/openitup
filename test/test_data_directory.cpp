#include <openitup/asset/data_directory.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

namespace {

// Helper to create a unique temporary directory for testing.
std::filesystem::path create_temp_test_dir(const std::string& suffix) {
    auto temp_dir = std::filesystem::temp_directory_path() / ("openitup_test_" + suffix);
    std::filesystem::create_directories(temp_dir);
    return temp_dir;
}

// Helper to create a file in a directory.
void create_file(const std::filesystem::path& dir, const std::string& filename) {
    std::ofstream f(dir / filename);
    f << "test content\n";
}

// Scoped environment variable helper for tests.
// Sets an environment variable on construction, restores the original value on destruction.
struct ScopedEnvVar {
    const char* name_;
    std::string original_;
    bool had_original_;

    ScopedEnvVar(const char* name, const char* value)
        : name_(name) {
        const char* orig = std::getenv(name);
        had_original_ = (orig != nullptr);
        if (had_original_) original_ = orig;
        setenv(name, value, 1);
    }
    ~ScopedEnvVar() {
        if (had_original_) setenv(name_, original_.c_str(), 1);
        else unsetenv(name_);
    }
};

} // namespace

TEST(DataDirectory, ValidDirectoryPasses) {
    auto temp_dir = create_temp_test_dir("valid");
    openitup::DataDirectory dd(temp_dir);
    EXPECT_TRUE(dd.validate());
    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, NonexistentDirectoryFails) {
    auto nonexistent = std::filesystem::temp_directory_path() / "openitup_test_nonexistent_12345";
    openitup::DataDirectory dd(nonexistent);
    EXPECT_FALSE(dd.validate());
}

TEST(DataDirectory, PathResolvedToAbsolute) {
    auto temp_dir = create_temp_test_dir("absolute");
    // Create a relative path by getting the basename
    auto basename = temp_dir.filename();
    auto parent = temp_dir.parent_path();

    // Change to parent directory temporarily (in a scoped manner)
    auto original_cwd = std::filesystem::current_path();
    std::filesystem::current_path(parent);

    openitup::DataDirectory dd(basename);
    EXPECT_TRUE(dd.path().is_absolute());

    // Restore original cwd and cleanup
    std::filesystem::current_path(original_cwd);
    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, FindFileByExtensionFound) {
    auto temp_dir = create_temp_test_dir("find_ext");
    create_file(temp_dir, "test.ksf");

    openitup::DataDirectory dd(temp_dir);
    auto result = dd.find_file_by_extension(".ksf");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->filename(), "test.ksf");

    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, FindFileByExtensionMissing) {
    auto temp_dir = create_temp_test_dir("find_ext_missing");

    openitup::DataDirectory dd(temp_dir);
    auto result = dd.find_file_by_extension(".ksf");
    EXPECT_FALSE(result.has_value());

    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, FindFileByExtensionCaseInsensitive) {
    auto temp_dir = create_temp_test_dir("find_ext_case");
    create_file(temp_dir, "test.KSF");

    openitup::DataDirectory dd(temp_dir);
    auto result = dd.find_file_by_extension(".ksf");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->filename(), "test.KSF");

    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, FindFileCiFound) {
    auto temp_dir = create_temp_test_dir("find_ci");
    create_file(temp_dir, "SONG.ogg");

    openitup::DataDirectory dd(temp_dir);
    auto result = dd.find_file_ci("song.ogg");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->filename(), "SONG.ogg");

    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, FindFileCiMissing) {
    auto temp_dir = create_temp_test_dir("find_ci_missing");

    openitup::DataDirectory dd(temp_dir);
    auto result = dd.find_file_ci("missing.ogg");
    EXPECT_FALSE(result.has_value());

    std::filesystem::remove_all(temp_dir);
}

// --- resolve_data_directory tests ---

TEST(DataDirectory, ResolveCliPathUsed) {
    auto temp_dir = create_temp_test_dir("resolve_cli");
    auto result = openitup::resolve_data_directory(temp_dir.string());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->path(), std::filesystem::absolute(temp_dir));
    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, ResolveEnvFallback) {
    auto temp_dir = create_temp_test_dir("resolve_env");
    ScopedEnvVar env("OPENITUP_DATA_DIR", temp_dir.string().c_str());

    auto result = openitup::resolve_data_directory("");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->path(), std::filesystem::absolute(temp_dir));

    std::filesystem::remove_all(temp_dir);
}

TEST(DataDirectory, ResolveCliOverridesEnv) {
    auto cli_dir = create_temp_test_dir("resolve_cli_override");
    auto env_dir = create_temp_test_dir("resolve_env_override");

    ScopedEnvVar env("OPENITUP_DATA_DIR", env_dir.string().c_str());

    auto result = openitup::resolve_data_directory(cli_dir.string());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->path(), std::filesystem::absolute(cli_dir));

    std::filesystem::remove_all(cli_dir);
    std::filesystem::remove_all(env_dir);
}

TEST(DataDirectory, ResolveNeitherReturnsNullopt) {
    // Make sure OPENITUP_DATA_DIR is not set
    ScopedEnvVar env("OPENITUP_DATA_DIR", "");
    unsetenv("OPENITUP_DATA_DIR");

    auto result = openitup::resolve_data_directory("");
    EXPECT_FALSE(result.has_value());
}

TEST(DataDirectory, ResolveEnvEmptyStringReturnsNullopt) {
    ScopedEnvVar env("OPENITUP_DATA_DIR", "");

    auto result = openitup::resolve_data_directory("");
    EXPECT_FALSE(result.has_value());
}
