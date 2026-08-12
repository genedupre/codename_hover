#include "game/vehicle_simulation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace hover::game {

void simulate_vehicle(VehicleState& state, const VehicleTick& tick) {
    assert(tick.tick_seconds > 0.0F);
    const HandlingProfile& handling = tick.definition.handling;
    float acceleration =
        tick.input.throttle * handling.forward_acceleration_metres_per_second_squared -
        tick.input.brake * handling.braking_deceleration_metres_per_second_squared;
    if (tick.input.throttle <= 0.0F && tick.input.brake <= 0.0F &&
        state.forward_speed_metres_per_second > 0.0F) {
        acceleration -= handling.coasting_deceleration_metres_per_second_squared;
    }

    state.forward_speed_metres_per_second =
        std::clamp(state.forward_speed_metres_per_second + acceleration * tick.tick_seconds, 0.0F,
                   handling.maximum_forward_speed_metres_per_second);

    const float speed_ratio =
        state.forward_speed_metres_per_second / handling.maximum_forward_speed_metres_per_second;
    const float steering_authority = 0.15F + 0.85F * speed_ratio;
    state.pose.yaw_radians += tick.input.steering * handling.steering_rate_radians_per_second *
                              steering_authority * tick.tick_seconds;

    const ShipPresentationProfile& presentation = tick.definition.presentation;
    const float target_roll = -tick.input.steering * presentation.maximum_turn_roll_radians *
                              speed_ratio;
    const float roll_blend =
        1.0F - std::exp(-presentation.turn_roll_response_per_second * tick.tick_seconds);
    state.pose.turn_roll_radians +=
        (target_roll - state.pose.turn_roll_radians) * roll_blend;

    state.pose.position =
        state.pose.position +
        forward_direction(state.pose) * (state.forward_speed_metres_per_second * tick.tick_seconds);
}

VehiclePose interpolate(const VehiclePose& previous, const VehiclePose& current, float alpha) {
    const float clamped_alpha = std::clamp(alpha, 0.0F, 1.0F);
    return VehiclePose{
        previous.position + (current.position - previous.position) * clamped_alpha,
        previous.yaw_radians + (current.yaw_radians - previous.yaw_radians) * clamped_alpha,
        previous.turn_roll_radians +
            (current.turn_roll_radians - previous.turn_roll_radians) * clamped_alpha,
    };
}

math::Vec3 forward_direction(const VehiclePose& pose) {
    return math::Vec3{std::sin(pose.yaw_radians), 0.0F, std::cos(pose.yaw_radians)};
}

math::Mat4 model_matrix(const VehiclePose& pose) {
    return math::translation(pose.position) * math::rotation_y(pose.yaw_radians) *
           math::rotation_z(pose.turn_roll_radians);
}

} // namespace hover::game
