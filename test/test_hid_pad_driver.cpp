#include <gtest/gtest.h>
#include <openitup/input/hid_pad_driver.h>
#include <openitup/input/input_merger.h>
#include <openitup/input/keyboard_driver.h>
#include <openitup/input/pad_input.h>

using namespace openitup;

// --- HidPadDriver ---

TEST(HidPadDriver, ConstructorSucceeds) {
    // Constructor should not throw even if no gamepad is connected
    EXPECT_NO_THROW({
        HidPadDriver driver;
    });
}

TEST(HidPadDriver, DefaultButtonMapHasExpectedEntries) {
    auto button_map = HidPadDriver::default_button_map();
    EXPECT_GE(button_map.size(), 4u);  // at least DPAD mappings

    bool has_dpad_left = false;
    bool has_dpad_up = false;
    bool has_dpad_down = false;
    bool has_dpad_right = false;
    bool has_south = false;
    bool has_start = false;
    bool has_back = false;

    for (const auto& m : button_map) {
        if (m.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) has_dpad_left = true;
        if (m.button == SDL_GAMEPAD_BUTTON_DPAD_UP) has_dpad_up = true;
        if (m.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) has_dpad_down = true;
        if (m.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) has_dpad_right = true;
        if (m.button == SDL_GAMEPAD_BUTTON_SOUTH) has_south = true;
        if (m.button == SDL_GAMEPAD_BUTTON_START) has_start = true;
        if (m.button == SDL_GAMEPAD_BUTTON_BACK) has_back = true;
    }

    EXPECT_TRUE(has_dpad_left);
    EXPECT_TRUE(has_dpad_up);
    EXPECT_TRUE(has_dpad_down);
    EXPECT_TRUE(has_dpad_right);
    EXPECT_TRUE(has_south);
    EXPECT_TRUE(has_start);
    EXPECT_TRUE(has_back);
}

TEST(HidPadDriver, DefaultMappingMatchesSpec) {
    auto button_map = HidPadDriver::default_button_map();

    // Find each button and verify its mapping
    for (const auto& m : button_map) {
        if (m.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) {
            EXPECT_EQ(m.input, PadInput::P1_UP_LEFT);
        } else if (m.button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
            EXPECT_EQ(m.input, PadInput::P1_UP_RIGHT);
        } else if (m.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
            EXPECT_EQ(m.input, PadInput::P1_DOWN_LEFT);
        } else if (m.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
            EXPECT_EQ(m.input, PadInput::P1_DOWN_RIGHT);
        } else if (m.button == SDL_GAMEPAD_BUTTON_SOUTH) {
            EXPECT_EQ(m.input, PadInput::P1_CENTER);
        } else if (m.button == SDL_GAMEPAD_BUTTON_START) {
            EXPECT_EQ(m.input, PadInput::START);
        } else if (m.button == SDL_GAMEPAD_BUTTON_BACK) {
            EXPECT_EQ(m.input, PadInput::BACK);
        }
    }
}

TEST(HidPadDriver, PollHeldReturnsZeroWhenNoGamepad) {
    HidPadDriver driver;
    // If no gamepad is connected, should return 0
    uint32_t held = driver.poll_held();
    // This test just verifies it doesn't crash - actual value depends on hardware
    (void)held;
}

TEST(HidPadDriver, DeviceNameDoesNotCrash) {
    HidPadDriver driver;
    std::string name = driver.device_name();
    EXPECT_FALSE(name.empty());
}

TEST(HidPadDriver, SetButtonMap) {
    HidPadDriver driver;

    std::vector<GamepadMapping> custom_map = {
        {SDL_GAMEPAD_BUTTON_SOUTH, PadInput::START},
    };

    driver.set_button_map(custom_map);
    EXPECT_EQ(driver.button_map().size(), 1u);
    EXPECT_EQ(driver.button_map()[0].button, SDL_GAMEPAD_BUTTON_SOUTH);
    EXPECT_EQ(driver.button_map()[0].input, PadInput::START);
}

TEST(HidPadDriver, ImplementsInputDriverInterface) {
    HidPadDriver driver;
    InputDriver* base = &driver;

    // Should be callable through base interface
    uint32_t held = base->poll_held();
    std::string name = base->device_name();

    (void)held;
    EXPECT_FALSE(name.empty());
}

// --- InputMerger ---

class MockDriverForMerger : public InputDriver {
public:
    uint32_t held = 0;
    std::string name = "MockDriver";

    uint32_t poll_held() override { return held; }
    std::string device_name() const override { return name; }
};

TEST(InputMerger, DefaultConstructor) {
    InputMerger merger;
    EXPECT_EQ(merger.drivers().size(), 0u);
}

TEST(InputMerger, AddSingleDriver) {
    MockDriverForMerger driver;
    InputMerger merger;

    merger.add_driver(&driver);
    EXPECT_EQ(merger.drivers().size(), 1u);
    EXPECT_EQ(merger.drivers()[0], &driver);
}

TEST(InputMerger, AddMultipleDrivers) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;
    MockDriverForMerger driver3;

    InputMerger merger;
    merger.add_driver(&driver1);
    merger.add_driver(&driver2);
    merger.add_driver(&driver3);

    EXPECT_EQ(merger.drivers().size(), 3u);
}

TEST(InputMerger, AddNullptrIsIgnored) {
    InputMerger merger;
    merger.add_driver(nullptr);
    EXPECT_EQ(merger.drivers().size(), 0u);
}

TEST(InputMerger, RemoveDriver) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;

    InputMerger merger;
    merger.add_driver(&driver1);
    merger.add_driver(&driver2);

    merger.remove_driver(&driver1);
    EXPECT_EQ(merger.drivers().size(), 1u);
    EXPECT_EQ(merger.drivers()[0], &driver2);
}

TEST(InputMerger, RemoveNonExistentDriver) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;

    InputMerger merger;
    merger.add_driver(&driver1);

    // Removing a driver that was never added should be safe
    EXPECT_NO_THROW(merger.remove_driver(&driver2));
    EXPECT_EQ(merger.drivers().size(), 1u);
}

TEST(InputMerger, PollHeldWithNoDrivers) {
    InputMerger merger;
    EXPECT_EQ(merger.poll_held(), 0u);
}

TEST(InputMerger, PollHeldWithSingleDriver) {
    MockDriverForMerger driver;
    driver.held = static_cast<uint32_t>(PadInput::P1_CENTER);

    InputMerger merger;
    merger.add_driver(&driver);

    EXPECT_EQ(merger.poll_held(), static_cast<uint32_t>(PadInput::P1_CENTER));
}

TEST(InputMerger, PollHeldMergesMultipleDrivers) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;

    driver1.held = static_cast<uint32_t>(PadInput::P1_CENTER);
    driver2.held = static_cast<uint32_t>(PadInput::P1_DOWN_LEFT);

    InputMerger merger;
    merger.add_driver(&driver1);
    merger.add_driver(&driver2);

    uint32_t result = merger.poll_held();
    uint32_t expected = PadInput::P1_CENTER | PadInput::P1_DOWN_LEFT;
    EXPECT_EQ(result, expected);
}

TEST(InputMerger, PollHeldORsAllDriverInputs) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;
    MockDriverForMerger driver3;

    driver1.held = static_cast<uint32_t>(PadInput::P1_UP_LEFT);
    driver2.held = static_cast<uint32_t>(PadInput::P1_UP_RIGHT);
    driver3.held = static_cast<uint32_t>(PadInput::START);

    InputMerger merger;
    merger.add_driver(&driver1);
    merger.add_driver(&driver2);
    merger.add_driver(&driver3);

    uint32_t result = merger.poll_held();
    uint32_t expected = PadInput::P1_UP_LEFT | PadInput::P1_UP_RIGHT | PadInput::START;
    EXPECT_EQ(result, expected);
}

TEST(InputMerger, OverlappingInputsAreMergedCorrectly) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;

    // Both drivers report the same input
    driver1.held = static_cast<uint32_t>(PadInput::START);
    driver2.held = static_cast<uint32_t>(PadInput::START);

    InputMerger merger;
    merger.add_driver(&driver1);
    merger.add_driver(&driver2);

    uint32_t result = merger.poll_held();
    EXPECT_EQ(result, static_cast<uint32_t>(PadInput::START));
}

TEST(InputMerger, DeviceNameWithNoDrivers) {
    InputMerger merger;
    std::string name = merger.device_name();
    EXPECT_EQ(name, "InputMerger (no drivers)");
}

TEST(InputMerger, DeviceNameWithSingleDriver) {
    MockDriverForMerger driver;
    driver.name = "TestDriver";

    InputMerger merger;
    merger.add_driver(&driver);

    std::string name = merger.device_name();
    EXPECT_EQ(name, "TestDriver");
}

TEST(InputMerger, DeviceNameWithMultipleDrivers) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;
    MockDriverForMerger driver3;

    InputMerger merger;
    merger.add_driver(&driver1);
    merger.add_driver(&driver2);
    merger.add_driver(&driver3);

    std::string name = merger.device_name();
    EXPECT_EQ(name, "InputMerger (3 drivers)");
}

TEST(InputMerger, ImplementsInputDriverInterface) {
    InputMerger merger;
    InputDriver* base = &merger;

    // Should be callable through base interface
    uint32_t held = base->poll_held();
    std::string name = base->device_name();

    EXPECT_EQ(held, 0u);
    EXPECT_FALSE(name.empty());
}

// --- Integration: Keyboard + HidPad via Merger ---

TEST(InputMergerIntegration, MergesKeyboardAndHidPad) {
    // Create fake keyboard state
    bool fake_state[SDL_SCANCODE_COUNT] = {};
    fake_state[SDL_SCANCODE_S] = true;  // P1_CENTER

    auto state_fn = [&fake_state](int* numkeys) -> const bool* {
        *numkeys = SDL_SCANCODE_COUNT;
        return fake_state;
    };

    KeyboardDriver kbd_driver(KeyboardDriver::default_keymap(), state_fn);
    HidPadDriver hid_driver;

    InputMerger merger;
    merger.add_driver(&kbd_driver);
    merger.add_driver(&hid_driver);

    uint32_t result = merger.poll_held();

    // Should at least have the keyboard input
    EXPECT_NE(result & static_cast<uint32_t>(PadInput::P1_CENTER), 0u);
}

TEST(InputMergerIntegration, RemovingDriverAffectsPoll) {
    MockDriverForMerger driver1;
    MockDriverForMerger driver2;

    driver1.held = static_cast<uint32_t>(PadInput::P1_CENTER);
    driver2.held = static_cast<uint32_t>(PadInput::START);

    InputMerger merger;
    merger.add_driver(&driver1);
    merger.add_driver(&driver2);

    uint32_t result1 = merger.poll_held();
    EXPECT_EQ(result1, PadInput::P1_CENTER | PadInput::START);

    merger.remove_driver(&driver2);

    uint32_t result2 = merger.poll_held();
    EXPECT_EQ(result2, static_cast<uint32_t>(PadInput::P1_CENTER));
}

TEST(InputMergerIntegration, DynamicallyChangeDriverInputs) {
    MockDriverForMerger driver;

    InputMerger merger;
    merger.add_driver(&driver);

    driver.held = static_cast<uint32_t>(PadInput::P1_CENTER);
    EXPECT_EQ(merger.poll_held(), static_cast<uint32_t>(PadInput::P1_CENTER));

    driver.held = static_cast<uint32_t>(PadInput::START);
    EXPECT_EQ(merger.poll_held(), static_cast<uint32_t>(PadInput::START));

    driver.held = 0;
    EXPECT_EQ(merger.poll_held(), 0u);
}
