#include <gtest/gtest.h>
#include <fstream>
#include <openitup/chart/chart_builder.h>

#include <openitup/judge/gameplay_state.h>
#include <openitup/judge/judgment_event.h>
#include <openitup/judge/judgment_tier.h>
#include <openitup/judge/timing_profile.h>
#include <openitup/judge/timing_profile_loader.h>

using namespace openitup;

// US-JDG-014: Scoring Formula in Judge Profile

TEST(JudgeProfiles, ProfileContainsScoringWeights) {
    // Given a timing profile
    TimingProfile profile = default_timing_profile();

    // Then it contains scoring weights for all judgment types
    EXPECT_EQ(profile.score_perfect, 1000);
    EXPECT_EQ(profile.score_great, 800);
    EXPECT_EQ(profile.score_good, 500);
    EXPECT_EQ(profile.score_bad, 100);
    EXPECT_EQ(profile.score_miss, 0);
}

TEST(JudgeProfiles, GameplayStateUsesProfileScoring) {
    // Given a custom profile with different scoring
    TimingProfile profile;
    profile.name = "custom";
    profile.perfect_window_ms = 16.0;
    profile.great_window_ms = 33.0;
    profile.good_window_ms = 66.0;
    profile.bad_window_ms = 100.0;
    profile.score_perfect = 500;
    profile.score_great = 300;
    profile.score_good = 100;
    profile.score_bad = 10;
    profile.score_miss = 0;

    // When creating GameplayState with this profile
    GameplayState state(10, profile);

    // And applying a Perfect judgment
    JudgmentEvent event(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
    event = JudgmentEvent(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
    state.apply_single(event);

    // Then score increases by profile's perfect_points
    EXPECT_EQ(state.score(), 500);
}

TEST(JudgeProfiles, MaxScoreCalculationUsesProfile) {
    // Given a profile with score_perfect = 2000
    TimingProfile profile;
    profile.name = "high_score";
    profile.perfect_window_ms = 16.0;
    profile.great_window_ms = 33.0;
    profile.good_window_ms = 66.0;
    profile.bad_window_ms = 100.0;
    profile.score_perfect = 2000;
    profile.score_great = 1500;
    profile.score_good = 1000;
    profile.score_bad = 200;
    profile.score_miss = 0;

    // When creating GameplayState with 100 notes
    GameplayState state(100, profile);

    // And getting all perfects
    for (int i = 0; i < 100; ++i) {
        JudgmentEvent event(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
        event = JudgmentEvent(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
        state.apply_single(event);
    }

    // Then score percentage is 100%
    EXPECT_DOUBLE_EQ(state.score_percentage(), 100.0);
    EXPECT_EQ(state.score(), 200000);  // 100 * 2000
}

TEST(JudgeProfiles, ProfileScoringSerializesToJson) {
    // Given a profile with custom scoring
    TimingProfile profile;
    profile.name = "test";
    profile.perfect_window_ms = 16.0;
    profile.great_window_ms = 33.0;
    profile.good_window_ms = 66.0;
    profile.bad_window_ms = 100.0;
    profile.score_perfect = 1500;
    profile.score_great = 1200;
    profile.score_good = 800;
    profile.score_bad = 150;
    profile.score_miss = 5;

    // When serializing to JSON
    nlohmann::json json = profile;

    // Then scoring fields are present
    EXPECT_EQ(json["score_perfect"], 1500);
    EXPECT_EQ(json["score_great"], 1200);
    EXPECT_EQ(json["score_good"], 800);
    EXPECT_EQ(json["score_bad"], 150);
    EXPECT_EQ(json["score_miss"], 5);
}

TEST(JudgeProfiles, ProfileScoringDeserializesFromJson) {
    // Given JSON with custom scoring
    nlohmann::json json = {
        {"name", "test"},
        {"perfect_window_ms", 16.0},
        {"great_window_ms", 33.0},
        {"good_window_ms", 66.0},
        {"bad_window_ms", 100.0},
        {"score_perfect", 1500},
        {"score_great", 1200},
        {"score_good", 800},
        {"score_bad", 150},
        {"score_miss", 5}
    };

    // When deserializing
    TimingProfile profile = json.get<TimingProfile>();

    // Then scoring fields are populated
    EXPECT_EQ(profile.score_perfect, 1500);
    EXPECT_EQ(profile.score_great, 1200);
    EXPECT_EQ(profile.score_good, 800);
    EXPECT_EQ(profile.score_bad, 150);
    EXPECT_EQ(profile.score_miss, 5);
}

TEST(JudgeProfiles, ScoringFieldsOptionalInJson) {
    // Given JSON without scoring fields
    nlohmann::json json = {
        {"name", "test"},
        {"perfect_window_ms", 16.0},
        {"great_window_ms", 33.0},
        {"good_window_ms", 66.0},
        {"bad_window_ms", 100.0}
    };

    // When deserializing
    TimingProfile profile = json.get<TimingProfile>();

    // Then default scoring values are used
    EXPECT_EQ(profile.score_perfect, 1000);
    EXPECT_EQ(profile.score_great, 800);
    EXPECT_EQ(profile.score_good, 500);
    EXPECT_EQ(profile.score_bad, 100);
    EXPECT_EQ(profile.score_miss, 0);
}

// US-JDG-015: Grade Calculation from Profile

TEST(JudgeProfiles, ProfileContainsGradeThresholds) {
    // Given a timing profile
    TimingProfile profile = default_timing_profile();

    // Then it contains grade thresholds
    EXPECT_EQ(profile.grade_thresholds.size(), 7);
    EXPECT_EQ(profile.grade_thresholds["SSS"], 0.995);
    EXPECT_EQ(profile.grade_thresholds["SS"], 0.99);
    EXPECT_EQ(profile.grade_thresholds["S"], 0.95);
    EXPECT_EQ(profile.grade_thresholds["A"], 0.90);
    EXPECT_EQ(profile.grade_thresholds["B"], 0.80);
    EXPECT_EQ(profile.grade_thresholds["C"], 0.70);
    EXPECT_EQ(profile.grade_thresholds["D"], 0.60);
}

TEST(JudgeProfiles, CalculateGradeReturnsCorrectGrade) {
    // Given a profile with standard thresholds
    TimingProfile profile = default_timing_profile();

    // When calculating grades at various percentages
    // Then correct grades are returned
    EXPECT_EQ(calculate_grade(99.6, profile), "SSS");
    EXPECT_EQ(calculate_grade(99.2, profile), "SS");
    EXPECT_EQ(calculate_grade(96.0, profile), "S");
    EXPECT_EQ(calculate_grade(92.0, profile), "A");
    EXPECT_EQ(calculate_grade(85.0, profile), "B");
    EXPECT_EQ(calculate_grade(75.0, profile), "C");
    EXPECT_EQ(calculate_grade(65.0, profile), "D");
    EXPECT_EQ(calculate_grade(55.0, profile), "F");
}

TEST(JudgeProfiles, CalculateGradeHandlesRatioInput) {
    // Given a profile
    TimingProfile profile = default_timing_profile();

    // When calculating grade with ratio (0.0-1.0) input
    std::string grade = calculate_grade(0.96, profile);

    // Then correct grade is returned
    EXPECT_EQ(grade, "S");
}

TEST(JudgeProfiles, CalculateGradeHandlesPercentageInput) {
    // Given a profile
    TimingProfile profile = default_timing_profile();

    // When calculating grade with percentage (0-100) input
    std::string grade = calculate_grade(96.0, profile);

    // Then correct grade is returned
    EXPECT_EQ(grade, "S");
}

TEST(JudgeProfiles, CalculateGradeAtExactThreshold) {
    // Given a profile
    TimingProfile profile = default_timing_profile();

    // When score is exactly at threshold (95%)
    std::string grade = calculate_grade(95.0, profile);

    // Then the grade for that threshold is returned
    EXPECT_EQ(grade, "S");
}

TEST(JudgeProfiles, GameplayStateCalculatesGradeFromProfile) {
    // Given GameplayState with 100 notes
    TimingProfile profile = default_timing_profile();
    GameplayState state(100, profile);

    // When getting 96 perfects and 4 greats
    for (int i = 0; i < 96; ++i) {
        JudgmentEvent event(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
        event = JudgmentEvent(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
        state.apply_single(event);
    }
    for (int i = 0; i < 4; ++i) {
        JudgmentEvent event(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
        event = JudgmentEvent(0, 0, 0.0, JudgmentTier::GREAT, 0.0, false);
        state.apply_single(event);
    }

    // Then grade calculation uses profile thresholds
    std::string grade = state.current_grade();
    double percentage = state.score_percentage();

    // Score: 96*1000 + 4*800 = 99200/100000 = 99.2%
    EXPECT_DOUBLE_EQ(percentage, 99.2);
    EXPECT_EQ(grade, "SS");
}

TEST(JudgeProfiles, GradeThresholdsSerializeToJson) {
    // Given a profile with custom grade thresholds
    TimingProfile profile;
    profile.name = "custom";
    profile.perfect_window_ms = 16.0;
    profile.great_window_ms = 33.0;
    profile.good_window_ms = 66.0;
    profile.bad_window_ms = 100.0;
    profile.grade_thresholds = {
        {"S", 0.98},
        {"A", 0.95},
        {"B", 0.90}
    };

    // When serializing to JSON
    nlohmann::json json = profile;

    // Then grade thresholds are present
    EXPECT_EQ(json["grade_thresholds"]["S"], 0.98);
    EXPECT_EQ(json["grade_thresholds"]["A"], 0.95);
    EXPECT_EQ(json["grade_thresholds"]["B"], 0.90);
}

TEST(JudgeProfiles, GradeThresholdsDeserializeFromJson) {
    // Given JSON with custom grade thresholds
    nlohmann::json json = {
        {"name", "custom"},
        {"perfect_window_ms", 16.0},
        {"great_window_ms", 33.0},
        {"good_window_ms", 66.0},
        {"bad_window_ms", 100.0},
        {"grade_thresholds", {
            {"S", 0.98},
            {"A", 0.95},
            {"B", 0.90}
        }}
    };

    // When deserializing
    TimingProfile profile = json.get<TimingProfile>();

    // Then grade thresholds are populated
    EXPECT_EQ(profile.grade_thresholds["S"], 0.98);
    EXPECT_EQ(profile.grade_thresholds["A"], 0.95);
    EXPECT_EQ(profile.grade_thresholds["B"], 0.90);
}

// US-JDG-016: Judge Profile Per PIU Version

TEST(JudgeProfiles, BuiltInProfilesIncludeAllVersions) {
    // Given the built-in profiles
    auto profiles = built_in_profiles();

    // Then all PIU versions are present
    EXPECT_TRUE(profiles.find("exceed") != profiles.end());
    EXPECT_TRUE(profiles.find("zero") != profiles.end());
    EXPECT_TRUE(profiles.find("nx") != profiles.end());
    EXPECT_TRUE(profiles.find("nx2") != profiles.end());
    EXPECT_TRUE(profiles.find("fiesta") != profiles.end());
    EXPECT_TRUE(profiles.find("fiesta2") != profiles.end());
    EXPECT_TRUE(profiles.find("prime") != profiles.end());
}

TEST(JudgeProfiles, ExceedProfileHasCorrectTimingAndScoring) {
    // Given the exceed profile
    auto profiles = built_in_profiles();
    TimingProfile exceed = profiles["exceed"];

    // Then timing windows match Exceed era
    EXPECT_EQ(exceed.perfect_window_ms, 16.0);
    EXPECT_EQ(exceed.great_window_ms, 33.0);
    EXPECT_EQ(exceed.good_window_ms, 66.0);
    EXPECT_EQ(exceed.bad_window_ms, 100.0);

    // And scoring values are present
    EXPECT_EQ(exceed.score_perfect, 1000);
    EXPECT_EQ(exceed.score_great, 800);
    EXPECT_EQ(exceed.score_good, 500);
    EXPECT_EQ(exceed.score_bad, 100);
    EXPECT_EQ(exceed.score_miss, 0);

    // And grade thresholds are present
    EXPECT_FALSE(exceed.grade_thresholds.empty());
}

TEST(JudgeProfiles, ZeroProfileExists) {
    // Given the zero profile
    auto profiles = built_in_profiles();
    TimingProfile zero = profiles["zero"];

    // Then it has valid timing and scoring
    EXPECT_EQ(zero.name, "zero");
    EXPECT_TRUE(zero.is_valid());
    EXPECT_EQ(zero.score_perfect, 1000);
    EXPECT_FALSE(zero.grade_thresholds.empty());
}

TEST(JudgeProfiles, NxProfileHasTighterTiming) {
    // Given the nx profile
    auto profiles = built_in_profiles();
    TimingProfile nx = profiles["nx"];

    // Then timing windows are tighter than Exceed
    EXPECT_EQ(nx.perfect_window_ms, 15.0);
    EXPECT_EQ(nx.great_window_ms, 30.0);
    EXPECT_EQ(nx.good_window_ms, 60.0);
    EXPECT_EQ(nx.bad_window_ms, 90.0);

    // And has different scoring
    EXPECT_EQ(nx.score_perfect, 1000);
    EXPECT_EQ(nx.score_great, 750);
    EXPECT_EQ(nx.score_good, 450);
    EXPECT_EQ(nx.score_bad, 80);

    // And grade thresholds
    EXPECT_FALSE(nx.grade_thresholds.empty());
}

TEST(JudgeProfiles, Nx2ProfileExists) {
    // Given the nx2 profile
    auto profiles = built_in_profiles();
    TimingProfile nx2 = profiles["nx2"];

    // Then it has valid configuration
    EXPECT_EQ(nx2.name, "nx2");
    EXPECT_TRUE(nx2.is_valid());
    EXPECT_FALSE(nx2.grade_thresholds.empty());
}

TEST(JudgeProfiles, FiestaProfileHasTightestTiming) {
    // Given the fiesta profile
    auto profiles = built_in_profiles();
    TimingProfile fiesta = profiles["fiesta"];

    // Then timing windows are tightest
    EXPECT_EQ(fiesta.perfect_window_ms, 14.0);
    EXPECT_EQ(fiesta.great_window_ms, 28.0);
    EXPECT_EQ(fiesta.good_window_ms, 56.0);
    EXPECT_EQ(fiesta.bad_window_ms, 84.0);

    // And has strict scoring
    EXPECT_EQ(fiesta.score_perfect, 1000);
    EXPECT_EQ(fiesta.score_great, 700);
    EXPECT_EQ(fiesta.score_good, 400);
    EXPECT_EQ(fiesta.score_bad, 50);

    // And grade thresholds
    EXPECT_FALSE(fiesta.grade_thresholds.empty());
}

TEST(JudgeProfiles, Fiesta2ProfileExists) {
    // Given the fiesta2 profile
    auto profiles = built_in_profiles();
    TimingProfile fiesta2 = profiles["fiesta2"];

    // Then it has valid configuration
    EXPECT_EQ(fiesta2.name, "fiesta2");
    EXPECT_TRUE(fiesta2.is_valid());
    EXPECT_FALSE(fiesta2.grade_thresholds.empty());
}

TEST(JudgeProfiles, PrimeProfileExists) {
    // Given the prime profile
    auto profiles = built_in_profiles();
    TimingProfile prime = profiles["prime"];

    // Then it has valid configuration with modern values
    EXPECT_EQ(prime.name, "prime");
    EXPECT_TRUE(prime.is_valid());
    EXPECT_EQ(prime.perfect_window_ms, 14.0);
    EXPECT_FALSE(prime.grade_thresholds.empty());
}

TEST(JudgeProfiles, AllBuiltInProfilesAreValid) {
    // Given all built-in profiles
    auto profiles = built_in_profiles();

    // Then each profile is valid
    for (const auto& [name, profile] : profiles) {
        EXPECT_TRUE(profile.is_valid())
            << "Profile '" << name << "' should be valid";
        EXPECT_FALSE(profile.grade_thresholds.empty())
            << "Profile '" << name << "' should have grade thresholds";
    }
}

TEST(JudgeProfiles, ProfilesHaveUniqueCharacteristics) {
    // Given all built-in profiles
    auto profiles = built_in_profiles();

    // Then Exceed and NX have different timing
    EXPECT_NE(profiles["exceed"].perfect_window_ms,
              profiles["nx"].perfect_window_ms);

    // And NX and Fiesta have different scoring
    EXPECT_NE(profiles["nx"].score_great,
              profiles["fiesta"].score_great);
}

TEST(JudgeProfiles, DifferentProfilesProduceDifferentScores) {
    // Given two different profiles
    auto profiles = built_in_profiles();
    TimingProfile exceed = profiles["exceed"];
    TimingProfile fiesta = profiles["fiesta"];

    // When creating GameplayState with each
    GameplayState state_exceed(100, exceed);
    GameplayState state_fiesta(100, fiesta);

    // And applying same judgments (10 great)
    for (int i = 0; i < 10; ++i) {
        JudgmentEvent event(0, 0, 0.0, JudgmentTier::PERFECT, 0.0, false);
        event = JudgmentEvent(0, 0, 0.0, JudgmentTier::GREAT, 0.0, false);
        state_exceed.apply_single(event);
        state_fiesta.apply_single(event);
    }

    // Then scores are different due to different score_great values
    EXPECT_NE(state_exceed.score(), state_fiesta.score());
    EXPECT_EQ(state_exceed.score(), 10 * 800);  // Exceed: 800 per great
    EXPECT_EQ(state_fiesta.score(), 10 * 700);  // Fiesta: 700 per great
}
