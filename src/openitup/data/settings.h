#pragma once

#include <map>
#include <string>

#include <nlohmann/json.hpp>

namespace openitup::data {

struct VideoSettings {
    int width = 1920;
    int height = 1080;
};

struct AudioSettings {
    float master_volume = 1.0f;  // [0.0, 1.0]
    float music_volume = 1.0f;   // [0.0, 1.0]
    float sfx_volume = 1.0f;     // [0.0, 1.0]
    int global_audio_offset_ms = 0;  // [-500, +500] US-AUD-061
};

struct InputSettings {
    // Keymap: SDL scancode name -> PadInput name
    // e.g., {"SDL_SCANCODE_Q": "P1_UP_LEFT"}
    std::map<std::string, std::string> keymap;

    // Returns default QWEASDZXC keymap matching US-INP-022
    static InputSettings make_default();
};

struct SettingsData {
    int schema_version = 1;

    VideoSettings video;
    AudioSettings audio;
    InputSettings input;

    // Returns defaults for all fields.
    static SettingsData make_default();
};

// JSON serialization functions
void to_json(nlohmann::json& j, const VideoSettings& v);
void from_json(const nlohmann::json& j, VideoSettings& v);

void to_json(nlohmann::json& j, const AudioSettings& a);
void from_json(const nlohmann::json& j, AudioSettings& a);

void to_json(nlohmann::json& j, const InputSettings& i);
void from_json(const nlohmann::json& j, InputSettings& i);

void to_json(nlohmann::json& j, const SettingsData& s);
void from_json(const nlohmann::json& j, SettingsData& s);

} // namespace openitup::data
