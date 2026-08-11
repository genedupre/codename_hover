#include "input/player_input.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hover::input {

PlayerInput merge(PlayerInput current, const PlayerInput& contribution) {
    if (std::abs(contribution.steering) > std::abs(current.steering)) {
        current.steering = contribution.steering;
    }
    current.throttle = std::max(current.throttle, contribution.throttle);
    current.brake = std::max(current.brake, contribution.brake);
    current.drift = current.drift || contribution.drift;
    current.boost = current.boost || contribution.boost;
    return current;
}

float normalize_signed_axis(AxisSample sample) {
    const std::int16_t value = sample.value;
    const int magnitude = std::abs(static_cast<int>(value));
    const int threshold = std::max(0, static_cast<int>(sample.dead_zone));
    if (magnitude <= threshold) {
        return 0.0F;
    }

    const int maximum_magnitude = value < 0
                                      ? -static_cast<int>(std::numeric_limits<std::int16_t>::min())
                                      : static_cast<int>(std::numeric_limits<std::int16_t>::max());
    const float normalized = static_cast<float>(magnitude - threshold) /
                             static_cast<float>(maximum_magnitude - threshold);
    return value < 0 ? -normalized : normalized;
}

float normalize_trigger(AxisSample sample) {
    const int threshold = std::max(0, static_cast<int>(sample.dead_zone));
    const std::int16_t value = sample.value;
    const int magnitude = std::max(0, static_cast<int>(value));
    if (magnitude <= threshold) {
        return 0.0F;
    }

    return static_cast<float>(magnitude - threshold) /
           static_cast<float>(static_cast<int>(std::numeric_limits<std::int16_t>::max()) -
                              threshold);
}

} // namespace hover::input
