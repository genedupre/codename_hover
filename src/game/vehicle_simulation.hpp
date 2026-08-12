#pragma once

#include "game/ship_definition.hpp"
#include "hover_math.hpp"
#include "input/player_input.hpp"

namespace hover::game {

struct VehiclePose {
    math::Vec3 position{0.0F, 0.0F, 0.0F};
    math::Vec3 forward{0.0F, 0.0F, 1.0F};
    math::Vec3 up{0.0F, 1.0F, 0.0F};
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

// Advances shared propulsion, boost, and visual steering response without
// choosing how the vehicle moves through the world. Free-planar and
// track-attached movement both build on this deterministic step.
VehicleTickEvents simulate_vehicle_dynamics(VehicleState& state, const VehicleTick& tick);
VehicleTickEvents simulate_vehicle(VehicleState& state, const VehicleTick& tick);
[[nodiscard]] bool is_valid(const VehiclePose& pose);
[[nodiscard]] bool is_valid(const VehicleState& state);
[[nodiscard]] VehiclePose interpolate(const VehiclePose& previous, const VehiclePose& current,
                                      float alpha);
[[nodiscard]] math::Vec3 forward_direction(const VehiclePose& pose);
[[nodiscard]] math::Vec3 up_direction(const VehiclePose& pose);
[[nodiscard]] math::Mat4 model_matrix(const VehiclePose& pose);

} // namespace hover::game
