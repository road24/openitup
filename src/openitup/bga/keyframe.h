#pragma once

#include <optional>
#include <vector>
#include <algorithm>
#include <cstdint>

#include <openitup/core/json_serializable.h>
#include <openitup/math/types.h>

namespace openitup {

enum class BlendEffect : uint8_t {
    Normal = 0,
    Screen = 1,
    Multiply = 2,
    Dodge = 3,
    Difference = 4
};

inline BlendEffect blend_effect_from_string(const std::string& s) {
    if (s == "screen") return BlendEffect::Screen;
    if (s == "multiply") return BlendEffect::Multiply;
    if (s == "dodge") return BlendEffect::Dodge;
    if (s == "difference") return BlendEffect::Difference;
    return BlendEffect::Normal;
}

inline const char* blend_effect_to_string(BlendEffect e) {
    switch (e) {
        case BlendEffect::Screen:     return "screen";
        case BlendEffect::Multiply:   return "multiply";
        case BlendEffect::Dodge:      return "dodge";
        case BlendEffect::Difference: return "difference";
        default:                      return "normal";
    }
}

struct Keyframe : public JsonSerializable {
    uint16_t tick = 0;
    float translate_x = 0.0f, translate_y = 0.0f;
    float pivot_x = 0.0f, pivot_y = 0.0f;
    float scale_x = 1.0f, scale_y = 1.0f;
    float rotate = 0.0f;
    float color_r = 1.0f, color_g = 1.0f, color_b = 1.0f, color_a = 1.0f;
    bool display = true;
    BlendEffect effect = BlendEffect::Normal;

    void from_json(const nlohmann::json& j) override {
        tick = j.at("tick").get<uint16_t>();

        const auto& tr = j.at("translate");
        translate_x = tr[0].get<float>();
        translate_y = tr[1].get<float>();

        const auto& pv = j.at("pivot");
        pivot_x = pv[0].get<float>();
        pivot_y = pv[1].get<float>();

        const auto& sc = j.at("scale");
        scale_x = sc[0].get<float>();
        scale_y = sc[1].get<float>();

        rotate = j.at("rotate").get<float>();

        const auto& col = j.at("color");
        color_r = col[0].get<float>();
        color_g = col[1].get<float>();
        color_b = col[2].get<float>();
        color_a = col[3].get<float>();

        display = j.at("display").get<bool>();
        effect = blend_effect_from_string(j.at("effect").get<std::string>());
    }

    nlohmann::json to_json() const override {
        return {
            {"tick", tick},
            {"translate", {translate_x, translate_y}},
            {"pivot", {pivot_x, pivot_y}},
            {"scale", {scale_x, scale_y}},
            {"rotate", rotate},
            {"color", {color_r, color_g, color_b, color_a}},
            {"display", display},
            {"effect", blend_effect_to_string(effect)}
        };
    }
};

struct InterpolatedProps {
    float translate_x, translate_y;
    float pivot_x, pivot_y;
    float scale_x, scale_y;
    float rotate;
    float color_r, color_g, color_b, color_a;
    bool display;
    BlendEffect effect;
    float dt;
};

inline InterpolatedProps interpolate_keyframes(const Keyframe& start,
                                                const Keyframe& end,
                                                float tick) {
    float dt = 0.0f;
    float duration = static_cast<float>(end.tick) - static_cast<float>(start.tick);
    if (duration > 0.0f) {
        dt = (tick - static_cast<float>(start.tick)) / duration;
    }

    InterpolatedProps props{};
    props.translate_x = lerp(start.translate_x, end.translate_x, dt);
    props.translate_y = lerp(start.translate_y, end.translate_y, dt);
    props.scale_x     = lerp(start.scale_x, end.scale_x, dt);
    props.scale_y     = lerp(start.scale_y, end.scale_y, dt);
    props.rotate      = lerp(start.rotate, end.rotate, dt);
    props.color_r     = lerp(start.color_r, end.color_r, dt);
    props.color_g     = lerp(start.color_g, end.color_g, dt);
    props.color_b     = lerp(start.color_b, end.color_b, dt);
    props.color_a     = lerp(start.color_a, end.color_a, dt);

    // Not interpolated: snap from start keyframe
    props.pivot_x = start.pivot_x;
    props.pivot_y = start.pivot_y;
    props.display = start.display;
    props.effect  = start.effect;
    props.dt      = dt;

    return props;
}

inline std::optional<InterpolatedProps> evaluate_keyframes(
        const std::vector<Keyframe>& keyframes, float tick) {
    if (keyframes.empty()) return std::nullopt;
    if (tick < static_cast<float>(keyframes.front().tick)) return std::nullopt;
    if (tick >= static_cast<float>(keyframes.back().tick)) return std::nullopt;

    auto it = std::upper_bound(
        keyframes.begin(), keyframes.end(), tick,
        [](float t, const Keyframe& kf) { return t < static_cast<float>(kf.tick); });

    const Keyframe& kf_start = *std::prev(it);
    const Keyframe& kf_end   = *it;

    if (!kf_start.display) return std::nullopt;

    return interpolate_keyframes(kf_start, kf_end, tick);
}

} // namespace openitup
