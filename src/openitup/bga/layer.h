#pragma once

#include <optional>
#include <string>
#include <vector>

#include <openitup/bga/keyframe.h>
#include <openitup/sprite/sprite.h>

namespace openitup {

class Layer {
public:
    std::string sprite_name;
    Sprite* sprite = nullptr;
    std::vector<Keyframe> keyframes;

    std::optional<InterpolatedProps> evaluate(float tick) const;
};

} // namespace openitup
