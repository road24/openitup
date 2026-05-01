#include <gtest/gtest.h>

#include <openitup/judge/timing_profile.h>
#include <openitup/judge/timing_profile_loader.h>

#include <fstream>
#include <nlohmann/json.hpp>

using namespace openitup;

// --- JSON Serialization Tests ---

TEST(TimingProfileLoader, ToJsonSerializesAllFields) {
    // Given a TimingProfile with all fields populated
    TimingProfile profile{"exceed", 16.0, 33.0, 66.0, 100.0};

    // When serializing to JSON
    nlohmann::json json = profile;

    // Then all fields are present
    EXPECT_EQ(json["name"], "exceed");
    EXPECT_EQ(json["perfect_window_ms"], 16.0);
    EXPECT_EQ(json["great_window_ms"], 33.0);
    EXPECT_EQ(json["good_window_ms"], 66.0);
    EXPECT_EQ(json["bad_window_ms"], 100.0);
}

TEST(TimingProfileLoader, FromJsonDeserializesAllFields) {
    // Given a JSON object with all required fields
    nlohmann::json json = {
        {"name", "nx"},
        {"perfect_window_ms", 15.0},
        {"great_window_ms", 30.0},
        {"good_window_ms", 60.0},
        {"bad_window_ms", 90.0}
    };

    // When deserializing from JSON
    TimingProfile profile = json.get<TimingProfile>();

    // Then all fields are populated correctly
    EXPECT_EQ(profile.name, "nx");
    EXPECT_EQ(profile.perfect_window_ms, 15.0);
    EXPECT_EQ(profile.great_window_ms, 30.0);
    EXPECT_EQ(profile.good_window_ms, 60.0);
    EXPECT_EQ(profile.bad_window_ms, 90.0);
}

TEST(TimingProfileLoader, JsonRoundTripPreservesData) {
    // Given a TimingProfile
    TimingProfile original{"fiesta", 14.0, 28.0, 56.0, 84.0};

    // When serializing and deserializing
    nlohmann::json json = original;
    TimingProfile roundtrip = json.get<TimingProfile>();

    // Then the data is preserved
    EXPECT_EQ(roundtrip.name, original.name);
    EXPECT_EQ(roundtrip.perfect_window_ms, original.perfect_window_ms);
    EXPECT_EQ(roundtrip.great_window_ms, original.great_window_ms);
    EXPECT_EQ(roundtrip.good_window_ms, original.good_window_ms);
    EXPECT_EQ(roundtrip.bad_window_ms, original.bad_window_ms);
}

// --- Load from JSON Object Tests ---

TEST(TimingProfileLoader, LoadFromValidJson) {
    // Given a valid JSON object
    nlohmann::json json = {
        {"name", "test"},
        {"perfect_window_ms", 16.0},
        {"great_window_ms", 33.0},
        {"good_window_ms", 66.0},
        {"bad_window_ms", 100.0}
    };

    // When loading the profile
    auto result = load_timing_profile(json);

    // Then it succeeds
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "test");
    EXPECT_EQ(result->perfect_window_ms, 16.0);
}

TEST(TimingProfileLoader, LoadRejectsInvalidWindows) {
    // Given a JSON with perfect_window > great_window (invalid ordering)
    nlohmann::json json = {
        {"name", "invalid"},
        {"perfect_window_ms", 50.0},
        {"great_window_ms", 40.0},
        {"good_window_ms", 66.0},
        {"bad_window_ms", 100.0}
    };

    // When loading the profile
    auto result = load_timing_profile(json);

    // Then it fails validation
    EXPECT_FALSE(result.has_value());
}

TEST(TimingProfileLoader, LoadRejectsNegativeWindows) {
    // Given a JSON with negative window value
    nlohmann::json json = {
        {"name", "negative"},
        {"perfect_window_ms", 16.0},
        {"great_window_ms", 33.0},
        {"good_window_ms", 66.0},
        {"bad_window_ms", -10.0}
    };

    // When loading the profile
    auto result = load_timing_profile(json);

    // Then it fails validation
    EXPECT_FALSE(result.has_value());
}

TEST(TimingProfileLoader, LoadRejectsMissingField) {
    // Given a JSON missing the "name" field
    nlohmann::json json = {
        {"perfect_window_ms", 16.0},
        {"great_window_ms", 33.0},
        {"good_window_ms", 66.0},
        {"bad_window_ms", 100.0}
    };

    // When loading the profile
    auto result = load_timing_profile(json);

    // Then it fails
    EXPECT_FALSE(result.has_value());
}

// --- Load from File Tests ---

TEST(TimingProfileLoader, LoadFromValidFile) {
    // Given a valid JSON file
    const std::string test_file = "/tmp/test_profile.json";
    {
        std::ofstream file(test_file);
        nlohmann::json json = {
            {"name", "file_test"},
            {"perfect_window_ms", 16.0},
            {"great_window_ms", 33.0},
            {"good_window_ms", 66.0},
            {"bad_window_ms", 100.0}
        };
        file << json.dump();
    }

    // When loading the profile from file
    auto result = load_timing_profile(test_file);

    // Then it succeeds
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "file_test");
    EXPECT_EQ(result->perfect_window_ms, 16.0);
}

TEST(TimingProfileLoader, LoadFromNonexistentFile) {
    // Given a nonexistent file path
    const std::string test_file = "/tmp/nonexistent_profile_xyz123.json";

    // When loading the profile
    auto result = load_timing_profile(test_file);

    // Then it fails
    EXPECT_FALSE(result.has_value());
}

TEST(TimingProfileLoader, LoadFromInvalidJsonFile) {
    // Given a file with invalid JSON syntax
    const std::string test_file = "/tmp/invalid_profile.json";
    {
        std::ofstream file(test_file);
        file << "{ this is not valid json }";
    }

    // When loading the profile
    auto result = load_timing_profile(test_file);

    // Then it fails
    EXPECT_FALSE(result.has_value());
}

// --- Built-in Profiles Tests ---

TEST(TimingProfileLoader, BuiltInProfilesIncludeExceed) {
    // Given the built-in profiles
    auto profiles = built_in_profiles();

    // When checking for "exceed"
    auto it = profiles.find("exceed");

    // Then it exists with correct values
    ASSERT_NE(it, profiles.end());
    EXPECT_EQ(it->second.name, "exceed");
    EXPECT_EQ(it->second.perfect_window_ms, 16.0);
    EXPECT_EQ(it->second.great_window_ms, 33.0);
    EXPECT_EQ(it->second.good_window_ms, 66.0);
    EXPECT_EQ(it->second.bad_window_ms, 100.0);
}

TEST(TimingProfileLoader, BuiltInProfilesIncludeNx) {
    // Given the built-in profiles
    auto profiles = built_in_profiles();

    // When checking for "nx"
    auto it = profiles.find("nx");

    // Then it exists with tighter timing
    ASSERT_NE(it, profiles.end());
    EXPECT_EQ(it->second.name, "nx");
    EXPECT_EQ(it->second.perfect_window_ms, 15.0);
    EXPECT_EQ(it->second.great_window_ms, 30.0);
    EXPECT_EQ(it->second.good_window_ms, 60.0);
    EXPECT_EQ(it->second.bad_window_ms, 90.0);
}

TEST(TimingProfileLoader, BuiltInProfilesIncludeFiesta) {
    // Given the built-in profiles
    auto profiles = built_in_profiles();

    // When checking for "fiesta"
    auto it = profiles.find("fiesta");

    // Then it exists with tightest timing
    ASSERT_NE(it, profiles.end());
    EXPECT_EQ(it->second.name, "fiesta");
    EXPECT_EQ(it->second.perfect_window_ms, 14.0);
    EXPECT_EQ(it->second.great_window_ms, 28.0);
    EXPECT_EQ(it->second.good_window_ms, 56.0);
    EXPECT_EQ(it->second.bad_window_ms, 84.0);
}

TEST(TimingProfileLoader, AllBuiltInProfilesAreValid) {
    // Given the built-in profiles
    auto profiles = built_in_profiles();

    // When checking each profile
    // Then all are valid
    for (const auto& [name, profile] : profiles) {
        EXPECT_TRUE(profile.is_valid()) << "Profile '" << name << "' should be valid";
    }
}

TEST(TimingProfileLoader, DefaultTimingProfileMatchesExceed) {
    // Given the default timing profile
    auto default_profile = default_timing_profile();

    // When comparing to the exceed built-in profile
    auto profiles = built_in_profiles();
    auto exceed = profiles["exceed"];

    // Then they match
    EXPECT_EQ(default_profile.name, exceed.name);
    EXPECT_EQ(default_profile.perfect_window_ms, exceed.perfect_window_ms);
    EXPECT_EQ(default_profile.great_window_ms, exceed.great_window_ms);
    EXPECT_EQ(default_profile.good_window_ms, exceed.good_window_ms);
    EXPECT_EQ(default_profile.bad_window_ms, exceed.bad_window_ms);
}
