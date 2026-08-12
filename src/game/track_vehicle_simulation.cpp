#include "game/track_vehicle_simulation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

namespace hover::game {
namespace {

constexpr float minimum_longitudinal_scale = 0.05F;

float wrap_heading(float radians) {
    return std::remainder(radians, 2.0F * std::numbers::pi_v<float>);
}

float signed_track_yaw(const TrackFrame& previous, const TrackFrame& current) {
    const math::Vec3 projected_previous_tangent =
        previous.tangent - current.normal * math::dot(previous.tangent, current.normal);
    const float projected_length_squared =
        math::dot(projected_previous_tangent, projected_previous_tangent);
    if (projected_length_squared < 0.000001F) {
        return 0.0F;
    }

    const math::Vec3 transported_tangent = math::normalized(projected_previous_tangent);
    const float sine = math::dot(math::cross(transported_tangent, current.tangent), current.normal);
    const float cosine = math::dot(transported_tangent, current.tangent);
    return std::atan2(sine, cosine);
}

float longitudinal_lane_scale(const SampledTrack& track, const TrackVehicleState& state) {
    const float probe_distance = std::min(0.5F, track.length_metres * 0.001F);
    const TrackOffset offset{state.location.lateral_offset_metres, state.normal_offset_metres};
    const math::Vec3 before = point_on_track_frame(
        sample_track(track, state.location.distance_along_path_metres - probe_distance), offset);
    const math::Vec3 after = point_on_track_frame(
        sample_track(track, state.location.distance_along_path_metres + probe_distance), offset);
    const math::Vec3 lane_delta = after - before;
    const float world_metres = std::sqrt(math::dot(lane_delta, lane_delta));
    return std::max(world_metres / (2.0F * probe_distance), minimum_longitudinal_scale);
}

void derive_attached_pose(TrackVehicleState& state, const SampledTrack& track) {
    const TrackFrame frame = sample_track(track, state.location.distance_along_path_metres);
    state.vehicle.pose.position = point_on_track_frame(
        frame, TrackOffset{state.location.lateral_offset_metres, state.normal_offset_metres});

    state.vehicle.pose.forward =
        math::normalized(frame.tangent * std::cos(state.heading_offset_radians) +
                         frame.binormal * std::sin(state.heading_offset_radians));
    state.vehicle.pose.up = frame.normal;
}

void constrain_to_track_width(TrackVehicleState& state, const TrackFrame& frame,
                              const LocalBoxCollider& collider) {
    const float minimum_lateral =
        -frame.half_width_metres - collider.center.x + collider.half_extents.x;
    const float maximum_lateral =
        frame.half_width_metres - collider.center.x - collider.half_extents.x;
    assert(minimum_lateral <= maximum_lateral);

    const float requested_lateral = state.location.lateral_offset_metres;
    state.location.lateral_offset_metres =
        std::clamp(requested_lateral, minimum_lateral, maximum_lateral);
    const bool stopped_at_left = state.location.lateral_offset_metres == minimum_lateral &&
                                 state.lateral_velocity_metres_per_second < 0.0F;
    const bool stopped_at_right = state.location.lateral_offset_metres == maximum_lateral &&
                                  state.lateral_velocity_metres_per_second > 0.0F;
    if (stopped_at_left || stopped_at_right) {
        state.lateral_velocity_metres_per_second = 0.0F;
    }
}

} // namespace

TrackVehicleState make_track_vehicle_state(TrackVehicleSpawn spawn,
                                           const ShipDefinition& definition,
                                           ResolvedTrackPath path) {
    assert(is_valid(path.id));
    assert(is_valid(path.geometry));
    assert(is_valid(definition));
    assert(std::isfinite(spawn.distance_along_path_metres));
    assert(std::isfinite(spawn.lateral_offset_metres));

    TrackVehicleState state{};
    state.location.path = path.id;
    state.location.distance_along_path_metres =
        wrap_track_distance(path.geometry, spawn.distance_along_path_metres);
    state.location.lateral_offset_metres = spawn.lateral_offset_metres;
    state.normal_offset_metres = definition.handling.track_ride_height_metres;
    const TrackFrame frame = sample_track(path.geometry, state.location.distance_along_path_metres);
    constrain_to_track_width(state, frame, definition.collision.local_bounds);
    derive_attached_pose(state, path.geometry);
    assert(is_valid(state));
    return state;
}

VehicleTickEvents simulate_track_vehicle(TrackVehicleState& state, const TrackVehicleTick& tick) {
    assert(tick.tick_seconds > 0.0F);
    assert(is_valid(tick.path.id));
    assert(state.location.path == tick.path.id);
    assert(is_valid(state));
    assert(state.location.distance_along_path_metres < tick.path.geometry.length_metres);

    const VehicleTickEvents events = simulate_vehicle_dynamics(
        state.vehicle, VehicleTick{tick.input, tick.definition, tick.tick_seconds});
    const HandlingProfile& handling = tick.definition.handling;
    const float speed_ratio = std::clamp(state.vehicle.forward_speed_metres_per_second /
                                             handling.base_maximum_forward_speed_metres_per_second,
                                         0.0F, 1.0F);
    const float steering_authority = 0.60F + 0.40F * std::sqrt(speed_ratio);
    state.heading_offset_radians =
        wrap_heading(state.heading_offset_radians + tick.input.steering *
                                                        handling.steering_rate_radians_per_second *
                                                        steering_authority * tick.tick_seconds);

    const float body_forward_speed = state.vehicle.forward_speed_metres_per_second;
    const float target_lateral_velocity =
        std::clamp(body_forward_speed * std::sin(state.heading_offset_radians),
                   -handling.maximum_lateral_speed_metres_per_second,
                   handling.maximum_lateral_speed_metres_per_second);
    const bool drift_active = tick.input.drift_left != tick.input.drift_right;
    const float lateral_grip = drift_active ? handling.drift_lateral_grip_per_second
                                            : handling.normal_lateral_grip_per_second;
    const float lateral_blend = 1.0F - std::exp(-lateral_grip * tick.tick_seconds);
    state.lateral_velocity_metres_per_second +=
        (target_lateral_velocity - state.lateral_velocity_metres_per_second) * lateral_blend;

    const TrackFrame previous_frame =
        sample_track(tick.path.geometry, state.location.distance_along_path_metres);
    const float path_forward_speed = body_forward_speed * std::cos(state.heading_offset_radians) /
                                     longitudinal_lane_scale(tick.path.geometry, state);
    state.location.distance_along_path_metres =
        wrap_track_distance(tick.path.geometry, state.location.distance_along_path_metres +
                                                    path_forward_speed * tick.tick_seconds);
    state.location.lateral_offset_metres +=
        state.lateral_velocity_metres_per_second * tick.tick_seconds;

    // Attached motion deliberately has no world-gravity integration. Jumps will
    // transition to a separate airborne state instead of weakening this surface
    // constraint or assuming world Y is the track normal.
    state.normal_offset_metres = handling.track_ride_height_metres;
    state.normal_velocity_metres_per_second = 0.0F;

    const TrackFrame frame =
        sample_track(tick.path.geometry, state.location.distance_along_path_metres);
    state.heading_offset_radians =
        wrap_heading(state.heading_offset_radians - signed_track_yaw(previous_frame, frame));
    constrain_to_track_width(state, frame, tick.definition.collision.local_bounds);
    derive_attached_pose(state, tick.path.geometry);
    assert(is_valid(state));
    return events;
}

} // namespace hover::game
