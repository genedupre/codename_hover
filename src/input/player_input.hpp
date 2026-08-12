#pragma once

#include <cstdint>

namespace hover::input {

struct PlayerInput {
    float steering = 0.0F;
    float throttle = 0.0F;
    float brake = 0.0F;
    bool drift_left = false;
    bool drift_right = false;
    bool boost = false;
};

struct AxisSample {
    std::int16_t value;
    std::int16_t dead_zone;
};

[[nodiscard]] PlayerInput merge(PlayerInput current, const PlayerInput& contribution);
[[nodiscard]] float normalize_signed_axis(AxisSample sample);
[[nodiscard]] float normalize_trigger(AxisSample sample);

} // namespace hover::input
