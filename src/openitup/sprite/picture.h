#pragma once

#include <string>

#include <openitup/core/json_serializable.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/math/types.h>

namespace openitup {

struct Picture : public JsonSerializable {
    TextureHandle texture = TextureHandle::Invalid;
    std::string texture_name;
    ScreenRect rect{};
    UVRect uv{};
    std::string name;

    int width() const { return rect.width(); }
    int height() const { return rect.height(); }

    void from_json(const nlohmann::json& j) override {
        texture_name = j.at("texture").get<std::string>();

        const auto& r = j.at("rect");
        rect.x1 = r[0].get<int>();
        rect.y1 = r[1].get<int>();
        rect.x2 = r[2].get<int>();
        rect.y2 = r[3].get<int>();

        const auto& u = j.at("uv");
        uv.u1 = u[0].get<float>();
        uv.v1 = u[1].get<float>();
        uv.u2 = u[2].get<float>();
        uv.v2 = u[3].get<float>();

        if (j.contains("name")) {
            name = j.at("name").get<std::string>();
        }
    }

    nlohmann::json to_json() const override {
        nlohmann::json j = {
            {"texture", texture_name},
            {"rect", {rect.x1, rect.y1, rect.x2, rect.y2}},
            {"uv", {uv.u1, uv.v1, uv.u2, uv.v2}}
        };
        if (!name.empty()) {
            j["name"] = name;
        }
        return j;
    }
};

} // namespace openitup
