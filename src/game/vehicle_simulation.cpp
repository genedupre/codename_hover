#include "game/vehicle_simulation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace hover::game {
namespace {

constexpr float pose_tolerance = 0.002F;

bool is_finite(math::Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

math::Vec3 rotate_planar_forward(math::Vec3 forward, float radians) {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return math::normalized(math::Vec3{
        forward.x * cosine + forward.z * sine,
        0.0F,
        -forward.x * sine + forward.z * cosine,
    });
}

} // namespace

VehicleBoostTickResult advance_vehicle_boost_state(VehicleState& state, const VehicleTick& tick) {
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
    const bool active_for_tick = state.boost_seconds_remaining > 0.0F;
    state.boosting = active_for_tick;
    const VehicleTickEvents events{.boost_activated = boost_just_pressed && can_activate_boost};

    if (active_for_tick) {
        state.boost_seconds_remaining =
            std::max(0.0F, state.boost_seconds_remaining - tick.tick_seconds);
        if (state.boost_seconds_remaining < 0.0001F) {
            state.boost_seconds_remaining = 0.0F;
        }
        state.boosting = state.boost_seconds_remaining > 0.0F;
    }
    return VehicleBoostTickResult{events, active_for_tick};
}

void update_vehicle_turn_roll(VehicleState& state, const VehicleTick& tick) {
    assert(tick.tick_seconds > 0.0F);
    const HandlingProfile& handling = tick.definition.handling;
    const float speed_ratio = std::clamp(state.forward_speed_metres_per_second /
                                             handling.base_maximum_forward_speed_metres_per_second,
                                         0.0F, 1.0F);
    const ShipPresentationProfile& presentation = tick.definition.presentation;
    const float target_roll =
        -tick.input.steering * presentation.maximum_turn_roll_radians * speed_ratio;
    const float roll_blend =
        1.0F - std::exp(-presentation.turn_roll_response_per_second * tick.tick_seconds);
    state.pose.turn_roll_radians += (target_roll - state.pose.turn_roll_radians) * roll_blend;
}

VehicleTickEvents simulate_vehicle_dynamics(VehicleState& state, const VehicleTick& tick) {
    assert(tick.tick_seconds > 0.0F);
    const HandlingProfile& handling = tick.definition.handling;
    const VehicleBoostTickResult boost = advance_vehicle_boost_state(state, tick);
    float acceleration =
        tick.input.throttle * handling.forward_acceleration_metres_per_second_squared -
        tick.input.brake * handling.braking_deceleration_metres_per_second_squared;
    if (tick.input.throttle <= 0.0F && tick.input.brake <= 0.0F &&
        state.forward_speed_metres_per_second > 0.0F) {
        acceleration -= handling.coasting_deceleration_metres_per_second_squared;
    }
    const bool returning_from_boost =
        !boost.active_for_tick && state.forward_speed_metres_per_second >
                                      handling.base_maximum_forward_speed_metres_per_second;
    if (boost.active_for_tick) {
        acceleration += tick.input.throttle * handling.boost_acceleration_metres_per_second_squared;
    } else if (returning_from_boost) {
        acceleration -= handling.boost_excess_speed_decay_metres_per_second_squared;
    }

    const float boosted_speed_limit = handling.base_maximum_forward_speed_metres_per_second *
                                      handling.boost_maximum_speed_multiplier;
    const float active_speed_limit =
        boost.active_for_tick ? boosted_speed_limit
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

    update_vehicle_turn_roll(state, tick);
    return boost.events;
}

VehicleTickEvents simulate_vehicle(VehicleState& state, const VehicleTick& tick) {
    const VehicleTickEvents events = simulate_vehicle_dynamics(state, tick);
    const HandlingProfile& handling = tick.definition.handling;
    const float speed_ratio = std::clamp(state.forward_speed_metres_per_second /
                                             handling.base_maximum_forward_speed_metres_per_second,
                                         0.0F, 1.0F);
    const float steering_authority = 0.60F + 0.40F * std::sqrt(speed_ratio);
    const float steering_radians = tick.input.steering * handling.steering_rate_radians_per_second *
                                   steering_authority * tick.tick_seconds;
    state.pose.forward = rotate_planar_forward(state.pose.forward, steering_radians);
    state.pose.up = {0.0F, 1.0F, 0.0F};
    state.pose.position =
        state.pose.position +
        forward_direction(state.pose) * (state.forward_speed_metres_per_second * tick.tick_seconds);
    return events;
}

bool is_valid(const VehiclePose& pose) {
    if (!is_finite(pose.position) || !is_finite(pose.forward) || !is_finite(pose.up) ||
        !std::isfinite(pose.turn_roll_radians)) {
        return false;
    }
    return std::abs(math::dot(pose.forward, pose.forward) - 1.0F) <= pose_tolerance &&
           std::abs(math::dot(pose.up, pose.up) - 1.0F) <= pose_tolerance &&
           std::abs(math::dot(pose.forward, pose.up)) <= pose_tolerance;
}

bool is_valid(const VehicleState& state) {
    return is_valid(state.pose) && std::isfinite(state.forward_speed_metres_per_second) &&
           state.forward_speed_metres_per_second >= 0.0F &&
           std::isfinite(state.boost_seconds_remaining) && state.boost_seconds_remaining >= 0.0F &&
           state.boosting == (state.boost_seconds_remaining > 0.0F);
}

VehiclePose interpolate(const VehiclePose& previous, const VehiclePose& current, float alpha) {
    const float clamped_alpha = std::clamp(alpha, 0.0F, 1.0F);
    const math::Vec3 forward =
        math::normalized(previous.forward + (current.forward - previous.forward) * clamped_alpha);
    const math::Vec3 blended_up = previous.up + (current.up - previous.up) * clamped_alpha;
    const math::Vec3 right = math::normalized(math::cross(blended_up, forward));
    const math::Vec3 up = math::normalized(math::cross(forward, right));
    return VehiclePose{
        .position = previous.position + (current.position - previous.position) * clamped_alpha,
        .forward = forward,
        .up = up,
        .turn_roll_radians =
            previous.turn_roll_radians +
            (current.turn_roll_radians - previous.turn_roll_radians) * clamped_alpha,
    };
}

math::Vec3 forward_direction(const VehiclePose& pose) { return pose.forward; }

math::Vec3 up_direction(const VehiclePose& pose) { return pose.up; }

math::Mat4 model_matrix(const VehiclePose& pose) {
    const math::Vec3 right = math::normalized(math::cross(pose.up, pose.forward));
    const math::Vec3 up = math::normalized(math::cross(pose.forward, right));
    math::Mat4 orientation = math::identity();
    orientation.at(0, 0) = right.x;
    orientation.at(1, 0) = right.y;
    orientation.at(2, 0) = right.z;
    orientation.at(0, 1) = up.x;
    orientation.at(1, 1) = up.y;
    orientation.at(2, 1) = up.z;
    orientation.at(0, 2) = pose.forward.x;
    orientation.at(1, 2) = pose.forward.y;
    orientation.at(2, 2) = pose.forward.z;
    return math::translation(pose.position) * orientation *
           math::rotation_z(pose.turn_roll_radians);
}

} // namespace hover::game
