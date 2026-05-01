#include <gtest/gtest.h>

#include <openitup/input/keyboard_driver.h>
#include <openitup/data/settings.h>

using namespace openitup;
using namespace openitup::data;

class KeyboardDriverTest : public ::testing::Test {
protected:
};

TEST_F(KeyboardDriverTest, DefaultKeymapMatchesSettingsDefault) {
    auto driver_keymap = KeyboardDriver::default_keymap();
    auto settings_keymap = InputSettings::make_default();

    // Both should have the same number of entries
    EXPECT_EQ(driver_keymap.size(), settings_keymap.keymap.size());

    // Verify each mapping matches
    for (const auto& mapping : driver_keymap) {
        std::string scancode_str = KeyboardDriver::scancode_to_string(mapping.scancode);
        std::string pad_input_str = pad_input_to_string(mapping.input);

        ASSERT_TRUE(settings_keymap.keymap.count(scancode_str) > 0)
            << "Scancode " << scancode_str << " not found in settings default";
        EXPECT_EQ(settings_keymap.keymap.at(scancode_str), pad_input_str)
            << "Mismatch for scancode " << scancode_str;
    }
}

TEST_F(KeyboardDriverTest, LoadKeymapFromSettings) {
    KeyboardDriver driver;

    InputSettings settings;
    settings.keymap = {
        {"SDL_SCANCODE_A", "P1_UP_LEFT"},
        {"SDL_SCANCODE_D", "P1_UP_RIGHT"},
    };

    driver.load_keymap(settings);

    auto keymap = driver.keymap();
    EXPECT_EQ(keymap.size(), 2);
    EXPECT_EQ(keymap[0].scancode, SDL_SCANCODE_A);
    EXPECT_EQ(keymap[0].input, PadInput::P1_UP_LEFT);
    EXPECT_EQ(keymap[1].scancode, SDL_SCANCODE_D);
    EXPECT_EQ(keymap[1].input, PadInput::P1_UP_RIGHT);
}

TEST_F(KeyboardDriverTest, SaveKeymapToSettings) {
    KeyboardDriver driver;
    driver.set_keymap({
        {SDL_SCANCODE_Q, PadInput::P1_UP_LEFT},
        {SDL_SCANCODE_E, PadInput::P1_UP_RIGHT},
    });

    auto settings = driver.save_keymap();

    EXPECT_EQ(settings.keymap.size(), 2);
    EXPECT_EQ(settings.keymap["SDL_SCANCODE_Q"], "P1_UP_LEFT");
    EXPECT_EQ(settings.keymap["SDL_SCANCODE_E"], "P1_UP_RIGHT");
}

TEST_F(KeyboardDriverTest, LoadSaveRoundTrip) {
    KeyboardDriver driver1;
    auto original_settings = InputSettings::make_default();

    // Load settings into driver
    driver1.load_keymap(original_settings);

    // Save back to settings
    auto saved_settings = driver1.save_keymap();

    // Load into a second driver
    KeyboardDriver driver2;
    driver2.load_keymap(saved_settings);

    // Both drivers should have the same keymap
    auto keymap1 = driver1.keymap();
    auto keymap2 = driver2.keymap();

    ASSERT_EQ(keymap1.size(), keymap2.size());
    for (size_t i = 0; i < keymap1.size(); ++i) {
        EXPECT_EQ(keymap1[i].scancode, keymap2[i].scancode);
        EXPECT_EQ(keymap1[i].input, keymap2[i].input);
    }
}

TEST_F(KeyboardDriverTest, LoadInvalidScancodeLogsWarning) {
    KeyboardDriver driver;

    InputSettings settings;
    settings.keymap = {
        {"INVALID_SCANCODE", "P1_UP_LEFT"},
        {"SDL_SCANCODE_Q", "P1_UP_RIGHT"},  // Valid entry
    };

    driver.load_keymap(settings);

    // Should only have the valid entry
    auto keymap = driver.keymap();
    EXPECT_EQ(keymap.size(), 1);
    EXPECT_EQ(keymap[0].scancode, SDL_SCANCODE_Q);
    EXPECT_EQ(keymap[0].input, PadInput::P1_UP_RIGHT);
}

TEST_F(KeyboardDriverTest, LoadInvalidPadInputLogsWarning) {
    KeyboardDriver driver;

    InputSettings settings;
    settings.keymap = {
        {"SDL_SCANCODE_Q", "INVALID_INPUT"},
        {"SDL_SCANCODE_E", "P1_UP_RIGHT"},  // Valid entry
    };

    driver.load_keymap(settings);

    // Should only have the valid entry
    auto keymap = driver.keymap();
    EXPECT_EQ(keymap.size(), 1);
    EXPECT_EQ(keymap[0].scancode, SDL_SCANCODE_E);
    EXPECT_EQ(keymap[0].input, PadInput::P1_UP_RIGHT);
}

TEST_F(KeyboardDriverTest, LoadEmptyKeymapUsesDefaults) {
    KeyboardDriver driver;

    InputSettings settings;
    settings.keymap = {};  // Empty keymap

    driver.load_keymap(settings);

    // Should fall back to default keymap
    auto keymap = driver.keymap();
    auto default_keymap = KeyboardDriver::default_keymap();
    EXPECT_EQ(keymap.size(), default_keymap.size());
}

TEST_F(KeyboardDriverTest, LoadAllInvalidEntriesUsesDefaults) {
    KeyboardDriver driver;

    InputSettings settings;
    settings.keymap = {
        {"INVALID_SCANCODE", "P1_UP_LEFT"},
        {"SDL_SCANCODE_Q", "INVALID_INPUT"},
    };

    driver.load_keymap(settings);

    // All entries invalid, should fall back to defaults
    auto keymap = driver.keymap();
    auto default_keymap = KeyboardDriver::default_keymap();
    EXPECT_EQ(keymap.size(), default_keymap.size());
}

TEST_F(KeyboardDriverTest, ScancodeToString) {
    EXPECT_EQ(KeyboardDriver::scancode_to_string(SDL_SCANCODE_Q), "SDL_SCANCODE_Q");
    EXPECT_EQ(KeyboardDriver::scancode_to_string(SDL_SCANCODE_RETURN), "SDL_SCANCODE_RETURN");
    EXPECT_EQ(KeyboardDriver::scancode_to_string(SDL_SCANCODE_SPACE), "SDL_SCANCODE_SPACE");
}

TEST_F(KeyboardDriverTest, StringToScancode) {
    EXPECT_EQ(KeyboardDriver::string_to_scancode("SDL_SCANCODE_Q"), SDL_SCANCODE_Q);
    EXPECT_EQ(KeyboardDriver::string_to_scancode("SDL_SCANCODE_RETURN"), SDL_SCANCODE_RETURN);
    EXPECT_EQ(KeyboardDriver::string_to_scancode("SDL_SCANCODE_SPACE"), SDL_SCANCODE_SPACE);
    EXPECT_EQ(KeyboardDriver::string_to_scancode("INVALID"), SDL_SCANCODE_UNKNOWN);
}

TEST_F(KeyboardDriverTest, StringToPadInput) {
    auto result = KeyboardDriver::string_to_pad_input("P1_UP_LEFT");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, PadInput::P1_UP_LEFT);

    result = KeyboardDriver::string_to_pad_input("START");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, PadInput::START);

    result = KeyboardDriver::string_to_pad_input("INVALID");
    EXPECT_EQ(result, nullptr);
}
