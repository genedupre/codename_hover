#pragma once

#include "game/ship_definition.hpp"
#include "hover_math.hpp"
#include "input/player_input.hpp"

namespace hover::game {

struct VehiclePose {
    math::Vec3 position{0.0F, 0.0F, 0.0F};
    float yaw_radians = 0.0F;
    float turn_roll_radians = 0.0F;
};

struct VehicleState {
    VehiclePose pose;
    float forward_speed_metres_per_second = 0.0F;
    float boost_seconds_remaining = 0.0F;
    bool boosting = false;
    bool boost_input_was_down = false;
};

struct VehicleTick {
    input::PlayerInput input;
    const ShipDefinition& definition;
    float tick_seconds;
};

struct VehicleTickEvents {
    bool boost_activated = false;
};

VehicleTickEvents simulate_vehicle(VehicleState& state, const VehicleTick& tick);
[[nodiscard]] VehiclePose interpolate(const VehiclePose& previous, const VehiclePose& current,
                                      float alpha);
[[nodiscard]] math::Vec3 forward_direction(const VehiclePose& pose);
[[nodiscard]] math::Mat4 model_matrix(const VehiclePose& pose);

} // namespace hover::game
