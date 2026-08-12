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

float drift_direction(const input::PlayerInput& input) {
    return static_cast<float>(input.drift_right) - static_cast<float>(input.drift_left);
}

void transport_basis_to_surface(PhysicalVehicleBasis& basis, const TrackFrame& frame) {
    const math::Vec3 projected_forward =
        basis.forward - frame.normal * math::dot(basis.forward, frame.normal);
    basis.forward = length_squared(projected_forward) > 0.000001F
                        ? math::normalized(projected_forward)
                        : frame.tangent;
    basis.up = frame.normal;
}

float clamp_lateral_offset(float requested, const TrackFrame& frame,
                           const LocalBoxCollider& collider) {
    const float minimum = -frame.half_width_metres - collider.center.x + collider.half_extents.x;
    const float maximum = frame.half_width_metres - collider.center.x - collider.half_extents.x;
    assert(minimum <= maximum);
    return std::clamp(requested, minimum, maximum);
}

void remove_constrained_velocity(PhysicalVehicleState& physical, const TrackFrame& frame,
                                 float requested_lateral, float constrained_lateral) {
    const float lateral_speed = math::dot(physical.velocity, frame.binormal);
    const bool stopped_at_left = constrained_lateral > requested_lateral && lateral_speed < 0.0F;
    const bool stopped_at_right = constrained_lateral < requested_lateral && lateral_speed > 0.0F;
    if (stopped_at_left || stopped_at_right) {
        physical.velocity = physical.velocity - frame.binormal * lateral_speed;
    }

    const float normal_speed = math::dot(physical.velocity, frame.normal);
    physical.velocity = physical.velocity - frame.normal * normal_speed;
}

void update_presentation_pose(WorldTrackVehicleState& state) {
    state.vehicle.pose.position = state.physical.position;
    state.vehicle.pose.forward = state.physical.basis.forward;
    state.vehicle.pose.up = state.physical.basis.up;
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
           std::isfinite(reference.height_above_surface_metres) &&
           reference.height_above_surface_metres >= 0.0F && is_valid(reference.frame);
}

bool is_valid(const WorldTrackVehicleState& state) {
    return is_valid(state.physical) && is_valid(state.course) && is_valid(state.vehicle);
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
    update_presentation_pose(state);
    assert(is_valid(state));
    return state;
}

VehicleTickEvents simulate_world_track_vehicle(WorldTrackVehicleState& state,
                                               const WorldTrackVehicleTick& tick) {
    assert(tick.tick_seconds > 0.0F);
    assert(is_valid(tick.path.id));
    assert(state.course.location.path == tick.path.id);
    assert(is_valid(tick.path.geometry));
    assert(is_valid(tick.definition));
    assert(is_valid(state));

    const HandlingProfile& handling = tick.definition.handling;
    const float drift = drift_direction(tick.input);
    const bool drift_active = drift != 0.0F;
    const float speed_ratio = std::clamp(state.vehicle.forward_speed_metres_per_second /
                                             handling.base_maximum_forward_speed_metres_per_second,
                                         0.0F, 1.0F);
    const float steering_authority = 0.60F + 0.40F * std::sqrt(speed_ratio);
    const float drift_steering = drift_active ? handling.world_drift_steering_multiplier : 1.0F;
    const float steering_radians = tick.input.steering * handling.steering_rate_radians_per_second *
                                   steering_authority * drift_steering * tick.tick_seconds;
    state.physical.basis.forward = math::normalized(rotate_around_axis(
        state.physical.basis.forward, state.physical.basis.up, steering_radians));

    const math::Vec3 vehicle_right = right_direction(state.physical.basis);
    const float previous_forward_speed =
        std::max(0.0F, math::dot(state.physical.velocity, state.physical.basis.forward));
    state.vehicle.forward_speed_metres_per_second = previous_forward_speed;
    const VehicleTickEvents events = simulate_vehicle_dynamics(
        state.vehicle, VehicleTick{tick.input, tick.definition, tick.tick_seconds});
    const float forward_speed_change =
        state.vehicle.forward_speed_metres_per_second - previous_forward_speed;
    state.physical.velocity =
        state.physical.velocity + state.physical.basis.forward * forward_speed_change;

    if (drift_active) {
        state.physical.velocity =
            state.physical.velocity +
            vehicle_right *
                (drift * handling.world_drift_lateral_acceleration_metres_per_second_squared *
                 tick.tick_seconds);
        const float forward_speed =
            std::max(0.0F, math::dot(state.physical.velocity, state.physical.basis.forward));
        const float drift_deceleration = std::min(
            forward_speed, handling.world_drift_forward_deceleration_metres_per_second_squared *
                               tick.tick_seconds);
        state.physical.velocity =
            state.physical.velocity - state.physical.basis.forward * drift_deceleration;
    }

    const float lateral_speed = math::dot(state.physical.velocity, vehicle_right);
    const float grip = drift_active
                           ? handling.world_drift_grip_deceleration_metres_per_second_squared
                           : handling.world_lateral_grip_deceleration_metres_per_second_squared;
    const float gripped_lateral_speed = move_toward_zero(lateral_speed, grip * tick.tick_seconds);
    state.physical.velocity =
        state.physical.velocity + vehicle_right * (gripped_lateral_speed - lateral_speed);

    const math::Vec3 candidate_position =
        state.physical.position + state.physical.velocity * tick.tick_seconds;
    const float search_radius =
        std::max(minimum_projection_search_radius_metres,
                 length(state.physical.velocity) * tick.tick_seconds * 2.0F + 2.0F);
    const TrackProjection projection =
        project_point_onto_track(tick.path.geometry, candidate_position,
                                 state.course.location.distance_along_path_metres, search_radius);
    const float constrained_lateral = clamp_lateral_offset(
        projection.offset.lateral_metres, projection.frame, tick.definition.collision.local_bounds);
    remove_constrained_velocity(state.physical, projection.frame, projection.offset.lateral_metres,
                                constrained_lateral);

    state.course = ProjectedCourseReference{
        .location =
            TrackLocation{
                .path = tick.path.id,
                .distance_along_path_metres = projection.frame.distance_metres,
                .lateral_offset_metres = constrained_lateral,
            },
        .height_above_surface_metres = handling.track_ride_height_metres,
        .frame = projection.frame,
    };
    state.physical.position = point_on_track_frame(
        projection.frame, TrackOffset{constrained_lateral, handling.track_ride_height_metres});
    transport_basis_to_surface(state.physical.basis, projection.frame);
    state.vehicle.forward_speed_metres_per_second =
        std::max(0.0F, math::dot(state.physical.velocity, state.physical.basis.forward));
    update_presentation_pose(state);
    assert(is_valid(state));
    return events;
}

} // namespace hover::game
