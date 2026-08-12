#include "game/world_track_vehicle_simulation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace hover::game {
namespace {

constexpr float basis_tolerance = 0.002F;
constexpr float minimum_projection_search_radius_metres = 8.0F;

bool is_finite(math::Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

float length_squared(math::Vec3 value) { return math::dot(value, value); }

float length(math::Vec3 value) { return std::sqrt(length_squared(value)); }

math::Vec3 right_direction(const PhysicalVehicleBasis& basis) {
    return math::normalized(math::cross(basis.up, basis.forward));
}

math::Vec3 rotate_around_axis(math::Vec3 value, math::Vec3 unit_axis, float radians) {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return value * cosine + math::cross(unit_axis, value) * sine +
           unit_axis * (math::dot(unit_axis, value) * (1.0F - cosine));
}

float move_toward_zero(float value, float maximum_change) {
    if (value > 0.0F) {
        return std::max(0.0F, value - maximum_change);
    }
    return std::min(0.0F, value + maximum_change);
}

float smoothstep(float edge0, float edge1, float value) {
    const float normalized = std::clamp((value - edge0) / (edge1 - edge0), 0.0F, 1.0F);
    return normalized * normalized * (3.0F - 2.0F * normalized);
}

float drift_direction(const input::PlayerInput& input) {
    return static_cast<float>(input.drift_right) - static_cast<float>(input.drift_left);
}

void smooth_basis_up(PhysicalVehicleBasis& basis, math::Vec3 target_up, float response_per_second,
                     float tick_seconds) {
    const float blend = 1.0F - std::exp(-response_per_second * tick_seconds);
    const math::Vec3 blended_up = math::normalized(basis.up * (1.0F - blend) + target_up * blend);
    const math::Vec3 projected_forward =
        basis.forward - blended_up * math::dot(basis.forward, blended_up);
    basis.forward = length_squared(projected_forward) > 0.000001F
                        ? math::normalized(projected_forward)
                        : math::normalized(math::cross(right_direction(basis), blended_up));
    basis.up = blended_up;
}

float clamp_lateral_offset(float requested, const TrackFrame& frame,
                           const LocalBoxCollider& collider) {
    const float minimum = -frame.half_width_metres - collider.center.x + collider.half_extents.x;
    const float maximum = frame.half_width_metres - collider.center.x - collider.half_extents.x;
    assert(minimum <= maximum);
    return std::clamp(requested, minimum, maximum);
}

struct ColliderLateralExtent {
    float center;
    float radius;
};

ColliderLateralExtent collider_lateral_extent(const PhysicalVehicleState& physical,
                                              const TrackFrame& frame,
                                              const LocalBoxCollider& collider) {
    const math::Vec3 right = right_direction(physical.basis);
    const math::Vec3 collider_center = physical.position + right * collider.center.x +
                                       physical.basis.up * collider.center.y +
                                       physical.basis.forward * collider.center.z;
    const float radius =
        std::abs(math::dot(right, frame.binormal)) * collider.half_extents.x +
        std::abs(math::dot(physical.basis.up, frame.binormal)) * collider.half_extents.y +
        std::abs(math::dot(physical.basis.forward, frame.binormal)) * collider.half_extents.z;
    return ColliderLateralExtent{
        .center = math::dot(collider_center - frame.center, frame.binormal),
        .radius = radius,
    };
}

ColliderLateralExtent collider_normal_extent(const PhysicalVehicleState& physical,
                                             const TrackFrame& frame,
                                             const LocalBoxCollider& collider) {
    const math::Vec3 right = right_direction(physical.basis);
    const math::Vec3 collider_center = physical.position + right * collider.center.x +
                                       physical.basis.up * collider.center.y +
                                       physical.basis.forward * collider.center.z;
    const float radius =
        std::abs(math::dot(right, frame.normal)) * collider.half_extents.x +
        std::abs(math::dot(physical.basis.up, frame.normal)) * collider.half_extents.y +
        std::abs(math::dot(physical.basis.forward, frame.normal)) * collider.half_extents.z;
    return ColliderLateralExtent{
        .center = math::dot(collider_center - frame.center, frame.normal),
        .radius = radius,
    };
}

void update_presentation_pose(WorldTrackVehicleState& state) {
    state.vehicle.pose.position = state.physical.position;
    state.vehicle.pose.forward = state.physical.basis.forward;
    state.vehicle.pose.up = state.physical.basis.up;
}

struct SteeringStageResult {
    math::Vec3 vehicle_right;
    float direction_change_radians;
    float direction_change_ratio;
};

SteeringStageResult apply_steering(WorldTrackVehicleState& state, const WorldTrackVehicleTick& tick,
                                   bool drift_active) {
    const HandlingProfile& handling = tick.definition.handling;
    const float speed_ratio = std::clamp(state.vehicle.forward_speed_metres_per_second /
                                             handling.base_maximum_forward_speed_metres_per_second,
                                         0.0F, 1.0F);
    const float steering_authority = 0.60F + 0.40F * std::sqrt(speed_ratio);
    const float drift_steering = drift_active ? handling.world_drift_steering_multiplier : 1.0F;
    const float steering_radians = tick.input.steering * handling.steering_rate_radians_per_second *
                                   steering_authority * drift_steering * tick.tick_seconds;
    const math::Vec3 previous_forward = state.physical.basis.forward;
    state.physical.basis.forward = math::normalized(rotate_around_axis(
        state.physical.basis.forward, state.physical.basis.up, steering_radians));
    const float direction_change_radians = std::atan2(
        length(math::cross(previous_forward, state.physical.basis.forward)),
        std::clamp(math::dot(previous_forward, state.physical.basis.forward), -1.0F, 1.0F));
    const float maximum_direction_change = handling.steering_rate_radians_per_second *
                                           steering_authority * drift_steering * tick.tick_seconds;
    return SteeringStageResult{
        .vehicle_right = right_direction(state.physical.basis),
        .direction_change_radians = direction_change_radians,
        .direction_change_ratio =
            maximum_direction_change > 0.0F
                ? std::clamp(direction_change_radians / maximum_direction_change, 0.0F, 1.0F)
                : 0.0F,
    };
}

struct LocalDampingStageResult {
    float forward_deceleration;
    float lateral_deceleration;
    float normal_deceleration;
};

LocalDampingStageResult apply_local_axis_damping(WorldTrackVehicleState& state,
                                                 const WorldTrackVehicleTick& tick,
                                                 math::Vec3 vehicle_right) {
    const HandlingProfile& handling = tick.definition.handling;
    const float forward_before = math::dot(state.physical.velocity, state.physical.basis.forward);
    const float lateral_before = math::dot(state.physical.velocity, vehicle_right);
    const float normal_before = math::dot(state.physical.velocity, state.physical.basis.up);
    const float forward_after =
        forward_before * std::exp(-handling.world_forward_damping_per_second * tick.tick_seconds);
    const float lateral_after =
        lateral_before * std::exp(-handling.world_lateral_damping_per_second * tick.tick_seconds);
    const float normal_damping = state.contact.mode == VehicleContactMode::supported
                                     ? 0.0F
                                     : handling.world_normal_damping_per_second;
    const float normal_after = normal_before * std::exp(-normal_damping * tick.tick_seconds);
    state.physical.velocity = state.physical.basis.forward * forward_after +
                              vehicle_right * lateral_after +
                              state.physical.basis.up * normal_after;
    return LocalDampingStageResult{
        .forward_deceleration = std::abs(forward_before - forward_after) / tick.tick_seconds,
        .lateral_deceleration = std::abs(lateral_before - lateral_after) / tick.tick_seconds,
        .normal_deceleration = std::abs(normal_before - normal_after) / tick.tick_seconds,
    };
}

struct DriftStageResult {
    float direction;
    float force_fraction;
    bool active;
};

DriftStageResult resolve_drift(const WorldTrackVehicleState& state,
                               const WorldTrackVehicleTick& tick, math::Vec3 vehicle_right) {
    const float direction = drift_direction(tick.input);
    const bool active = direction != 0.0F && state.contact.mode == VehicleContactMode::supported;
    const float lateral_speed = math::dot(state.physical.velocity, vehicle_right);
    const float same_direction_speed = std::max(0.0F, direction * lateral_speed);
    const float force_fraction =
        active ? 1.0F - std::clamp(same_direction_speed /
                                       tick.definition.handling
                                           .world_drift_force_fade_lateral_speed_metres_per_second,
                                   0.0F, 1.0F)
               : 0.0F;
    return DriftStageResult{direction, force_fraction, active};
}

struct GroundedForcesStageResult {
    float selected_grip;
    float available_grip;
    float grip_demand;
    float saturation_ratio;
    float lateral_speed_after_grip;
};

GroundedForcesStageResult apply_drift_and_grip(WorldTrackVehicleState& state,
                                               const WorldTrackVehicleTick& tick,
                                               const DriftStageResult& drift,
                                               math::Vec3 vehicle_right) {
    const HandlingProfile& handling = tick.definition.handling;
    const bool supported = state.contact.mode == VehicleContactMode::supported;
    if (supported && drift.active) {
        state.physical.velocity =
            state.physical.velocity +
            vehicle_right * (drift.direction * drift.force_fraction *
                             handling.world_drift_lateral_acceleration_metres_per_second_squared *
                             tick.tick_seconds);
    }

    const float lateral_speed = math::dot(state.physical.velocity, vehicle_right);
    const float selected_grip =
        drift.active ? handling.world_drift_grip_deceleration_metres_per_second_squared
                     : handling.world_lateral_grip_deceleration_metres_per_second_squared;
    if (!supported) {
        return GroundedForcesStageResult{
            .selected_grip = selected_grip,
            .available_grip = 0.0F,
            .grip_demand = 0.0F,
            .saturation_ratio = 0.0F,
            .lateral_speed_after_grip = lateral_speed,
        };
    }

    const float forward_speed =
        std::max(0.0F, math::dot(state.physical.velocity, state.physical.basis.forward));
    const float risk_start = handling.base_maximum_forward_speed_metres_per_second *
                             handling.world_traction_risk_start_speed_fraction;
    const float risk_end = handling.base_maximum_forward_speed_metres_per_second *
                           handling.boost_maximum_speed_multiplier;
    const float speed_risk = smoothstep(risk_start, risk_end, forward_speed);
    const float speed_multiplier =
        1.0F + (handling.world_high_speed_grip_multiplier - 1.0F) * speed_risk;
    const float slip_multiplier = 1.0F + (handling.world_sustained_slip_grip_multiplier - 1.0F) *
                                             state.handling.sustained_slip_intensity;
    constexpr float active_input_threshold = 0.001F;
    float recovery_multiplier = 1.0F;
    if (tick.input.brake > active_input_threshold) {
        recovery_multiplier =
            1.0F + (handling.world_braking_grip_multiplier - 1.0F) * tick.input.brake;
    } else if (tick.input.throttle <= active_input_threshold) {
        recovery_multiplier = handling.world_lift_off_grip_multiplier;
    }
    const float available_grip =
        selected_grip * speed_multiplier * slip_multiplier * recovery_multiplier;
    const float grip_demand = std::abs(lateral_speed) / tick.tick_seconds;
    const float gripped_lateral_speed =
        move_toward_zero(lateral_speed, available_grip * tick.tick_seconds);
    state.physical.velocity =
        state.physical.velocity + vehicle_right * (gripped_lateral_speed - lateral_speed);

    return GroundedForcesStageResult{
        .selected_grip = selected_grip,
        .available_grip = available_grip,
        .grip_demand = grip_demand,
        .saturation_ratio = available_grip > 0.0F ? grip_demand / available_grip : 0.0F,
        .lateral_speed_after_grip = gripped_lateral_speed,
    };
}

struct SustainedSlipStageResult {
    float seconds;
    float intensity;
};

SustainedSlipStageResult update_sustained_slip(WorldTrackVehicleState& state,
                                               const WorldTrackVehicleTick& tick,
                                               float lateral_speed) {
    const HandlingProfile& handling = tick.definition.handling;
    state.handling.sustained_slip_intensity =
        std::max(0.0F, state.handling.sustained_slip_intensity -
                           tick.tick_seconds / handling.world_sustained_slip_release_seconds);
    if (std::abs(lateral_speed) > handling.world_slip_speed_threshold_metres_per_second) {
        state.handling.sustained_slip_seconds += tick.tick_seconds;
        const float buildup = std::clamp(state.handling.sustained_slip_seconds /
                                             handling.world_sustained_slip_buildup_seconds,
                                         0.0F, 1.0F);
        state.handling.sustained_slip_intensity =
            std::max(state.handling.sustained_slip_intensity, buildup);
    } else {
        state.handling.sustained_slip_seconds = 0.0F;
    }
    return SustainedSlipStageResult{
        state.handling.sustained_slip_seconds,
        state.handling.sustained_slip_intensity,
    };
}

struct PropulsionStageResult {
    float curve_multiplier;
    float requested_acceleration;
    float applied_acceleration;
    float applied_fraction;
    float post_boost_return_deceleration;
};

PropulsionStageResult
apply_propulsion(WorldTrackVehicleState& state, const WorldTrackVehicleTick& tick,
                 const VehicleBoostTickResult& boost, const SteeringStageResult& steering,
                 const DriftStageResult& drift, const SustainedSlipStageResult& slip) {
    constexpr float active_input_threshold = 0.001F;
    const HandlingProfile& handling = tick.definition.handling;
    const float forward_speed =
        std::max(0.0F, math::dot(state.physical.velocity, state.physical.basis.forward));
    const float speed_ratio = std::clamp(
        forward_speed / handling.base_maximum_forward_speed_metres_per_second, 0.0F, 1.0F);
    const float curve_progress =
        smoothstep(handling.world_propulsion_curve_knee_speed_fraction, 1.0F, speed_ratio);
    const float curve_multiplier =
        1.0F + (handling.world_propulsion_high_speed_multiplier - 1.0F) * curve_progress;
    const bool throttle_active = tick.input.throttle > active_input_threshold;
    const bool brake_active = tick.input.brake > active_input_threshold;
    const float boost_acceleration =
        boost.active_for_tick ? handling.boost_acceleration_metres_per_second_squared : 0.0F;
    const float requested_acceleration =
        throttle_active
            ? tick.input.throttle *
                  (handling.forward_acceleration_metres_per_second_squared + boost_acceleration) *
                  curve_multiplier
            : 0.0F;

    const float steering_loss =
        steering.direction_change_ratio * handling.world_steering_propulsion_loss_fraction;
    const float drift_loss = drift.force_fraction * handling.world_drift_propulsion_loss_fraction;
    const float control_fraction = 1.0F - std::clamp(steering_loss + drift_loss, 0.0F, 1.0F);
    const float slip_fraction =
        1.0F + (handling.world_sustained_slip_full_propulsion_multiplier - 1.0F) * slip.intensity;
    const float applied_fraction = control_fraction * slip_fraction;
    const float target_acceleration = requested_acceleration * applied_fraction;

    if (!throttle_active || brake_active ||
        target_acceleration <=
            state.handling.applied_propulsion_acceleration_metres_per_second_squared) {
        state.handling.applied_propulsion_acceleration_metres_per_second_squared =
            brake_active ? 0.0F : target_acceleration;
    } else {
        const float response_rate =
            handling.world_propulsion_response_rate_at_rest_per_second +
            (handling.world_propulsion_response_rate_at_base_speed_per_second -
             handling.world_propulsion_response_rate_at_rest_per_second) *
                speed_ratio;
        const float response = 1.0F - std::exp(-response_rate * tick.tick_seconds);
        state.handling.applied_propulsion_acceleration_metres_per_second_squared +=
            (target_acceleration -
             state.handling.applied_propulsion_acceleration_metres_per_second_squared) *
            response;
    }

    const float braking_deceleration =
        tick.input.brake * handling.braking_deceleration_metres_per_second_squared;
    const float post_boost_return_deceleration =
        !boost.active_for_tick &&
                forward_speed > handling.base_maximum_forward_speed_metres_per_second
            ? handling.world_boost_excess_speed_decay_metres_per_second_squared
            : 0.0F;
    const float longitudinal_acceleration =
        state.handling.applied_propulsion_acceleration_metres_per_second_squared -
        braking_deceleration - post_boost_return_deceleration;
    const float requested_forward_speed =
        std::max(0.0F, forward_speed + longitudinal_acceleration * tick.tick_seconds);
    const float boosted_speed_limit = handling.base_maximum_forward_speed_metres_per_second *
                                      handling.boost_maximum_speed_multiplier;
    const float speed_limit =
        boost.active_for_tick
            ? boosted_speed_limit
            : std::max(handling.base_maximum_forward_speed_metres_per_second, forward_speed);
    const float constrained_forward_speed = std::min(requested_forward_speed, speed_limit);
    state.physical.velocity =
        state.physical.velocity +
        state.physical.basis.forward * (constrained_forward_speed - forward_speed);

    return PropulsionStageResult{
        .curve_multiplier = curve_multiplier,
        .requested_acceleration = requested_acceleration,
        .applied_acceleration =
            state.handling.applied_propulsion_acceleration_metres_per_second_squared,
        .applied_fraction = requested_acceleration > 0.0F ? applied_fraction : 1.0F,
        .post_boost_return_deceleration = post_boost_return_deceleration,
    };
}

void apply_gravity_and_hover(WorldTrackVehicleState& state, const WorldTrackVehicleTick& tick) {
    const HandlingProfile& handling = tick.definition.handling;
    const math::Vec3 gravity =
        state.contact.gravity_up * -handling.world_gravity_acceleration_metres_per_second_squared;
    math::Vec3 acceleration = gravity;
    if (state.contact.mode == VehicleContactMode::supported) {
        const math::Vec3 surface_normal = state.course.frame.normal;
        const float normal_speed = math::dot(state.physical.velocity, surface_normal);
        const float height_error =
            handling.track_ride_height_metres - state.course.height_above_surface_metres;
        const float gravity_compensation =
            handling.world_gravity_acceleration_metres_per_second_squared *
            std::max(0.0F, math::dot(state.contact.gravity_up, surface_normal));
        const float hover_acceleration = std::clamp(
            gravity_compensation + handling.world_hover_spring_per_second_squared * height_error -
                handling.world_hover_damping_per_second * normal_speed,
            0.0F, handling.world_maximum_hover_acceleration_metres_per_second_squared);
        acceleration = acceleration + surface_normal * hover_acceleration;
    }
    state.physical.velocity = state.physical.velocity + acceleration * tick.tick_seconds;
}

struct ContactStageResult {
    bool edge_constraint_activated = false;
    bool wall_impact = false;
    bool support_lost = false;
    bool landed = false;
    bool recovered = false;
    float wall_impact_speed = 0.0F;
};

float resolve_wall_impact(PhysicalVehicleState& physical, math::Vec3 outward_normal,
                          const HandlingProfile& handling) {
    const float outward_speed = math::dot(physical.velocity, outward_normal);
    if (outward_speed <= 0.0F) {
        return 0.0F;
    }
    const math::Vec3 tangential_velocity = physical.velocity - outward_normal * outward_speed;
    physical.velocity = tangential_velocity * handling.world_wall_tangent_retention -
                        outward_normal * (outward_speed * handling.world_wall_restitution);
    return outward_speed;
}

void recover_to_last_safe_pose(WorldTrackVehicleState& state, const WorldTrackVehicleTick& tick) {
    assert(state.contact.has_last_safe_pose);
    state.physical = state.contact.last_safe_physical;
    state.course = state.contact.last_safe_course;
    state.physical.velocity =
        state.physical.basis.forward *
        (tick.definition.handling.base_maximum_forward_speed_metres_per_second *
         tick.definition.handling.world_recovery_speed_fraction);
    state.contact.mode = VehicleContactMode::supported;
    state.contact.gravity_up = state.course.frame.normal;
    state.contact.unsupported_seconds = 0.0F;
    state.contact.drop_from_last_safe_metres = 0.0F;
    state.handling = {};
    state.vehicle.boost_seconds_remaining = 0.0F;
    state.vehicle.boosting = false;
    state.vehicle.boost_input_was_down = tick.input.boost;
}

ContactStageResult integrate_and_resolve_contact(WorldTrackVehicleState& state,
                                                 const WorldTrackVehicleTick& tick) {
    ContactStageResult result{};
    state.physical.position = state.physical.position + state.physical.velocity * tick.tick_seconds;
    const float search_radius =
        std::max(minimum_projection_search_radius_metres,
                 length(state.physical.velocity) * tick.tick_seconds * 2.0F + 2.0F);
    const TrackProjection projection =
        project_point_onto_track(tick.path.geometry, state.physical.position,
                                 state.course.location.distance_along_path_metres, search_radius);
    const TrackSegmentProperties properties =
        tick.path.geometry.segment_properties[projection.segment_index];
    const HandlingProfile& handling = tick.definition.handling;
    const LocalBoxCollider& collider = tick.definition.collision.local_bounds;
    const VehicleContactMode previous_mode = state.contact.mode;

    if (previous_mode == VehicleContactMode::supported) {
        ColliderLateralExtent extent =
            collider_lateral_extent(state.physical, projection.frame, collider);
        const float left_penetration =
            -projection.frame.half_width_metres - (extent.center - extent.radius);
        if (left_penetration > 0.0F && properties.left_edge == TrackEdgePolicy::solid_wall) {
            state.physical.position =
                state.physical.position + projection.frame.binormal * left_penetration;
            result.edge_constraint_activated = true;
            result.wall_impact_speed = std::max(
                result.wall_impact_speed,
                resolve_wall_impact(state.physical, projection.frame.binormal * -1.0F, handling));
        }

        extent = collider_lateral_extent(state.physical, projection.frame, collider);
        const float right_penetration =
            extent.center + extent.radius - projection.frame.half_width_metres;
        if (right_penetration > 0.0F && properties.right_edge == TrackEdgePolicy::solid_wall) {
            state.physical.position =
                state.physical.position - projection.frame.binormal * right_penetration;
            result.edge_constraint_activated = true;
            result.wall_impact_speed =
                std::max(result.wall_impact_speed,
                         resolve_wall_impact(state.physical, projection.frame.binormal, handling));
        }
        result.wall_impact = result.wall_impact_speed > 0.0F;
    }

    const math::Vec3 displacement = state.physical.position - projection.frame.center;
    const float lateral = math::dot(displacement, projection.frame.binormal);
    float height = math::dot(displacement, projection.frame.normal);
    const float normal_speed = math::dot(state.physical.velocity, projection.frame.normal);
    const ColliderLateralExtent corrected_extent =
        collider_lateral_extent(state.physical, projection.frame, collider);
    const bool beyond_open_left = corrected_extent.center < -projection.frame.half_width_metres &&
                                  properties.left_edge == TrackEdgePolicy::open;
    const bool beyond_open_right = corrected_extent.center > projection.frame.half_width_metres &&
                                   properties.right_edge == TrackEdgePolicy::open;

    state.course = ProjectedCourseReference{
        .location =
            TrackLocation{
                .path = tick.path.id,
                .distance_along_path_metres = projection.frame.distance_metres,
                .lateral_offset_metres = lateral,
            },
        .height_above_surface_metres = height,
        .frame = projection.frame,
    };

    if (previous_mode == VehicleContactMode::crashed) {
        state.contact.mode = VehicleContactMode::crashed;
    } else if (previous_mode == VehicleContactMode::falling) {
        state.contact.mode = VehicleContactMode::falling;
    } else if (beyond_open_left || beyond_open_right) {
        state.contact.mode = VehicleContactMode::falling;
        result.support_lost = previous_mode == VehicleContactMode::supported;
    } else if (previous_mode == VehicleContactMode::supported &&
               (height > handling.track_ride_height_metres +
                             handling.world_support_detach_height_metres ||
                normal_speed > handling.world_takeoff_normal_speed_metres_per_second)) {
        state.contact.mode = VehicleContactMode::airborne;
        result.support_lost = true;
    } else if (previous_mode == VehicleContactMode::airborne && normal_speed <= 0.0F &&
               height <= handling.track_ride_height_metres +
                             handling.world_support_landing_height_metres) {
        state.contact.mode = VehicleContactMode::supported;
        result.landed = true;
    }

    if (state.contact.mode == VehicleContactMode::supported) {
        const ColliderLateralExtent normal_extent =
            collider_normal_extent(state.physical, projection.frame, collider);
        const float penetration = normal_extent.radius - normal_extent.center;
        if (penetration > 0.0F) {
            state.physical.position =
                state.physical.position + projection.frame.normal * penetration;
            height += penetration;
            state.course.height_above_surface_metres = height;
            const float inward_speed = math::dot(state.physical.velocity, projection.frame.normal);
            if (inward_speed < 0.0F) {
                state.physical.velocity =
                    state.physical.velocity - projection.frame.normal * inward_speed;
            }
        }
        state.contact.gravity_up = projection.frame.normal;
        state.contact.unsupported_seconds = 0.0F;
        state.contact.drop_from_last_safe_metres = 0.0F;
        smooth_basis_up(state.physical.basis, projection.frame.normal,
                        handling.world_supported_up_response_per_second, tick.tick_seconds);

        const ColliderLateralExtent safe_extent =
            collider_lateral_extent(state.physical, projection.frame, collider);
        if (std::abs(safe_extent.center) + safe_extent.radius +
                handling.world_recovery_safe_margin_metres <=
            projection.frame.half_width_metres) {
            state.contact.last_safe_physical = state.physical;
            state.contact.last_safe_course = state.course;
            state.contact.has_last_safe_pose = true;
        }
    } else {
        state.contact.unsupported_seconds += tick.tick_seconds;
        const math::Vec3 world_up{0.0F, 1.0F, 0.0F};
        const float gravity_blend =
            1.0F -
            std::exp(-handling.world_airborne_gravity_up_response_per_second * tick.tick_seconds);
        state.contact.gravity_up = math::normalized(
            state.contact.gravity_up * (1.0F - gravity_blend) + world_up * gravity_blend);
        smooth_basis_up(state.physical.basis, state.contact.gravity_up,
                        handling.world_airborne_gravity_up_response_per_second, tick.tick_seconds);
        if (state.contact.has_last_safe_pose) {
            state.contact.drop_from_last_safe_metres = std::max(
                0.0F, math::dot(state.contact.last_safe_physical.position - state.physical.position,
                                state.contact.last_safe_course.frame.normal));
        }
    }

    if (state.contact.mode == VehicleContactMode::falling && state.contact.has_last_safe_pose &&
        (state.contact.unsupported_seconds >= handling.world_recovery_delay_seconds ||
         state.contact.drop_from_last_safe_metres >= handling.world_recovery_drop_metres)) {
        recover_to_last_safe_pose(state, tick);
        result.recovered = true;
    }

    return result;
}

WorldTrackVehicleTelemetry
make_telemetry(const WorldTrackVehicleState& state, const SteeringStageResult& steering,
               const LocalDampingStageResult& damping, const DriftStageResult& drift,
               const GroundedForcesStageResult& grounded_forces,
               const SustainedSlipStageResult& slip, const PropulsionStageResult& propulsion,
               const ContactStageResult& contact) {
    const math::Vec3 vehicle_right = right_direction(state.physical.basis);
    const float local_forward = math::dot(state.physical.velocity, state.physical.basis.forward);
    const float local_lateral = math::dot(state.physical.velocity, vehicle_right);
    const float local_normal = math::dot(state.physical.velocity, state.physical.basis.up);
    return WorldTrackVehicleTelemetry{
        .world_speed_metres_per_second = length(state.physical.velocity),
        .local_forward_speed_metres_per_second = local_forward,
        .local_lateral_speed_metres_per_second = local_lateral,
        .local_normal_speed_metres_per_second = local_normal,
        .signed_slip_angle_radians = std::atan2(local_lateral, local_forward),
        .steering_direction_change_radians = steering.direction_change_radians,
        .steering_direction_change_ratio = steering.direction_change_ratio,
        .drift_direction = drift.direction,
        .drift_force_fraction = drift.force_fraction,
        .selected_grip_deceleration_metres_per_second_squared = grounded_forces.selected_grip,
        .available_grip_deceleration_metres_per_second_squared = grounded_forces.available_grip,
        .grip_demand_deceleration_metres_per_second_squared = grounded_forces.grip_demand,
        .traction_saturation_ratio = grounded_forces.saturation_ratio,
        .forward_damping_deceleration_metres_per_second_squared = damping.forward_deceleration,
        .lateral_damping_deceleration_metres_per_second_squared = damping.lateral_deceleration,
        .normal_damping_deceleration_metres_per_second_squared = damping.normal_deceleration,
        .propulsion_curve_multiplier = propulsion.curve_multiplier,
        .sustained_slip_seconds = slip.seconds,
        .sustained_slip_intensity = slip.intensity,
        .requested_propulsion_acceleration_metres_per_second_squared =
            propulsion.requested_acceleration,
        .applied_propulsion_acceleration_metres_per_second_squared =
            propulsion.applied_acceleration,
        .propulsion_fraction = propulsion.applied_fraction,
        .post_boost_return_deceleration_metres_per_second_squared =
            propulsion.post_boost_return_deceleration,
        .height_above_surface_metres = state.course.height_above_surface_metres,
        .surface_normal_speed_metres_per_second =
            math::dot(state.physical.velocity, state.course.frame.normal),
        .wall_impact_speed_metres_per_second = contact.wall_impact_speed,
        .contact_mode = state.contact.mode,
        .edge_constraint_activated = contact.edge_constraint_activated,
    };
}

} // namespace

bool is_valid(const PhysicalVehicleBasis& basis) {
    return is_finite(basis.forward) && is_finite(basis.up) &&
           std::abs(length_squared(basis.forward) - 1.0F) <= basis_tolerance &&
           std::abs(length_squared(basis.up) - 1.0F) <= basis_tolerance &&
           std::abs(math::dot(basis.forward, basis.up)) <= basis_tolerance;
}

bool is_valid(const PhysicalVehicleState& state) {
    return is_finite(state.position) && is_finite(state.velocity) && is_valid(state.basis);
}

bool is_valid(const ProjectedCourseReference& reference) {
    return is_valid(reference.location.path) &&
           std::isfinite(reference.location.distance_along_path_metres) &&
           reference.location.distance_along_path_metres >= 0.0F &&
           std::isfinite(reference.location.lateral_offset_metres) &&
           std::isfinite(reference.height_above_surface_metres) && is_valid(reference.frame);
}

bool is_valid(const SurfaceContactState& state) {
    const bool mode_valid =
        state.mode == VehicleContactMode::supported || state.mode == VehicleContactMode::airborne ||
        state.mode == VehicleContactMode::falling || state.mode == VehicleContactMode::crashed;
    return mode_valid && is_finite(state.gravity_up) &&
           std::abs(length_squared(state.gravity_up) - 1.0F) <= basis_tolerance &&
           std::isfinite(state.unsupported_seconds) && state.unsupported_seconds >= 0.0F &&
           std::isfinite(state.drop_from_last_safe_metres) &&
           state.drop_from_last_safe_metres >= 0.0F &&
           (!state.has_last_safe_pose ||
            (is_valid(state.last_safe_physical) && is_valid(state.last_safe_course)));
}

bool is_valid(const HandlingRuntimeState& state) {
    return std::isfinite(state.sustained_slip_seconds) && state.sustained_slip_seconds >= 0.0F &&
           std::isfinite(state.sustained_slip_intensity) &&
           state.sustained_slip_intensity >= 0.0F && state.sustained_slip_intensity <= 1.0F &&
           std::isfinite(state.applied_propulsion_acceleration_metres_per_second_squared) &&
           state.applied_propulsion_acceleration_metres_per_second_squared >= 0.0F;
}

bool is_valid(const WorldTrackVehicleState& state) {
    return is_valid(state.physical) && is_valid(state.course) && is_valid(state.contact) &&
           is_valid(state.handling) && is_valid(state.vehicle);
}

bool is_valid(const WorldTrackVehicleTelemetry& telemetry) {
    return std::isfinite(telemetry.world_speed_metres_per_second) &&
           telemetry.world_speed_metres_per_second >= 0.0F &&
           std::isfinite(telemetry.local_forward_speed_metres_per_second) &&
           std::isfinite(telemetry.local_lateral_speed_metres_per_second) &&
           std::isfinite(telemetry.local_normal_speed_metres_per_second) &&
           std::isfinite(telemetry.signed_slip_angle_radians) &&
           std::isfinite(telemetry.steering_direction_change_radians) &&
           telemetry.steering_direction_change_radians >= 0.0F &&
           std::isfinite(telemetry.steering_direction_change_ratio) &&
           telemetry.steering_direction_change_ratio >= 0.0F &&
           telemetry.steering_direction_change_ratio <= 1.0F &&
           std::isfinite(telemetry.drift_direction) && telemetry.drift_direction >= -1.0F &&
           telemetry.drift_direction <= 1.0F && std::isfinite(telemetry.drift_force_fraction) &&
           telemetry.drift_force_fraction >= 0.0F && telemetry.drift_force_fraction <= 1.0F &&
           std::isfinite(telemetry.selected_grip_deceleration_metres_per_second_squared) &&
           telemetry.selected_grip_deceleration_metres_per_second_squared >= 0.0F &&
           std::isfinite(telemetry.available_grip_deceleration_metres_per_second_squared) &&
           telemetry.available_grip_deceleration_metres_per_second_squared >= 0.0F &&
           std::isfinite(telemetry.grip_demand_deceleration_metres_per_second_squared) &&
           telemetry.grip_demand_deceleration_metres_per_second_squared >= 0.0F &&
           std::isfinite(telemetry.traction_saturation_ratio) &&
           telemetry.traction_saturation_ratio >= 0.0F &&
           std::isfinite(telemetry.forward_damping_deceleration_metres_per_second_squared) &&
           telemetry.forward_damping_deceleration_metres_per_second_squared >= 0.0F &&
           std::isfinite(telemetry.lateral_damping_deceleration_metres_per_second_squared) &&
           telemetry.lateral_damping_deceleration_metres_per_second_squared >= 0.0F &&
           std::isfinite(telemetry.normal_damping_deceleration_metres_per_second_squared) &&
           telemetry.normal_damping_deceleration_metres_per_second_squared >= 0.0F &&
           std::isfinite(telemetry.propulsion_curve_multiplier) &&
           telemetry.propulsion_curve_multiplier > 0.0F &&
           telemetry.propulsion_curve_multiplier <= 1.0F &&
           std::isfinite(telemetry.sustained_slip_seconds) &&
           telemetry.sustained_slip_seconds >= 0.0F &&
           std::isfinite(telemetry.sustained_slip_intensity) &&
           telemetry.sustained_slip_intensity >= 0.0F &&
           telemetry.sustained_slip_intensity <= 1.0F &&
           std::isfinite(telemetry.requested_propulsion_acceleration_metres_per_second_squared) &&
           std::isfinite(telemetry.applied_propulsion_acceleration_metres_per_second_squared) &&
           std::isfinite(telemetry.propulsion_fraction) && telemetry.propulsion_fraction >= 0.0F &&
           telemetry.propulsion_fraction <= 1.0F &&
           std::isfinite(telemetry.post_boost_return_deceleration_metres_per_second_squared) &&
           telemetry.post_boost_return_deceleration_metres_per_second_squared >= 0.0F &&
           std::isfinite(telemetry.height_above_surface_metres) &&
           std::isfinite(telemetry.surface_normal_speed_metres_per_second) &&
           std::isfinite(telemetry.wall_impact_speed_metres_per_second) &&
           telemetry.wall_impact_speed_metres_per_second >= 0.0F;
}

WorldTrackVehicleState make_world_track_vehicle_state(WorldTrackVehicleSpawn spawn,
                                                      const ShipDefinition& definition,
                                                      ResolvedTrackPath path) {
    assert(is_valid(path.id));
    assert(is_valid(path.geometry));
    assert(is_valid(definition));
    assert(std::isfinite(spawn.distance_along_path_metres));
    assert(std::isfinite(spawn.lateral_offset_metres));

    const TrackFrame frame = sample_track(path.geometry, spawn.distance_along_path_metres);
    const float lateral =
        clamp_lateral_offset(spawn.lateral_offset_metres, frame, definition.collision.local_bounds);
    const float ride_height = definition.handling.track_ride_height_metres;

    WorldTrackVehicleState state{};
    state.course = ProjectedCourseReference{
        .location =
            TrackLocation{
                .path = path.id,
                .distance_along_path_metres = frame.distance_metres,
                .lateral_offset_metres = lateral,
            },
        .height_above_surface_metres = ride_height,
        .frame = frame,
    };
    state.physical.position = point_on_track_frame(frame, TrackOffset{lateral, ride_height});
    state.physical.basis = PhysicalVehicleBasis{frame.tangent, frame.normal};
    const float safe_minimum = -frame.half_width_metres -
                               definition.collision.local_bounds.center.x +
                               definition.collision.local_bounds.half_extents.x +
                               definition.handling.world_recovery_safe_margin_metres;
    const float safe_maximum = frame.half_width_metres -
                               definition.collision.local_bounds.center.x -
                               definition.collision.local_bounds.half_extents.x -
                               definition.handling.world_recovery_safe_margin_metres;
    const float safe_lateral =
        safe_minimum <= safe_maximum ? std::clamp(lateral, safe_minimum, safe_maximum) : 0.0F;
    PhysicalVehicleState safe_physical = state.physical;
    safe_physical.position = point_on_track_frame(frame, TrackOffset{safe_lateral, ride_height});
    ProjectedCourseReference safe_course = state.course;
    safe_course.location.lateral_offset_metres = safe_lateral;
    state.contact = SurfaceContactState{
        .mode = VehicleContactMode::supported,
        .gravity_up = frame.normal,
        .unsupported_seconds = 0.0F,
        .drop_from_last_safe_metres = 0.0F,
        .last_safe_physical = safe_physical,
        .last_safe_course = safe_course,
        .has_last_safe_pose = true,
    };
    update_presentation_pose(state);
    assert(is_valid(state));
    return state;
}

WorldTrackVehicleTickResult simulate_world_track_vehicle(WorldTrackVehicleState& state,
                                                         const WorldTrackVehicleTick& tick) {
    assert(tick.tick_seconds > 0.0F);
    assert(is_valid(tick.path.id));
    assert(state.course.location.path == tick.path.id);
    assert(is_valid(tick.path.geometry));
    assert(is_valid(tick.definition));
    assert(is_valid(state));

    const float drift = drift_direction(tick.input);
    const bool drift_active = drift != 0.0F && state.contact.mode == VehicleContactMode::supported;
    state.vehicle.forward_speed_metres_per_second =
        std::max(0.0F, math::dot(state.physical.velocity, state.physical.basis.forward));
    const VehicleBoostTickResult boost = advance_vehicle_boost_state(
        state.vehicle, VehicleTick{tick.input, tick.definition, tick.tick_seconds});

    const SteeringStageResult steering = apply_steering(state, tick, drift_active);
    const LocalDampingStageResult damping =
        apply_local_axis_damping(state, tick, steering.vehicle_right);
    const DriftStageResult drift_stage = resolve_drift(state, tick, steering.vehicle_right);
    const GroundedForcesStageResult grounded_forces =
        apply_drift_and_grip(state, tick, drift_stage, steering.vehicle_right);
    const SustainedSlipStageResult slip =
        update_sustained_slip(state, tick, grounded_forces.lateral_speed_after_grip);
    const PropulsionStageResult propulsion =
        apply_propulsion(state, tick, boost, steering, drift_stage, slip);
    apply_gravity_and_hover(state, tick);
    const ContactStageResult contact = integrate_and_resolve_contact(state, tick);
    state.vehicle.forward_speed_metres_per_second =
        std::max(0.0F, math::dot(state.physical.velocity, state.physical.basis.forward));
    update_vehicle_turn_roll(state.vehicle,
                             VehicleTick{tick.input, tick.definition, tick.tick_seconds});
    const WorldTrackVehicleTelemetry telemetry = make_telemetry(
        state, steering, damping, drift_stage, grounded_forces, slip, propulsion, contact);
    update_presentation_pose(state);
    assert(is_valid(state));
    assert(is_valid(telemetry));
    return WorldTrackVehicleTickResult{
        .events =
            WorldTrackVehicleTickEvents{
                .boost_activated = boost.events.boost_activated,
                .wall_impact = contact.wall_impact,
                .support_lost = contact.support_lost,
                .landed = contact.landed,
                .recovered = contact.recovered,
                .wall_impact_speed_metres_per_second = contact.wall_impact_speed,
            },
        .telemetry = telemetry,
    };
}

} // namespace hover::game
