#include "game/vehicle_simulation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace hover::game {

VehicleTickEvents simulate_vehicle(VehicleState& state, const VehicleTick& tick) {
    assert(tick.tick_seconds > 0.0F);
    const HandlingProfile& handling = tick.definition.handling;
    const bool boost_just_pressed = tick.input.boost && !state.boost_input_was_down;
    state.boost_input_was_down = tick.input.boost;
    constexpr float active_input_threshold = 0.001F;
    const bool throttle_active = tick.input.throttle > active_input_threshold;
    const bool brake_active = tick.input.brake > active_input_threshold;
    const bool can_activate_boost = throttle_active && !brake_active;
    if (boost_just_pressed && can_activate_boost) {
        state.boost_seconds_remaining = handling.boost_duration_seconds;
    }
    if (brake_active) {
        state.boost_seconds_remaining = 0.0F;
    } else if (!throttle_active && state.boost_seconds_remaining > 0.0F) {
        state.boost_seconds_remaining =
            std::min(state.boost_seconds_remaining, handling.boost_throttle_release_tail_seconds);
    }
    state.boosting = state.boost_seconds_remaining > 0.0F;
    const VehicleTickEvents events{.boost_activated = boost_just_pressed && can_activate_boost};
    float acceleration =
        tick.input.throttle * handling.forward_acceleration_metres_per_second_squared -
        tick.input.brake * handling.braking_deceleration_metres_per_second_squared;
    if (tick.input.throttle <= 0.0F && tick.input.brake <= 0.0F &&
        state.forward_speed_metres_per_second > 0.0F) {
        acceleration -= handling.coasting_deceleration_metres_per_second_squared;
    }
    const bool returning_from_boost =
        !state.boosting && state.forward_speed_metres_per_second >
                               handling.base_maximum_forward_speed_metres_per_second;
    if (state.boosting) {
        acceleration += tick.input.throttle * handling.boost_acceleration_metres_per_second_squared;
    } else if (returning_from_boost) {
        acceleration -= handling.boost_excess_speed_decay_metres_per_second_squared;
    }

    const float boosted_speed_limit = handling.base_maximum_forward_speed_metres_per_second *
                                      handling.boost_maximum_speed_multiplier;
    const float active_speed_limit =
        state.boosting ? boosted_speed_limit
                       : std::max(handling.base_maximum_forward_speed_metres_per_second,
                                  state.forward_speed_metres_per_second);
    state.forward_speed_metres_per_second =
        std::clamp(state.forward_speed_metres_per_second + acceleration * tick.tick_seconds, 0.0F,
                   active_speed_limit);
    if (returning_from_boost && state.forward_speed_metres_per_second <
                                    handling.base_maximum_forward_speed_metres_per_second) {
        state.forward_speed_metres_per_second =
            handling.base_maximum_forward_speed_metres_per_second;
    }

    const float speed_ratio = std::clamp(state.forward_speed_metres_per_second /
                                             handling.base_maximum_forward_speed_metres_per_second,
                                         0.0F, 1.0F);
    const float steering_authority = 0.60F + 0.40F * std::sqrt(speed_ratio);
    state.pose.yaw_radians += tick.input.steering * handling.steering_rate_radians_per_second *
                              steering_authority * tick.tick_seconds;

    const ShipPresentationProfile& presentation = tick.definition.presentation;
    const float target_roll =
        -tick.input.steering * presentation.maximum_turn_roll_radians * speed_ratio;
    const float roll_blend =
        1.0F - std::exp(-presentation.turn_roll_response_per_second * tick.tick_seconds);
    state.pose.turn_roll_radians += (target_roll - state.pose.turn_roll_radians) * roll_blend;

    state.pose.position =
        state.pose.position +
        forward_direction(state.pose) * (state.forward_speed_metres_per_second * tick.tick_seconds);

    if (state.boosting) {
        state.boost_seconds_remaining =
            std::max(0.0F, state.boost_seconds_remaining - tick.tick_seconds);
        if (state.boost_seconds_remaining < 0.0001F) {
            state.boost_seconds_remaining = 0.0F;
        }
        state.boosting = state.boost_seconds_remaining > 0.0F;
    }
    return events;
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
