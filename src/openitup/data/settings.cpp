#include <openitup/data/settings.h>

namespace openitup::data {

SettingsData SettingsData::make_default() {
    SettingsData defaults;
    defaults.schema_version = 1;
    defaults.video.width = 1920;
    defaults.video.height = 1080;
    defaults.audio.master_volume = 1.0f;
    defaults.audio.music_volume = 1.0f;
    defaults.audio.sfx_volume = 1.0f;
    defaults.input = InputSettings::make_default();
    return defaults;
}

void to_json(nlohmann::json& j, const VideoSettings& v) {
    j = nlohmann::json{
        {"width", v.width},
        {"height", v.height}
    };
}

void from_json(const nlohmann::json& j, VideoSettings& v) {
    v.width = j.value("width", 1920);
    v.height = j.value("height", 1080);
}

void to_json(nlohmann::json& j, const AudioSettings& a) {
    j = nlohmann::json{
        {"master_volume", a.master_volume},
        {"music_volume", a.music_volume},
        {"sfx_volume", a.sfx_volume}
    };
}

void from_json(const nlohmann::json& j, AudioSettings& a) {
    a.master_volume = j.value("master_volume", 1.0f);
    a.music_volume = j.value("music_volume", 1.0f);
    a.sfx_volume = j.value("sfx_volume", 1.0f);
}

InputSettings InputSettings::make_default() {
    InputSettings defaults;
    defaults.keymap = {
        {"SDL_SCANCODE_Q", "P1_UP_LEFT"},
        {"SDL_SCANCODE_E", "P1_UP_RIGHT"},
        {"SDL_SCANCODE_S", "P1_CENTER"},
        {"SDL_SCANCODE_Z", "P1_DOWN_LEFT"},
        {"SDL_SCANCODE_C", "P1_DOWN_RIGHT"},
        {"SDL_SCANCODE_RETURN", "START"},
        {"SDL_SCANCODE_ESCAPE", "BACK"},
        {"SDL_SCANCODE_SPACE", "SELECT"},
    };
    return defaults;
}

void to_json(nlohmann::json& j, const InputSettings& i) {
    j = nlohmann::json{
        {"keymap", i.keymap}
    };
}

void from_json(const nlohmann::json& j, InputSettings& i) {
    if (j.contains("keymap") && j["keymap"].is_object()) {
        i.keymap = j["keymap"].get<std::map<std::string, std::string>>();
    } else {
        // Use defaults if keymap is missing or malformed
        i = InputSettings::make_default();
    }
}

void to_json(nlohmann::json& j, const SettingsData& s) {
    j = nlohmann::json{
        {"schema_version", s.schema_version},
        {"video", s.video},
        {"audio", s.audio},
        {"input", s.input}
    };
}

void from_json(const nlohmann::json& j, SettingsData& s) {
    s.schema_version = j.value("schema_version", 1);

    // Parse sections with defaults if missing
    if (j.contains("video")) {
        s.video = j["video"].get<VideoSettings>();
    } else {
        s.video = VideoSettings{};
    }

    if (j.contains("audio")) {
        s.audio = j["audio"].get<AudioSettings>();
    } else {
        s.audio = AudioSettings{};
    }

    if (j.contains("input")) {
        s.input = j["input"].get<InputSettings>();
    } else {
        s.input = InputSettings{};
    }
}

} // namespace openitup::data
