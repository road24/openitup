#include <gtest/gtest.h>

#include <openitup/chart/timing_data.h>

#include <chrono>
#include <cmath>

using namespace openitup;

// Helper for floating-point comparison
constexpr double EPSILON = 0.0001;

// --- Single BPM Tests ---

TEST(TimingData, SingleBpmTimeAtBeat) {
    // 120 BPM: time_at_beat(4.0) == 2.0
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0}
    };
    TimingData timing(std::move(events));

    EXPECT_DOUBLE_EQ(timing.time_at_beat(4.0), 2.0);
}

TEST(TimingData, SingleBpmBeatAtTime) {
    // 120 BPM: beat_at_time(2.0) == 4.0
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0}
    };
    TimingData timing(std::move(events));

    EXPECT_DOUBLE_EQ(timing.beat_at_time(2.0), 4.0);
}

// --- BPM Change Tests ---

TEST(TimingData, BpmChangeTimeAtBeat) {
    // 120->180 at beat 8: time_at_beat(12.0) ≈ 5.333
    // Time at beat 8: 8 * 0.5 = 4.0
    // Time from beat 8 to 12 at 180 BPM: 4 * (60/180) = 4 * 0.333... = 1.333...
    // Total: 4.0 + 1.333... = 5.333...
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing(std::move(events));

    double expected = 4.0 + 4.0 * (60.0 / 180.0);
    EXPECT_NEAR(timing.time_at_beat(12.0), expected, EPSILON);
}

TEST(TimingData, BpmChangeBeatAtTime) {
    // 120->180: beat_at_time(5.333) ≈ 12.0
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing(std::move(events));

    double time = 4.0 + 4.0 * (60.0 / 180.0);
    EXPECT_NEAR(timing.beat_at_time(time), 12.0, EPSILON);
}

// --- Stop Tests ---

TEST(TimingData, StopExtendsTime) {
    // 120 BPM + 1s stop at beat 4
    // time_at_beat(5.0):
    //   - Time to beat 4: 4 * 0.5 = 2.0
    //   - Stop duration: 1.0
    //   - Time from beat 4 to 5: 1 * 0.5 = 0.5
    //   - Total: 2.0 + 1.0 + 0.5 = 3.5
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {4.0, TimingEventType::STOP, 0.0, 1.0}
    };
    TimingData timing(std::move(events));

    EXPECT_NEAR(timing.time_at_beat(5.0), 3.5, EPSILON);
}

TEST(TimingData, StopBeatAtTime) {
    // During stop, beat doesn't advance
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {4.0, TimingEventType::STOP, 0.0, 1.0}
    };
    TimingData timing(std::move(events));

    // Time during the stop (2.0 to 3.0) should return beat 4.0
    EXPECT_NEAR(timing.beat_at_time(2.5), 4.0, EPSILON);
}

TEST(TimingData, BeatAtTimeAfterStop) {
    // Time after stop returns correct beat
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {4.0, TimingEventType::STOP, 0.0, 1.0}
    };
    TimingData timing(std::move(events));

    // At time 3.5: stop ends at 3.0, then 0.5 seconds at 120 BPM = 1 beat
    // Beat: 4.0 + 1.0 = 5.0
    EXPECT_NEAR(timing.beat_at_time(3.5), 5.0, EPSILON);
}

// --- Multiple BPM Changes ---

TEST(TimingData, MultipleBpmChanges) {
    // 120 at beat 0, 150 at beat 4, 180 at beat 8
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {4.0, TimingEventType::BPM_CHANGE, 150.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing(std::move(events));

    // Time at beat 4: 4 * 0.5 = 2.0
    EXPECT_NEAR(timing.time_at_beat(4.0), 2.0, EPSILON);

    // Time at beat 8: 2.0 + 4 * (60/150) = 2.0 + 1.6 = 3.6
    EXPECT_NEAR(timing.time_at_beat(8.0), 3.6, EPSILON);

    // Time at beat 12: 3.6 + 4 * (60/180) = 3.6 + 1.333... = 4.933...
    double expected = 3.6 + 4.0 * (60.0 / 180.0);
    EXPECT_NEAR(timing.time_at_beat(12.0), expected, EPSILON);
}

// --- Multiple Stops ---

TEST(TimingData, MultipleStops) {
    // 120 BPM + 0.5s stop at beat 2 + 1.0s stop at beat 6
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {2.0, TimingEventType::STOP, 0.0, 0.5},
        {6.0, TimingEventType::STOP, 0.0, 1.0}
    };
    TimingData timing(std::move(events));

    // Time at beat 8:
    //   - Beat 0 to 2: 2 * 0.5 = 1.0
    //   - Stop 1: 0.5
    //   - Beat 2 to 6: 4 * 0.5 = 2.0
    //   - Stop 2: 1.0
    //   - Beat 6 to 8: 2 * 0.5 = 1.0
    //   - Total: 1.0 + 0.5 + 2.0 + 1.0 + 1.0 = 5.5
    EXPECT_NEAR(timing.time_at_beat(8.0), 5.5, EPSILON);
}

// --- Boundary Tests ---

TEST(TimingData, ZeroBeat) {
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0}
    };
    TimingData timing(std::move(events));

    EXPECT_DOUBLE_EQ(timing.time_at_beat(0.0), 0.0);
}

TEST(TimingData, NegativeBeat) {
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0}
    };
    TimingData timing(std::move(events));

    EXPECT_DOUBLE_EQ(timing.time_at_beat(-1.0), 0.0);
}

// --- BPM Query Tests ---

TEST(TimingData, BpmAtBeat) {
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {4.0, TimingEventType::BPM_CHANGE, 150.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing(std::move(events));

    EXPECT_NEAR(timing.bpm_at_beat(0.0), 120.0, EPSILON);
    EXPECT_NEAR(timing.bpm_at_beat(3.9), 120.0, EPSILON);
    EXPECT_NEAR(timing.bpm_at_beat(4.0), 150.0, EPSILON);
    EXPECT_NEAR(timing.bpm_at_beat(7.9), 150.0, EPSILON);
    EXPECT_NEAR(timing.bpm_at_beat(8.0), 180.0, EPSILON);
    EXPECT_NEAR(timing.bpm_at_beat(12.0), 180.0, EPSILON);
}

// --- Round Trip Tests ---

TEST(TimingData, RoundTrip) {
    std::vector<TimingEvent> events = {
        {0.0, TimingEventType::BPM_CHANGE, 120.0, 0.0},
        {8.0, TimingEventType::BPM_CHANGE, 180.0, 0.0}
    };
    TimingData timing(std::move(events));

    // Round trip: time_at_beat(beat_at_time(t)) ≈ t
    for (double t = 0.0; t <= 10.0; t += 0.5) {
        double beat = timing.beat_at_time(t);
        double time_back = timing.time_at_beat(beat);
        EXPECT_NEAR(time_back, t, EPSILON);
    }
}

// --- Default Constructor ---

TEST(TimingData, DefaultTimingDataIs120Bpm) {
    TimingData timing;

    EXPECT_EQ(timing.size(), 1);
    EXPECT_FALSE(timing.empty());
    EXPECT_NEAR(timing.bpm_at_beat(0.0), 120.0, EPSILON);
    EXPECT_DOUBLE_EQ(timing.time_at_beat(4.0), 2.0);
}

// --- Performance Test ---

TEST(TimingData, PerformanceWith100Events) {
    // 100 BPM changes, 10k queries in < 10ms
    std::vector<TimingEvent> events;
    for (int i = 0; i < 100; ++i) {
        double beat = i * 4.0;
        double bpm = 120.0 + (i % 3) * 30.0;  // 120, 150, 180 pattern
        events.push_back({beat, TimingEventType::BPM_CHANGE, bpm, 0.0});
    }
    TimingData timing(std::move(events));

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        double beat = (i % 400) * 1.0;
        volatile double time = timing.time_at_beat(beat);
        (void)time;  // prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 10);
}
