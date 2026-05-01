#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <openitup/data/profile_service.h>
#include <openitup/data/settings_manager.h>
#include <openitup/data/user_data_dir.h>

using namespace openitup::data;

class ProfileServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temp directory for profiles
        temp_dir_ = std::filesystem::temp_directory_path() / "openitup_test_profiles";
        std::filesystem::create_directories(temp_dir_);

        settings_path_ = temp_dir_ / "settings.json";

        // Create SettingsManager
        settings_manager_ = std::make_unique<SettingsManager>(settings_path_);
        settings_manager_->load();

        // Create ProfileService
        profile_service_ = std::make_unique<ProfileService>(
            settings_manager_.get(), temp_dir_);
    }

    void TearDown() override {
        profile_service_.reset();
        settings_manager_.reset();
        std::filesystem::remove_all(temp_dir_);
    }

    std::filesystem::path temp_dir_;
    std::filesystem::path settings_path_;
    std::unique_ptr<SettingsManager> settings_manager_;
    std::unique_ptr<ProfileService> profile_service_;
};

// US-DAT-024 Scenario 1: Create new profile successfully
TEST_F(ProfileServiceTest, CreateNewProfile) {
    bool created = profile_service_->create_profile("Player1");

    EXPECT_TRUE(created);

    // Check that the profile file was created
    auto profile_path = temp_dir_ / "Player1.json";
    EXPECT_TRUE(std::filesystem::exists(profile_path));

    // Check that the profile is now active
    EXPECT_NE(profile_service_->active_profile(), nullptr);
    EXPECT_EQ(profile_service_->active_profile()->display_name, "Player1");

    // Check that the profile name was persisted in settings
    EXPECT_EQ(settings_manager_->get_last_active_profile(), "Player1");
}

// US-DAT-024 Scenario 2: Cannot create duplicate profile
TEST_F(ProfileServiceTest, CannotCreateDuplicateProfile) {
    profile_service_->create_profile("Player1");

    bool created_again = profile_service_->create_profile("Player1");
    EXPECT_FALSE(created_again);
}

// US-DAT-010 Scenario 1: Load existing profile
TEST_F(ProfileServiceTest, LoadExistingProfile) {
    profile_service_->create_profile("Player1");

    auto loaded = profile_service_->load_profile("Player1");

    EXPECT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->display_name, "Player1");
    EXPECT_EQ(loaded->speed_mod.type, SpeedModType::MULTIPLIER);
    EXPECT_FLOAT_EQ(loaded->speed_mod.value, 3.0f);
}

// US-DAT-010 Scenario 2: Load non-existent profile fails
TEST_F(ProfileServiceTest, LoadNonExistentProfileFails) {
    auto loaded = profile_service_->load_profile("NonExistent");

    EXPECT_FALSE(loaded.has_value());
}

// US-DAT-011: Save profile updates
TEST_F(ProfileServiceTest, SaveProfileUpdates) {
    profile_service_->create_profile("Player1");

    auto* profile = profile_service_->active_profile();
    profile->speed_mod.type = SpeedModType::CONSTANT;
    profile->speed_mod.value = 450.0f;
    profile->total_plays = 42;

    bool saved = profile_service_->save_profile(*profile);
    EXPECT_TRUE(saved);

    // Load again and verify changes
    auto loaded = profile_service_->load_profile("Player1");
    EXPECT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->speed_mod.type, SpeedModType::CONSTANT);
    EXPECT_FLOAT_EQ(loaded->speed_mod.value, 450.0f);
    EXPECT_EQ(loaded->total_plays, 42);
}

// US-DAT-012: Atomic write ensures no partial writes
TEST_F(ProfileServiceTest, AtomicWriteNoPartialWrites) {
    profile_service_->create_profile("Player1");

    auto* profile = profile_service_->active_profile();
    profile->speed_mod.value = 999.0f;

    bool saved = profile_service_->save_profile(*profile);
    EXPECT_TRUE(saved);

    // Check that no .tmp file remains
    auto temp_path = temp_dir_ / "Player1.json.tmp";
    EXPECT_FALSE(std::filesystem::exists(temp_path));
}

// US-DAT-023 Scenario 1: List profiles with multiple profiles
TEST_F(ProfileServiceTest, ListMultipleProfiles) {
    profile_service_->create_profile("Player1");
    profile_service_->create_profile("Player2");
    profile_service_->create_profile("Player3");

    auto profiles = profile_service_->list_profiles();

    EXPECT_EQ(profiles.size(), 3);
    EXPECT_NE(std::find(profiles.begin(), profiles.end(), "Player1"), profiles.end());
    EXPECT_NE(std::find(profiles.begin(), profiles.end(), "Player2"), profiles.end());
    EXPECT_NE(std::find(profiles.begin(), profiles.end(), "Player3"), profiles.end());
}

// US-DAT-023 Scenario 2: List profiles when empty directory
TEST_F(ProfileServiceTest, ListProfilesEmptyDirectory) {
    auto profiles = profile_service_->list_profiles();

    EXPECT_EQ(profiles.size(), 0);
}

// US-DAT-025 Scenario 1: Delete profile successfully
TEST_F(ProfileServiceTest, DeleteProfileSuccessfully) {
    profile_service_->create_profile("Player1");
    profile_service_->create_profile("Player2");

    // Switch to Player2 so we can delete Player1
    profile_service_->set_active_profile("Player2");

    bool deleted = profile_service_->delete_profile("Player1");
    EXPECT_TRUE(deleted);

    auto profile_path = temp_dir_ / "Player1.json";
    EXPECT_FALSE(std::filesystem::exists(profile_path));

    auto profiles = profile_service_->list_profiles();
    EXPECT_EQ(profiles.size(), 1);
    EXPECT_EQ(profiles[0], "Player2");
}

// US-DAT-025 Scenario 2: Cannot delete active profile
TEST_F(ProfileServiceTest, CannotDeleteActiveProfile) {
    profile_service_->create_profile("Player1");

    bool deleted = profile_service_->delete_profile("Player1");
    EXPECT_FALSE(deleted);

    // Profile should still exist
    auto profile_path = temp_dir_ / "Player1.json";
    EXPECT_TRUE(std::filesystem::exists(profile_path));
}

// US-DAT-026 Scenario 1: Switch active profile
TEST_F(ProfileServiceTest, SwitchActiveProfile) {
    profile_service_->create_profile("Player1");
    profile_service_->create_profile("Player2");

    bool switched = profile_service_->set_active_profile("Player2");
    EXPECT_TRUE(switched);

    EXPECT_NE(profile_service_->active_profile(), nullptr);
    EXPECT_EQ(profile_service_->active_profile()->display_name, "Player2");

    // Check that the setting was persisted
    EXPECT_EQ(settings_manager_->get_last_active_profile(), "Player2");
}

// US-DAT-026 Scenario 2: Switch to non-existent profile fails
TEST_F(ProfileServiceTest, SwitchToNonExistentProfileFails) {
    profile_service_->create_profile("Player1");

    bool switched = profile_service_->set_active_profile("NonExistent");
    EXPECT_FALSE(switched);

    // Active profile should remain unchanged
    EXPECT_NE(profile_service_->active_profile(), nullptr);
    EXPECT_EQ(profile_service_->active_profile()->display_name, "Player1");
}

// US-DAT-027 Scenario 1: Active profile persists across sessions
TEST_F(ProfileServiceTest, ActiveProfilePersistsAcrossSessions) {
    profile_service_->create_profile("Player1");
    profile_service_->create_profile("Player2");
    profile_service_->set_active_profile("Player2");

    // Simulate restart: create new ProfileService
    auto new_settings_manager = std::make_unique<SettingsManager>(settings_path_);
    new_settings_manager->load();

    auto new_profile_service = std::make_unique<ProfileService>(
        new_settings_manager.get(), temp_dir_);

    // Load the persisted active profile
    std::string last_active = new_settings_manager->get_last_active_profile();
    EXPECT_EQ(last_active, "Player2");

    bool loaded = new_profile_service->set_active_profile(last_active);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(new_profile_service->active_profile()->display_name, "Player2");
}

// US-DAT-027 Scenario 2: No active profile at first launch
TEST_F(ProfileServiceTest, NoActiveProfileAtFirstLaunch) {
    std::string last_active = settings_manager_->get_last_active_profile();
    EXPECT_EQ(last_active, "");

    EXPECT_EQ(profile_service_->active_profile(), nullptr);
}

// US-DAT-015 Scenario 1: Record high score after gameplay
TEST_F(ProfileServiceTest, RecordHighScoreAfterGameplay) {
    profile_service_->create_profile("Player1");

    HighScoreEntry entry;
    entry.score = 950000;
    entry.grade = "S";
    entry.max_combo = 500;
    entry.date = "2026-04-30T12:00:00Z";
    entry.judge_profile = "exceed";

    std::string chart_hash = "abc123def456";

    bool recorded = profile_service_->record_high_score(chart_hash, entry);
    EXPECT_TRUE(recorded);

    // Check that the score was added to the profile
    auto* profile = profile_service_->active_profile();
    EXPECT_EQ(profile->high_scores[chart_hash].size(), 1);
    EXPECT_EQ(profile->high_scores[chart_hash][0].score, 950000);
}

// US-DAT-015 Scenario 2: Cannot record high score without active profile
TEST_F(ProfileServiceTest, CannotRecordHighScoreWithoutActiveProfile) {
    HighScoreEntry entry;
    entry.score = 950000;

    bool recorded = profile_service_->record_high_score("abc123", entry);
    EXPECT_FALSE(recorded);
}

// US-DAT-016 Scenario 1: High score entry includes metadata
TEST_F(ProfileServiceTest, HighScoreEntryMetadata) {
    profile_service_->create_profile("Player1");

    HighScoreEntry entry;
    entry.score = 950000;
    entry.grade = "S";
    entry.max_combo = 500;
    entry.judgments.perfect = 480;
    entry.judgments.great = 20;
    entry.judgments.good = 0;
    entry.judgments.bad = 0;
    entry.judgments.miss = 0;
    entry.date = "2026-04-30T12:00:00Z";
    entry.judge_profile = "exceed";

    std::string chart_hash = "abc123def456";
    profile_service_->record_high_score(chart_hash, entry);

    auto scores = profile_service_->get_high_scores(chart_hash, 1);
    EXPECT_EQ(scores.size(), 1);
    EXPECT_EQ(scores[0].score, 950000);
    EXPECT_EQ(scores[0].grade, "S");
    EXPECT_EQ(scores[0].max_combo, 500);
    EXPECT_EQ(scores[0].judgments.perfect, 480);
    EXPECT_EQ(scores[0].date, "2026-04-30T12:00:00Z");
    EXPECT_EQ(scores[0].judge_profile, "exceed");
}

// US-DAT-017 Scenario 1: Query top N high scores
TEST_F(ProfileServiceTest, QueryTopHighScores) {
    profile_service_->create_profile("Player1");

    std::string chart_hash = "abc123def456";

    // Add 5 scores
    for (int i = 0; i < 5; i++) {
        HighScoreEntry entry;
        entry.score = 900000 + (i * 10000);
        entry.grade = "A";
        entry.max_combo = 400 + i;
        entry.date = "2026-04-30T12:00:00Z";
        entry.judge_profile = "exceed";
        profile_service_->record_high_score(chart_hash, entry);
    }

    // Query top 3 scores
    auto scores = profile_service_->get_high_scores(chart_hash, 3);

    EXPECT_EQ(scores.size(), 3);
    EXPECT_EQ(scores[0].score, 940000);  // Highest
    EXPECT_EQ(scores[1].score, 930000);
    EXPECT_EQ(scores[2].score, 920000);
}

// US-DAT-017 Scenario 2: No high scores for chart returns empty
TEST_F(ProfileServiceTest, NoHighScoresReturnsEmpty) {
    profile_service_->create_profile("Player1");

    auto scores = profile_service_->get_high_scores("nonexistent_chart", 10);

    EXPECT_EQ(scores.size(), 0);
}

// US-DAT-015: Keep only top 10 high scores
TEST_F(ProfileServiceTest, KeepOnlyTop10HighScores) {
    profile_service_->create_profile("Player1");

    std::string chart_hash = "abc123def456";

    // Add 15 scores
    for (int i = 0; i < 15; i++) {
        HighScoreEntry entry;
        entry.score = 900000 + (i * 1000);
        entry.grade = "A";
        entry.max_combo = 400;
        entry.date = "2026-04-30T12:00:00Z";
        entry.judge_profile = "exceed";
        profile_service_->record_high_score(chart_hash, entry);
    }

    auto* profile = profile_service_->active_profile();
    EXPECT_EQ(profile->high_scores[chart_hash].size(), 10);

    // Check that the lowest scores were removed
    EXPECT_EQ(profile->high_scores[chart_hash][0].score, 914000);  // Highest
    EXPECT_EQ(profile->high_scores[chart_hash][9].score, 905000);  // 10th highest
}

// US-DAT-015: Scores are sorted by score descending
TEST_F(ProfileServiceTest, ScoresSortedDescending) {
    profile_service_->create_profile("Player1");

    std::string chart_hash = "abc123def456";

    // Add scores in random order
    std::vector<int> scores_to_add = {920000, 950000, 900000, 940000, 910000};

    for (int score : scores_to_add) {
        HighScoreEntry entry;
        entry.score = score;
        entry.grade = "A";
        entry.max_combo = 400;
        entry.date = "2026-04-30T12:00:00Z";
        entry.judge_profile = "exceed";
        profile_service_->record_high_score(chart_hash, entry);
    }

    auto scores = profile_service_->get_high_scores(chart_hash, 10);

    EXPECT_EQ(scores.size(), 5);
    EXPECT_EQ(scores[0].score, 950000);
    EXPECT_EQ(scores[1].score, 940000);
    EXPECT_EQ(scores[2].score, 920000);
    EXPECT_EQ(scores[3].score, 910000);
    EXPECT_EQ(scores[4].score, 900000);
}

// US-DAT-013: ProfileService provides complete API
TEST_F(ProfileServiceTest, ProfileServiceAPIComplete) {
    // This test verifies the ProfileService interface is complete

    // Create profile
    EXPECT_TRUE(profile_service_->create_profile("Player1"));

    // List profiles
    auto profiles = profile_service_->list_profiles();
    EXPECT_EQ(profiles.size(), 1);

    // Load profile
    auto loaded = profile_service_->load_profile("Player1");
    EXPECT_TRUE(loaded.has_value());

    // Get active profile
    EXPECT_NE(profile_service_->active_profile(), nullptr);

    // Save profile
    auto* profile = profile_service_->active_profile();
    profile->total_plays = 1;
    EXPECT_TRUE(profile_service_->save_profile(*profile));

    // Record high score
    HighScoreEntry entry;
    entry.score = 950000;
    entry.grade = "S";
    entry.max_combo = 500;
    entry.date = "2026-04-30T12:00:00Z";
    entry.judge_profile = "exceed";
    EXPECT_TRUE(profile_service_->record_high_score("chart123", entry));

    // Get high scores
    auto scores = profile_service_->get_high_scores("chart123", 10);
    EXPECT_EQ(scores.size(), 1);

    // Create another profile
    EXPECT_TRUE(profile_service_->create_profile("Player2"));

    // Switch active profile
    EXPECT_TRUE(profile_service_->set_active_profile("Player1"));

    // Delete profile
    EXPECT_TRUE(profile_service_->delete_profile("Player2"));
}
