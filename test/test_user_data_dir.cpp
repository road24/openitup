#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <openitup/data/user_data_dir.h>

namespace {

// RAII guard for environment variables
class ScopedEnvVar {
public:
    ScopedEnvVar(const char* name, const char* value)
        : name_(name) {
        // Save original value
        const char* orig = std::getenv(name);
        if (orig) {
            original_value_ = orig;
            had_value_ = true;
        } else {
            had_value_ = false;
        }

        // Set new value
#ifdef _WIN32
        _putenv_s(name, value);
#else
        setenv(name, value, 1);
#endif
    }

    ~ScopedEnvVar() {
        // Restore original value
        if (had_value_) {
#ifdef _WIN32
            _putenv_s(name_.c_str(), original_value_.c_str());
#else
            setenv(name_.c_str(), original_value_.c_str(), 1);
#endif
        } else {
#ifdef _WIN32
            _putenv_s(name_.c_str(), "");
#else
            unsetenv(name_.c_str());
#endif
        }
    }

private:
    std::string name_;
    std::string original_value_;
    bool had_value_;
};

} // anonymous namespace

class UserDataDirTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temp directory for testing
        temp_dir_ = std::filesystem::temp_directory_path() / "test_openitup_XXXXXX";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        // Clean up temp directory
        if (std::filesystem::exists(temp_dir_)) {
            std::filesystem::remove_all(temp_dir_);
        }
    }

    std::filesystem::path temp_dir_;
};

TEST_F(UserDataDirTest, InjectablePathOverride) {
    openitup::data::UserDataDir udd(temp_dir_);
    EXPECT_EQ(udd.path(), temp_dir_);
}

TEST_F(UserDataDirTest, PathIsAbsolute) {
    openitup::data::UserDataDir udd(temp_dir_);
    EXPECT_TRUE(udd.path().is_absolute());
}

TEST_F(UserDataDirTest, CreatesMissingDirectory) {
    auto test_path = temp_dir_ / "new_dir";
    ASSERT_FALSE(std::filesystem::exists(test_path));

    openitup::data::UserDataDir udd(test_path);
    EXPECT_TRUE(udd.ensure_directories());
    EXPECT_TRUE(std::filesystem::exists(test_path));
}

TEST_F(UserDataDirTest, CreatesProfilesSubdir) {
    openitup::data::UserDataDir udd(temp_dir_);
    EXPECT_TRUE(udd.ensure_directories());

    auto profiles = udd.profiles_dir();
    EXPECT_TRUE(std::filesystem::exists(profiles));
    EXPECT_TRUE(std::filesystem::is_directory(profiles));
}

TEST_F(UserDataDirTest, CreatesCacheSubdir) {
    openitup::data::UserDataDir udd(temp_dir_);
    EXPECT_TRUE(udd.ensure_directories());

    auto cache = udd.cache_dir();
    EXPECT_TRUE(std::filesystem::exists(cache));
    EXPECT_TRUE(std::filesystem::is_directory(cache));
}

TEST_F(UserDataDirTest, SettingsFilePath) {
    openitup::data::UserDataDir udd(temp_dir_);
    auto settings = udd.settings_file();
    EXPECT_EQ(settings, temp_dir_ / "settings.json");
}

#ifndef _WIN32
TEST_F(UserDataDirTest, ResolvesLinuxPath) {
    ScopedEnvVar home("HOME", "/home/testuser");
    ScopedEnvVar xdg("XDG_DATA_HOME", "");  // Unset XDG_DATA_HOME

    openitup::data::UserDataDir udd;
    auto path = udd.path();

    EXPECT_TRUE(path.string().find("/home/testuser/.local/share/openitup") != std::string::npos);
    EXPECT_TRUE(path.is_absolute());
}

TEST_F(UserDataDirTest, RespectsXDGDataHome) {
    ScopedEnvVar xdg("XDG_DATA_HOME", "/custom/data");

    openitup::data::UserDataDir udd;
    auto path = udd.path();

    EXPECT_TRUE(path.string().find("/custom/data/openitup") != std::string::npos);
}
#endif

#ifdef _WIN32
TEST_F(UserDataDirTest, ResolvesWindowsPath) {
    ScopedEnvVar appdata("APPDATA", "C:\\Users\\TestUser\\AppData\\Roaming");

    openitup::data::UserDataDir udd;
    auto path = udd.path();

    EXPECT_TRUE(path.string().find("AppData\\Roaming\\openitup") != std::string::npos);
    EXPECT_TRUE(path.is_absolute());
}
#endif

TEST_F(UserDataDirTest, ReadOnlyFallback) {
    // Create a directory and remove write permissions
    auto readonly_dir = temp_dir_ / "readonly";
    std::filesystem::create_directories(readonly_dir);
    std::filesystem::permissions(readonly_dir,
                                  std::filesystem::perms::owner_read |
                                  std::filesystem::perms::owner_exec,
                                  std::filesystem::perm_options::replace);

    auto test_path = readonly_dir / "subdir";
    openitup::data::UserDataDir udd(test_path);

    // This should fail and return false
    EXPECT_FALSE(udd.ensure_directories());

    // Restore permissions for cleanup
    std::filesystem::permissions(readonly_dir,
                                  std::filesystem::perms::owner_all,
                                  std::filesystem::perm_options::replace);
}
