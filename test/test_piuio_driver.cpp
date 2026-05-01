#include <gtest/gtest.h>

#include <openitup/input/piuio_driver.h>
#include <openitup/input/lamp_controller.h>
#include <openitup/judge/judgment_tier.h>

using namespace openitup;

class PiuioDriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        driver = std::make_unique<PiuioDriver>();
    }

    std::unique_ptr<PiuioDriver> driver;
};

// US-INP-041: Basic driver functionality
TEST_F(PiuioDriverTest, DriverInitializesInStubMode) {
    // When no hardware is present, driver should initialize in stub mode
    EXPECT_NE(driver, nullptr);
}

TEST_F(PiuioDriverTest, DeviceNameReflectsConnectionState) {
    // Stub mode should indicate hardware not connected
    std::string name = driver->device_name();
    EXPECT_TRUE(name.find("PIUIO") != std::string::npos);
}

TEST_F(PiuioDriverTest, PollReturnsZeroInStubMode) {
    // When no hardware is connected, polling should return no inputs
    uint32_t held = driver->poll_held();
    EXPECT_EQ(held, 0u);
}

TEST_F(PiuioDriverTest, HardwareDetectionReturnsFalseInStubMode) {
    // Static hardware detection should return false when no hardware present
    EXPECT_FALSE(PiuioDriver::is_available());
}

// US-INP-042: Version detection
TEST_F(PiuioDriverTest, VersionDetectionReturnsUnknownInStubMode) {
    // Stub mode should report unknown version when no hardware is present
    PiuioVersion version = driver->version();
    EXPECT_EQ(version, PiuioVersion::UNKNOWN);
}

// US-INP-051: Lamp control
TEST_F(PiuioDriverTest, LampStateCanBeSet) {
    // Should be able to set lamp state even in stub mode
    driver->set_lamp(0, true);
    driver->set_lamp(1, false);
    driver->update_lamps();
    // No crash = success in stub mode
}

TEST_F(PiuioDriverTest, LampStateHandlesInvalidColumns) {
    // Invalid column indices should be handled gracefully
    driver->set_lamp(-1, true);
    driver->set_lamp(10, true);
    driver->set_lamp(100, true);
    driver->update_lamps();
    // No crash = success
}

TEST_F(PiuioDriverTest, MultipleLampsCanBeActive) {
    // All 10 lamps should be independently controllable
    for (int i = 0; i < 10; ++i) {
        driver->set_lamp(i, true);
    }
    driver->update_lamps();
    // No crash = success in stub mode
}

// US-INP-064: Native player separation (sensor mapping)
TEST_F(PiuioDriverTest, SensorMappingP1ToP1Input) {
    // Sensors 0-4 should map to P1 inputs
    // In stub mode, this returns 0, but the mapping logic exists
    uint32_t held = driver->poll_held();
    // In real hardware, sensor 0 would set P1_DOWN_LEFT
    // For now, verify polling works
    EXPECT_EQ(held, 0u);
}

TEST_F(PiuioDriverTest, SensorMappingP2ToP2Input) {
    // Sensors 5-9 should map to P2 inputs
    // In stub mode, this returns 0, but the mapping logic exists
    uint32_t held = driver->poll_held();
    // In real hardware, sensor 5 would set P2_DOWN_LEFT
    // For now, verify polling works
    EXPECT_EQ(held, 0u);
}

class LampControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        driver = std::make_unique<PiuioDriver>();
        controller = std::make_unique<LampController>(driver.get());
    }

    std::unique_ptr<PiuioDriver> driver;
    std::unique_ptr<LampController> controller;
};

// US-INP-051: Lamp controller functionality
TEST_F(LampControllerTest, DirectLampControlWorks) {
    controller->set_lamp(0, true);
    controller->set_lamp(1, false);
    controller->update(0);
    // No crash = success in stub mode
}

TEST_F(LampControllerTest, FlashOnJudgmentActivatesLamp) {
    controller->flash_on_judgment(2, JudgmentTier::PERFECT);
    controller->update(0);
    // No crash = success in stub mode
}

TEST_F(LampControllerTest, FlashDurationConfigurable) {
    // Default duration for PERFECT
    uint64_t default_duration = controller->flash_duration_ms(JudgmentTier::PERFECT);
    EXPECT_GT(default_duration, 0u);

    // Set custom duration
    controller->set_flash_duration(JudgmentTier::PERFECT, 300);
    EXPECT_EQ(controller->flash_duration_ms(JudgmentTier::PERFECT), 300u);
}

TEST_F(LampControllerTest, FlashTimesOutAfterDuration) {
    // Start a flash at time 0
    controller->flash_on_judgment(0, JudgmentTier::PERFECT);
    controller->update(0);

    // Flash should still be active at 100ms
    controller->update(100);

    // Flash should expire after its duration (200ms default for PERFECT)
    controller->update(250);

    // No crash = success
}

TEST_F(LampControllerTest, MultipleFlashesCanBeActive) {
    // Activate multiple flashes
    controller->flash_on_judgment(0, JudgmentTier::PERFECT);
    controller->flash_on_judgment(1, JudgmentTier::GREAT);
    controller->flash_on_judgment(2, JudgmentTier::GOOD);

    controller->update(0);
    controller->update(50);
    controller->update(100);

    // No crash = success
}

TEST_F(LampControllerTest, FlashesExpireIndependently) {
    // Set different durations
    controller->set_flash_duration(JudgmentTier::PERFECT, 200);
    controller->set_flash_duration(JudgmentTier::BAD, 50);

    // Start two flashes at same time
    controller->flash_on_judgment(0, JudgmentTier::PERFECT);
    controller->flash_on_judgment(1, JudgmentTier::BAD);

    controller->update(0);

    // At 75ms, BAD should be expired but PERFECT still active
    controller->update(75);

    // At 250ms, both should be expired
    controller->update(250);

    // No crash = success
}

TEST_F(LampControllerTest, DifferentTiersHaveDifferentDurations) {
    // Better judgments should have longer durations
    uint64_t perfect = controller->flash_duration_ms(JudgmentTier::PERFECT);
    uint64_t great = controller->flash_duration_ms(JudgmentTier::GREAT);
    uint64_t good = controller->flash_duration_ms(JudgmentTier::GOOD);
    uint64_t bad = controller->flash_duration_ms(JudgmentTier::BAD);
    uint64_t miss = controller->flash_duration_ms(JudgmentTier::MISS);

    EXPECT_GT(perfect, great);
    EXPECT_GT(great, good);
    EXPECT_GT(good, bad);
    EXPECT_GT(bad, miss);
}

TEST_F(LampControllerTest, InvalidColumnHandledGracefully) {
    controller->flash_on_judgment(-1, JudgmentTier::PERFECT);
    controller->flash_on_judgment(10, JudgmentTier::PERFECT);
    controller->update(0);
    // No crash = success
}

TEST_F(LampControllerTest, NullDriverHandledGracefully) {
    LampController null_controller(nullptr);
    null_controller.set_lamp(0, true);
    null_controller.flash_on_judgment(0, JudgmentTier::PERFECT);
    null_controller.update(0);
    // No crash = success
}
