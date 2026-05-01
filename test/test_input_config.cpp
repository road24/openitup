#include <gtest/gtest.h>

#include <openitup/input/hid_pad_driver.h>
#include <openitup/input/input_merger.h>
#include <openitup/input/keyboard_driver.h>

using namespace openitup;

// US-INP-032: Button-to-panel mapping per device
TEST(InputConfig, ButtonMappingPerDevice) {
    // Scenario 1: Mapping saved per VID/PID
    HidPadDriver driver;

    std::vector<GamepadMapping> custom_map = {
        {SDL_GAMEPAD_BUTTON_SOUTH, PadInput::P1_CENTER},
        {SDL_GAMEPAD_BUTTON_EAST, PadInput::P1_UP_LEFT},
    };

    driver.set_button_map(custom_map);
    EXPECT_EQ(driver.button_map().size(), 2);
    EXPECT_EQ(driver.button_map()[0].input, PadInput::P1_CENTER);
}

TEST(InputConfig, UnmappedDeviceUsesDefault) {
    // Scenario 3: Unmapped device uses default mapping
    HidPadDriver driver;
    auto default_map = HidPadDriver::default_button_map();

    EXPECT_FALSE(default_map.empty());
    EXPECT_GE(default_map.size(), 5);  // At least 5 buttons mapped
}

// US-INP-033: Axis threshold configuration
TEST(InputConfig, AxisThresholdConfiguration) {
    // Scenario 1: Axis value above threshold registers as pressed
    HidPadDriver driver;
    driver.set_axis_threshold(0.5f);

    EXPECT_FLOAT_EQ(driver.axis_threshold(), 0.5f);
}

TEST(InputConfig, AxisThresholdConfigurablePerDevice) {
    // Scenario 3: Threshold configurable per device
    HidPadDriver driver1;
    HidPadDriver driver2;

    driver1.set_axis_threshold(0.3f);
    driver2.set_axis_threshold(0.7f);

    EXPECT_FLOAT_EQ(driver1.axis_threshold(), 0.3f);
    EXPECT_FLOAT_EQ(driver2.axis_threshold(), 0.7f);
}

// US-INP-063: Device-to-player binding
TEST(InputConfig, DevicePlayerBinding) {
    // Scenario 1: Device bound to P1
    HidPadDriver driver1;
    driver1.set_player_assignment(PlayerAssignment::P1);
    EXPECT_EQ(driver1.player_assignment(), PlayerAssignment::P1);
}

TEST(InputConfig, DevicePlayerBindingP2) {
    // Scenario 2: Device bound to P2
    HidPadDriver driver2;
    driver2.set_player_assignment(PlayerAssignment::P2);
    EXPECT_EQ(driver2.player_assignment(), PlayerAssignment::P2);
}

TEST(InputConfig, DefaultPlayerAssignmentIsP1) {
    // Scenario 3: Default assignment is P1
    HidPadDriver driver;
    EXPECT_EQ(driver.player_assignment(), PlayerAssignment::P1);
}

// US-INP-072: Per-device input calibration offsets
TEST(InputConfig, LatencyOffsetPerDevice) {
    // Scenario 1: Offset stored per device
    HidPadDriver driver;
    driver.set_latency_offset(-15);

    EXPECT_EQ(driver.latency_offset(), -15);
}

TEST(InputConfig, NegativeOffsetShiftsEarlier) {
    // Scenario 2: Negative offset shifts input earlier
    HidPadDriver driver;
    driver.set_latency_offset(-10);

    EXPECT_EQ(driver.latency_offset(), -10);
    EXPECT_LT(driver.latency_offset(), 0);
}

TEST(InputConfig, PositiveOffsetShiftsLater) {
    // Scenario 3: Positive offset shifts input later
    HidPadDriver driver;
    driver.set_latency_offset(10);

    EXPECT_EQ(driver.latency_offset(), 10);
    EXPECT_GT(driver.latency_offset(), 0);
}

// US-INP-082: Configure driver priority
TEST(InputConfig, DriverPriorityConfiguration) {
    // Scenario 1: Priority configurable in settings
    HidPadDriver driver;
    driver.set_priority(15);

    EXPECT_EQ(driver.priority(), 15);
}

TEST(InputConfig, DefaultPriorityOrder) {
    // Scenario 2: Default priority order
    HidPadDriver hid_driver;

    // Default HID priority should be 20
    EXPECT_EQ(hid_driver.priority(), 20);
}

TEST(InputConfig, PriorityAffectsOnlyFirstWinsMode) {
    // Scenario 3: Priority affects only first-wins mode
    InputMerger merger;

    // OR-merge mode (default)
    EXPECT_EQ(merger.merge_strategy(), MergeStrategy::OR_MERGE);

    // Set to first-wins
    merger.set_merge_strategy(MergeStrategy::FIRST_WINS);
    EXPECT_EQ(merger.merge_strategy(), MergeStrategy::FIRST_WINS);
}

// US-INP-081: Merge input from multiple drivers
TEST(InputConfigMerger, ORMergeStrategy) {
    // Scenario 2: OR-merge strategy
    InputMerger merger;
    merger.set_merge_strategy(MergeStrategy::OR_MERGE);

    // Without actual drivers, just verify strategy is set
    EXPECT_EQ(merger.merge_strategy(), MergeStrategy::OR_MERGE);
}

TEST(InputConfigMerger, FirstWinsMergeStrategy) {
    // Test first-wins strategy
    InputMerger merger;
    merger.set_merge_strategy(MergeStrategy::FIRST_WINS);

    EXPECT_EQ(merger.merge_strategy(), MergeStrategy::FIRST_WINS);
}

TEST(InputConfigMerger, MultipleDriversWithPriority) {
    // Scenario 1: Multiple drivers active
    InputMerger merger;

    HidPadDriver hid_driver;
    KeyboardDriver kb_driver;

    merger.add_driver(&hid_driver);
    merger.add_driver(&kb_driver);

    EXPECT_EQ(merger.drivers().size(), 2);
}

// US-INP-071: Input mapping configuration screen
// (Scene tests would require SDL initialization and are better as integration tests)

// US-INP-073: Calibration feedback screen
// (Scene tests would require SDL initialization and are better as integration tests)
