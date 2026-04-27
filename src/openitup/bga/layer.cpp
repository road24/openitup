#include <openitup/bga/layer.h>

namespace openitup {

std::optional<InterpolatedProps> Layer::evaluate(float tick) const {
    return evaluate_keyframes(keyframes, tick);
}

} // namespace openitup
