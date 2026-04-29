#include <gtest/gtest.h>
#include <openitup/input/pad_input.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/input_system.h>
#include <openitup/input/keyboard_driver.h>
#include <cmath>
#include <set>

using namespace openitup;

// --- PadInput ---

TEST(PadInput, AllValuesUniquePowerOfTwo) {
    std::set<uint32_t> values;
    for (auto input : ALL_PAD_INPUTS) {
        auto val = static_cast<uint32_t>(input);
        EXPECT_TRUE((val & (val - 1)) == 0) << "Not power of 2: " << val;
        EXPECT_TRUE(values.insert(val).second) << "Duplicate: " << val;
    }
}

TEST(PadInput, CorrectCount) {
    EXPECT_EQ(PAD_INPUT_COUNT, 14);
    EXPECT_EQ(sizeof(ALL_PAD_INPUTS) / sizeof(ALL_PAD_INPUTS[0]), 14u);
}

TEST(PadInput, BitwiseOrCombines) {
    uint32_t mask = PadInput::P1_CENTER | PadInput::P1_DOWN_LEFT;
    EXPECT_EQ(mask, 0x0005u);
}

TEST(PadInput, P1MaskExtraction) {
    uint32_t mask = PadInput::P1_CENTER | PadInput::P2_CENTER | PadInput::START;
    uint32_t p1_only = mask & 0x001F;
    EXPECT_EQ(p1_only, static_cast<uint32_t>(PadInput::P1_CENTER));
}

TEST(PadInput, StringConversion) {
    EXPECT_STREQ(pad_input_to_string(PadInput::P1_CENTER), "P1_CENTER");
    EXPECT_STREQ(pad_input_to_string(PadInput::START), "START");
}

// --- InputSnapshot ---

TEST(InputSnapshot, DefaultIsEmpty) {
    InputSnapshot snap;
    EXPECT_TRUE(snap.empty());
    EXPECT_EQ(snap.held_mask(), 0u);
    EXPECT_EQ(snap.pressed_mask(), 0u);
    EXPECT_EQ(snap.released_mask(), 0u);
}

TEST(InputSnapshot, HeldQuery) {
    uint32_t held = PadInput::P1_CENTER | PadInput::P1_DOWN_LEFT;
    InputSnapshot snap(held, 0, 0, 1);
    EXPECT_TRUE(snap.is_held(PadInput::P1_CENTER));
    EXPECT_TRUE(snap.is_held(PadInput::P1_DOWN_LEFT));
    EXPECT_FALSE(snap.is_held(PadInput::P1_UP_LEFT));
}

TEST(InputSnapshot, PressedQuery) {
    uint32_t pressed = static_cast<uint32_t>(PadInput::START);
    InputSnapshot snap(pressed, pressed, 0, 5);
    EXPECT_TRUE(snap.is_pressed(PadInput::START));
    EXPECT_FALSE(snap.is_pressed(PadInput::BACK));
}

TEST(InputSnapshot, ReleasedQuery) {
    uint32_t released = static_cast<uint32_t>(PadInput::P1_CENTER);
    InputSnapshot snap(0, 0, released, 10);
    EXPECT_TRUE(snap.is_released(PadInput::P1_CENTER));
    EXPECT_FALSE(snap.is_released(PadInput::P1_DOWN_LEFT));
}

TEST(InputSnapshot, TickNumber) {
    InputSnapshot snap(0, 0, 0, 42);
    EXPECT_EQ(snap.tick_number(), 42u);
}

// --- InputSystem edge detection ---

class MockDriver : public InputDriver {
public:
    uint32_t held = 0;
    uint32_t poll_held() override { return held; }
    std::string device_name() const override { return "Mock"; }
};

TEST(InputSystem, EdgeDetectionPress) {
    auto driver = std::make_unique<MockDriver>();
    auto* raw = driver.get();
    InputSystem sys(std::move(driver));

    raw->held = 0;
    sys.poll(0);
    EXPECT_EQ(sys.snapshot().pressed_mask(), 0u);

    raw->held = static_cast<uint32_t>(PadInput::P1_CENTER);
    sys.poll(1);
    EXPECT_TRUE(sys.snapshot().is_pressed(PadInput::P1_CENTER));
    EXPECT_TRUE(sys.snapshot().is_held(PadInput::P1_CENTER));
}

TEST(InputSystem, EdgeDetectionRelease) {
    auto driver = std::make_unique<MockDriver>();
    auto* raw = driver.get();
    InputSystem sys(std::move(driver));

    raw->held = static_cast<uint32_t>(PadInput::P1_CENTER);
    sys.poll(0);

    raw->held = 0;
    sys.poll(1);
    EXPECT_TRUE(sys.snapshot().is_released(PadInput::P1_CENTER));
    EXPECT_FALSE(sys.snapshot().is_held(PadInput::P1_CENTER));
}

TEST(InputSystem, HoldDoesNotRepress) {
    auto driver = std::make_unique<MockDriver>();
    auto* raw = driver.get();
    InputSystem sys(std::move(driver));

    raw->held = static_cast<uint32_t>(PadInput::P1_CENTER);
    sys.poll(0);

    sys.poll(1);
    EXPECT_TRUE(sys.snapshot().is_held(PadInput::P1_CENTER));
    EXPECT_FALSE(sys.snapshot().is_pressed(PadInput::P1_CENTER));
}

TEST(InputSystem, MultipleSimultaneousInputs) {
    auto driver = std::make_unique<MockDriver>();
    auto* raw = driver.get();
    InputSystem sys(std::move(driver));

    raw->held = 0;
    sys.poll(0);

    raw->held = PadInput::P1_CENTER | PadInput::P1_DOWN_LEFT | PadInput::P1_UP_RIGHT;
    sys.poll(1);
    EXPECT_TRUE(sys.snapshot().is_pressed(PadInput::P1_CENTER));
    EXPECT_TRUE(sys.snapshot().is_pressed(PadInput::P1_DOWN_LEFT));
    EXPECT_TRUE(sys.snapshot().is_pressed(PadInput::P1_UP_RIGHT));
    EXPECT_EQ(sys.snapshot().tick_number(), 1u);
}

TEST(InputSystem, PreviousSnapshotAvailable) {
    auto driver = std::make_unique<MockDriver>();
    auto* raw = driver.get();
    InputSystem sys(std::move(driver));

    raw->held = static_cast<uint32_t>(PadInput::P1_CENTER);
    sys.poll(0);

    raw->held = 0;
    sys.poll(1);

    EXPECT_TRUE(sys.previous_snapshot().is_held(PadInput::P1_CENTER));
    EXPECT_FALSE(sys.snapshot().is_held(PadInput::P1_CENTER));
}

// --- KeyboardDriver ---

TEST(KeyboardDriver, DefaultKeymapHasExpectedEntries) {
    auto keymap = KeyboardDriver::default_keymap();
    EXPECT_GE(keymap.size(), 5u);  // at least 5 P1 panel keys

    bool has_center = false;
    for (const auto& m : keymap) {
        if (m.input == PadInput::P1_CENTER) has_center = true;
    }
    EXPECT_TRUE(has_center);
}

TEST(KeyboardDriver, PollWithFakeKeyboardState) {
    bool fake_state[SDL_SCANCODE_COUNT] = {};

    auto state_fn = [&fake_state](int* numkeys) -> const bool* {
        *numkeys = SDL_SCANCODE_COUNT;
        return fake_state;
    };

    KeyboardDriver driver(KeyboardDriver::default_keymap(), state_fn);

    // No keys pressed
    EXPECT_EQ(driver.poll_held(), 0u);

    // Press S key (mapped to P1_CENTER in default keymap)
    fake_state[SDL_SCANCODE_S] = true;
    uint32_t held = driver.poll_held();
    EXPECT_NE(held & static_cast<uint32_t>(PadInput::P1_CENTER), 0u);

    // Press Q key simultaneously (P1_UP_LEFT)
    fake_state[SDL_SCANCODE_Q] = true;
    held = driver.poll_held();
    EXPECT_NE(held & static_cast<uint32_t>(PadInput::P1_CENTER), 0u);
    EXPECT_NE(held & static_cast<uint32_t>(PadInput::P1_UP_LEFT), 0u);
}

TEST(KeyboardDriver, UnmappedKeyIgnored) {
    bool fake_state[SDL_SCANCODE_COUNT] = {};
    fake_state[SDL_SCANCODE_F12] = true;

    auto state_fn = [&fake_state](int* numkeys) -> const bool* {
        *numkeys = SDL_SCANCODE_COUNT;
        return fake_state;
    };

    KeyboardDriver driver(KeyboardDriver::default_keymap(), state_fn);
    EXPECT_EQ(driver.poll_held(), 0u);
}

TEST(KeyboardDriver, NKeyRollover) {
    bool fake_state[SDL_SCANCODE_COUNT] = {};

    auto state_fn = [&fake_state](int* numkeys) -> const bool* {
        *numkeys = SDL_SCANCODE_COUNT;
        return fake_state;
    };

    auto keymap = KeyboardDriver::default_keymap();
    KeyboardDriver driver(keymap, state_fn);

    // Press all mapped keys simultaneously
    for (const auto& m : keymap) {
        fake_state[m.scancode] = true;
    }

    uint32_t held = driver.poll_held();
    for (const auto& m : keymap) {
        EXPECT_NE(held & static_cast<uint32_t>(m.input), 0u)
            << "Missing: " << pad_input_to_string(m.input);
    }
}

// --- End-to-end: Driver → System → Snapshot ---

TEST(InputEndToEnd, FakeKeyboardThroughSystem) {
    bool fake_state[SDL_SCANCODE_COUNT] = {};

    auto state_fn = [&fake_state](int* numkeys) -> const bool* {
        *numkeys = SDL_SCANCODE_COUNT;
        return fake_state;
    };

    auto driver = std::make_unique<KeyboardDriver>(
        KeyboardDriver::default_keymap(), state_fn);

    InputSystem sys(std::move(driver));

    // Tick 0: nothing pressed
    sys.poll(0);
    EXPECT_TRUE(sys.snapshot().empty());

    // Tick 1: press S (P1_CENTER)
    fake_state[SDL_SCANCODE_S] = true;
    sys.poll(1);
    EXPECT_TRUE(sys.snapshot().is_pressed(PadInput::P1_CENTER));
    EXPECT_TRUE(sys.snapshot().is_held(PadInput::P1_CENTER));

    // Tick 2: still held
    sys.poll(2);
    EXPECT_TRUE(sys.snapshot().is_held(PadInput::P1_CENTER));
    EXPECT_FALSE(sys.snapshot().is_pressed(PadInput::P1_CENTER));

    // Tick 3: release
    fake_state[SDL_SCANCODE_S] = false;
    sys.poll(3);
    EXPECT_FALSE(sys.snapshot().is_held(PadInput::P1_CENTER));
    EXPECT_TRUE(sys.snapshot().is_released(PadInput::P1_CENTER));

    // Tick 4: clean
    sys.poll(4);
    EXPECT_TRUE(sys.snapshot().empty());
}
