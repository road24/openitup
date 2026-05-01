#include <gtest/gtest.h>
#include <openitup/input/input_snapshot.h>
#include <openitup/input/pad_input.h>
#include <openitup/input/keyboard_driver.h>

using namespace openitup;

// US-INP-061: Separate input snapshots per player
TEST(InputSnapshot, GetPlayerSnapshotP1) {
    // Create snapshot with P1 and P2 inputs
    uint32_t held = static_cast<uint32_t>(PadInput::P1_CENTER) |
                    static_cast<uint32_t>(PadInput::P2_UP_LEFT) |
                    static_cast<uint32_t>(PadInput::START);
    uint32_t pressed = held;
    uint32_t released = 0;

    InputSnapshot full(held, pressed, released, 100);

    // Get P1 snapshot
    InputSnapshot p1 = full.get_player_snapshot(0);

    // P1 should have P1_CENTER and START
    EXPECT_TRUE(p1.is_held(PadInput::P1_CENTER));
    EXPECT_TRUE(p1.is_pressed(PadInput::P1_CENTER));
    EXPECT_TRUE(p1.is_held(PadInput::START));

    // P1 should NOT have P2_UP_LEFT
    EXPECT_FALSE(p1.is_held(PadInput::P2_UP_LEFT));
    EXPECT_FALSE(p1.is_pressed(PadInput::P2_UP_LEFT));

    // Tick number preserved
    EXPECT_EQ(p1.tick_number(), 100);
}

TEST(InputSnapshot, GetPlayerSnapshotP2) {
    // Create snapshot with P1 and P2 inputs
    uint32_t held = static_cast<uint32_t>(PadInput::P1_CENTER) |
                    static_cast<uint32_t>(PadInput::P2_UP_LEFT) |
                    static_cast<uint32_t>(PadInput::BACK);
    uint32_t pressed = held;
    uint32_t released = 0;

    InputSnapshot full(held, pressed, released, 200);

    // Get P2 snapshot
    InputSnapshot p2 = full.get_player_snapshot(1);

    // P2 should have P2_UP_LEFT and BACK
    EXPECT_TRUE(p2.is_held(PadInput::P2_UP_LEFT));
    EXPECT_TRUE(p2.is_pressed(PadInput::P2_UP_LEFT));
    EXPECT_TRUE(p2.is_held(PadInput::BACK));

    // P2 should NOT have P1_CENTER
    EXPECT_FALSE(p2.is_held(PadInput::P1_CENTER));
    EXPECT_FALSE(p2.is_pressed(PadInput::P1_CENTER));

    // Tick number preserved
    EXPECT_EQ(p2.tick_number(), 200);
}

TEST(InputSnapshot, GetPlayerSnapshotIndependent) {
    // Simultaneous press on both players
    uint32_t held = static_cast<uint32_t>(PadInput::P1_CENTER) |
                    static_cast<uint32_t>(PadInput::P2_CENTER);
    uint32_t pressed = held;
    uint32_t released = 0;

    InputSnapshot full(held, pressed, released, 300);

    InputSnapshot p1 = full.get_player_snapshot(0);
    InputSnapshot p2 = full.get_player_snapshot(1);

    // P1 has only P1_CENTER
    EXPECT_TRUE(p1.is_pressed(PadInput::P1_CENTER));
    EXPECT_FALSE(p1.is_pressed(PadInput::P2_CENTER));

    // P2 has only P2_CENTER
    EXPECT_TRUE(p2.is_pressed(PadInput::P2_CENTER));
    EXPECT_FALSE(p2.is_pressed(PadInput::P1_CENTER));

    // No cross-contamination
    EXPECT_EQ(p1.tick_number(), p2.tick_number());
}

TEST(InputSnapshot, GetPlayerSnapshotMenuInputsShared) {
    // Menu inputs should be visible to both players
    uint32_t held = static_cast<uint32_t>(PadInput::START) |
                    static_cast<uint32_t>(PadInput::SELECT);
    uint32_t pressed = held;
    uint32_t released = 0;

    InputSnapshot full(held, pressed, released, 400);

    InputSnapshot p1 = full.get_player_snapshot(0);
    InputSnapshot p2 = full.get_player_snapshot(1);

    // Both players see menu inputs
    EXPECT_TRUE(p1.is_held(PadInput::START));
    EXPECT_TRUE(p1.is_held(PadInput::SELECT));
    EXPECT_TRUE(p2.is_held(PadInput::START));
    EXPECT_TRUE(p2.is_held(PadInput::SELECT));
}

TEST(InputSnapshot, GetPlayerSnapshotAllP1Panels) {
    // Test all P1 panel inputs
    uint32_t held = static_cast<uint32_t>(PadInput::P1_DOWN_LEFT) |
                    static_cast<uint32_t>(PadInput::P1_UP_LEFT) |
                    static_cast<uint32_t>(PadInput::P1_CENTER) |
                    static_cast<uint32_t>(PadInput::P1_UP_RIGHT) |
                    static_cast<uint32_t>(PadInput::P1_DOWN_RIGHT);
    uint32_t pressed = held;
    uint32_t released = 0;

    InputSnapshot full(held, pressed, released, 500);
    InputSnapshot p1 = full.get_player_snapshot(0);

    // All P1 panels present
    EXPECT_TRUE(p1.is_pressed(PadInput::P1_DOWN_LEFT));
    EXPECT_TRUE(p1.is_pressed(PadInput::P1_UP_LEFT));
    EXPECT_TRUE(p1.is_pressed(PadInput::P1_CENTER));
    EXPECT_TRUE(p1.is_pressed(PadInput::P1_UP_RIGHT));
    EXPECT_TRUE(p1.is_pressed(PadInput::P1_DOWN_RIGHT));
}

TEST(InputSnapshot, GetPlayerSnapshotAllP2Panels) {
    // Test all P2 panel inputs
    uint32_t held = static_cast<uint32_t>(PadInput::P2_DOWN_LEFT) |
                    static_cast<uint32_t>(PadInput::P2_UP_LEFT) |
                    static_cast<uint32_t>(PadInput::P2_CENTER) |
                    static_cast<uint32_t>(PadInput::P2_UP_RIGHT) |
                    static_cast<uint32_t>(PadInput::P2_DOWN_RIGHT);
    uint32_t pressed = held;
    uint32_t released = 0;

    InputSnapshot full(held, pressed, released, 600);
    InputSnapshot p2 = full.get_player_snapshot(1);

    // All P2 panels present
    EXPECT_TRUE(p2.is_pressed(PadInput::P2_DOWN_LEFT));
    EXPECT_TRUE(p2.is_pressed(PadInput::P2_UP_LEFT));
    EXPECT_TRUE(p2.is_pressed(PadInput::P2_CENTER));
    EXPECT_TRUE(p2.is_pressed(PadInput::P2_UP_RIGHT));
    EXPECT_TRUE(p2.is_pressed(PadInput::P2_DOWN_RIGHT));
}

// US-INP-062: Keyboard driver player assignment
TEST(KeyboardDriver, DefaultKeymapIncludesP1Bindings) {
    auto keymap = KeyboardDriver::default_keymap();

    // Count P1 bindings
    int p1_count = 0;
    for (const auto& mapping : keymap) {
        uint32_t input_bits = static_cast<uint32_t>(mapping.input);
        // P1 panels are bits 0-4
        if (input_bits >= (1 << 0) && input_bits <= (1 << 4)) {
            p1_count++;
        }
    }

    EXPECT_EQ(p1_count, 5);  // 5 P1 panels mapped
}

TEST(KeyboardDriver, DefaultKeymapIncludesP2Bindings) {
    auto keymap = KeyboardDriver::default_keymap();

    // Count P2 bindings
    int p2_count = 0;
    for (const auto& mapping : keymap) {
        uint32_t input_bits = static_cast<uint32_t>(mapping.input);
        // P2 panels are bits 5-9
        if (input_bits >= (1 << 5) && input_bits <= (1 << 9)) {
            p2_count++;
        }
    }

    EXPECT_EQ(p2_count, 5);  // 5 P2 panels mapped
}

TEST(KeyboardDriver, P1KeysProduceP1Snapshot) {
    // Mock keyboard state: Q key pressed (P1_UP_LEFT)
    bool keyboard_state[512] = {false};
    keyboard_state[SDL_SCANCODE_Q] = true;

    auto keymap = KeyboardDriver::default_keymap();
    KeyboardDriver driver(keymap, [&keyboard_state](int* numkeys) {
        *numkeys = 512;
        return keyboard_state;
    });

    uint32_t held = driver.poll_held();
    InputSnapshot full(held, held, 0, 0);
    InputSnapshot p1 = full.get_player_snapshot(0);
    InputSnapshot p2 = full.get_player_snapshot(1);

    // P1 snapshot should contain P1_UP_LEFT
    EXPECT_TRUE(p1.is_held(PadInput::P1_UP_LEFT));

    // P2 snapshot should be empty
    EXPECT_FALSE(p2.is_held(PadInput::P1_UP_LEFT));
    EXPECT_TRUE(p2.empty());
}

TEST(KeyboardDriver, P2KeysProduceP2Snapshot) {
    // Mock keyboard state: KP_7 key pressed (P2_UP_LEFT)
    bool keyboard_state[512] = {false};
    keyboard_state[SDL_SCANCODE_KP_7] = true;

    auto keymap = KeyboardDriver::default_keymap();
    KeyboardDriver driver(keymap, [&keyboard_state](int* numkeys) {
        *numkeys = 512;
        return keyboard_state;
    });

    uint32_t held = driver.poll_held();
    InputSnapshot full(held, held, 0, 0);
    InputSnapshot p1 = full.get_player_snapshot(0);
    InputSnapshot p2 = full.get_player_snapshot(1);

    // P2 snapshot should contain P2_UP_LEFT
    EXPECT_TRUE(p2.is_held(PadInput::P2_UP_LEFT));

    // P1 snapshot should be empty
    EXPECT_FALSE(p1.is_held(PadInput::P2_UP_LEFT));
    EXPECT_TRUE(p1.empty());
}

TEST(KeyboardDriver, SimultaneousTwoPlayerInput) {
    // Mock keyboard state: both P1 (Q) and P2 (KP_7) pressed
    bool keyboard_state[512] = {false};
    keyboard_state[SDL_SCANCODE_Q] = true;      // P1_UP_LEFT
    keyboard_state[SDL_SCANCODE_KP_7] = true;   // P2_UP_LEFT

    auto keymap = KeyboardDriver::default_keymap();
    KeyboardDriver driver(keymap, [&keyboard_state](int* numkeys) {
        *numkeys = 512;
        return keyboard_state;
    });

    uint32_t held = driver.poll_held();
    InputSnapshot full(held, held, 0, 0);
    InputSnapshot p1 = full.get_player_snapshot(0);
    InputSnapshot p2 = full.get_player_snapshot(1);

    // P1 has only P1_UP_LEFT
    EXPECT_TRUE(p1.is_held(PadInput::P1_UP_LEFT));
    EXPECT_FALSE(p1.is_held(PadInput::P2_UP_LEFT));

    // P2 has only P2_UP_LEFT
    EXPECT_TRUE(p2.is_held(PadInput::P2_UP_LEFT));
    EXPECT_FALSE(p2.is_held(PadInput::P1_UP_LEFT));
}

TEST(KeyboardDriver, P2NumpadLayout) {
    // Verify P2 numpad layout matches PIU convention
    auto keymap = KeyboardDriver::default_keymap();

    // Find P2 bindings
    std::map<PadInput, SDL_Scancode> p2_map;
    for (const auto& mapping : keymap) {
        uint32_t input_bits = static_cast<uint32_t>(mapping.input);
        if (input_bits >= (1 << 5) && input_bits <= (1 << 9)) {
            p2_map[mapping.input] = mapping.scancode;
        }
    }

    // Verify numpad layout: 7/9 = up, 4/6 = down, 5 = center
    EXPECT_EQ(p2_map[PadInput::P2_UP_LEFT], SDL_SCANCODE_KP_7);
    EXPECT_EQ(p2_map[PadInput::P2_UP_RIGHT], SDL_SCANCODE_KP_9);
    EXPECT_EQ(p2_map[PadInput::P2_CENTER], SDL_SCANCODE_KP_5);
    EXPECT_EQ(p2_map[PadInput::P2_DOWN_LEFT], SDL_SCANCODE_KP_1);
    EXPECT_EQ(p2_map[PadInput::P2_DOWN_RIGHT], SDL_SCANCODE_KP_3);
}
