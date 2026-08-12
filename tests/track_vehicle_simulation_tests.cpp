#include "core/fixed_step.hpp"
#include "game/ships/prototype_01.hpp"
#include "game/track_vehicle_simulation.hpp"
#include "game/tracks/oval_track.hpp"
#include "game/tracks/speedway_track.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr float tolerance = 0.002F;
constexpr float tick_seconds = 1.0F / 90.0F;
constexpr hover::game::TrackPathId primary_path{1U};
int failure_count = 0;

bool nearly_equal(float left, float right, float allowed = tolerance) {
    return std::abs(left - right) <= allowed;
}

bool nearly_equal(hover::math::Vec3 left, hover::math::Vec3 right, float allowed = tolerance) {
    return nearly_equal(left.x, right.x, allowed) && nearly_equal(left.y, right.y, allowed) &&
           nearly_equal(left.z, right.z, allowed);
}

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

hover::game::tracks::OvalTrackDefinition oval_definition() {
    return hover::game::tracks::OvalTrackDefinition{
        .straight_length_metres = 120.0F,
        .turn_radius_metres = 40.0F,
        .half_width_metres = 12.0F,
        .elevation_metres = 0.0F,
    };
}

hover::game::SampledTrack make_flat_track() {
    return hover::game::tracks::make_sampled_oval(
        hover::game::tracks::OvalTrackBuild{oval_definition(), 512U});
}

hover::game::SampledTrack make_vertical_loop_track() {
    constexpr float radius = 40.0F;
    constexpr std::uint32_t sample_count = 512U;
    const float length = 2.0F * std::numbers::pi_v<float> * radius;
    std::vector<hover::game::TrackFrame> frames;
    frames.reserve(sample_count);
    for (std::uint32_t index = 0; index < sample_count; ++index) {
        const float alpha = static_cast<float>(index) / static_cast<float>(sample_count);
        const float angle = alpha * 2.0F * std::numbers::pi_v<float>;
        frames.push_back(hover::game::TrackFrame{
            .distance_metres = alpha * length,
            .center = {0.0F, radius - radius * std::cos(angle), radius * std::sin(angle)},
            .tangent = {0.0F, std::sin(angle), std::cos(angle)},
            .normal = {0.0F, std::cos(angle), -std::sin(angle)},
            .binormal = {1.0F, 0.0F, 0.0F},
            .half_width_metres = 12.0F,
        });
    }
    return hover::game::SampledTrack{length, std::move(frames)};
}

void test_spawn_uses_generic_path_frame() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    const hover::game::TrackVehicleState state =
        hover::game::make_track_vehicle_state({.lateral_offset_metres = 3.0F}, ship, path);
    const hover::game::TrackFrame frame = hover::game::sample_track(track, 0.0F);

    check(state.location.path == primary_path && state.location.distance_along_path_metres == 0.0F,
          "spawn retains the course-resolved path identity and canonical distance");
    check(nearly_equal(state.normal_offset_metres, ship.handling.track_ride_height_metres),
          "spawn uses the ship's explicit ride height");
    check(nearly_equal(state.vehicle.pose.position,
                       hover::game::point_on_track_frame(
                           frame, {3.0F, ship.handling.track_ride_height_metres})) &&
              nearly_equal(state.vehicle.pose.forward, frame.tangent) &&
              nearly_equal(state.vehicle.pose.up, frame.normal),
          "spawn derives position and full orientation from a generic sampled frame");
}

void test_distance_wraps_and_forward_motion_follows_path() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::TrackVehicleState state = hover::game::make_track_vehicle_state(
        {.distance_along_path_metres = track.length_metres - 0.25F}, ship, path);
    state.vehicle.forward_speed_metres_per_second = 100.0F;

    hover::game::simulate_track_vehicle(
        state, hover::game::TrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});

    check(state.location.distance_along_path_metres < 2.0F,
          "attached forward motion wraps cleanly through the closed path seam");
    const hover::game::TrackFrame frame =
        hover::game::sample_track(track, state.location.distance_along_path_metres);
    check(nearly_equal(state.vehicle.pose.up, frame.normal) &&
              hover::math::dot(state.vehicle.pose.forward, frame.tangent) > 0.99F,
          "world pose continues along the sampled path after wrapping");
}

void test_steering_moves_laterally_and_respects_road_width() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::TrackVehicleState state = hover::game::make_track_vehicle_state({}, ship, path);
    state.vehicle.forward_speed_metres_per_second =
        ship.handling.base_maximum_forward_speed_metres_per_second;
    const hover::input::PlayerInput steer_right{.steering = 1.0F, .throttle = 1.0F};

    for (int tick = 0; tick < 20; ++tick) {
        hover::game::simulate_track_vehicle(
            state, hover::game::TrackVehicleTick{steer_right, ship, path, tick_seconds});
    }
    check(state.location.lateral_offset_metres > 0.0F &&
              state.lateral_velocity_metres_per_second > 0.0F,
          "semantic right steering produces rightward track-space motion");

    for (int tick = 0; tick < 360; ++tick) {
        hover::game::simulate_track_vehicle(
            state, hover::game::TrackVehicleTick{steer_right, ship, path, tick_seconds});
    }
    const hover::game::TrackFrame frame =
        hover::game::sample_track(track, state.location.distance_along_path_metres);
    const float maximum_origin_offset = frame.half_width_metres -
                                        ship.collision.local_bounds.center.x -
                                        ship.collision.local_bounds.half_extents.x;
    check(nearly_equal(state.location.lateral_offset_metres, maximum_origin_offset) &&
              nearly_equal(state.lateral_velocity_metres_per_second, 0.0F),
          "the ship's collider center cannot be steered beyond the usable road width");
}

void test_drift_uses_the_same_controls_with_lower_lateral_grip() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::TrackVehicleState gripping = hover::game::make_track_vehicle_state({}, ship, path);
    hover::game::TrackVehicleState drifting = gripping;
    gripping.vehicle.forward_speed_metres_per_second =
        ship.handling.base_maximum_forward_speed_metres_per_second;
    drifting.vehicle.forward_speed_metres_per_second =
        ship.handling.base_maximum_forward_speed_metres_per_second;

    hover::game::simulate_track_vehicle(
        gripping, hover::game::TrackVehicleTick{
                      {.steering = 1.0F, .throttle = 1.0F}, ship, path, tick_seconds});
    hover::game::simulate_track_vehicle(
        drifting,
        hover::game::TrackVehicleTick{
            {.steering = 1.0F, .throttle = 1.0F, .drift = true}, ship, path, tick_seconds});

    check(gripping.lateral_velocity_metres_per_second >
                  drifting.lateral_velocity_metres_per_second &&
              drifting.lateral_velocity_metres_per_second > 0.0F,
          "drift keeps the shared steering target but approaches it with lower lateral grip");
}

void test_heading_persists_without_speed_or_steering() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::TrackVehicleState state = hover::game::make_track_vehicle_state({}, ship, path);

    hover::game::simulate_track_vehicle(
        state, hover::game::TrackVehicleTick{{.steering = 1.0F}, ship, path, tick_seconds});
    const float rotated_heading = state.heading_offset_radians;
    const hover::math::Vec3 rotated_forward = state.vehicle.pose.forward;
    for (int tick = 0; tick < 30; ++tick) {
        hover::game::simulate_track_vehicle(
            state, hover::game::TrackVehicleTick{{}, ship, path, tick_seconds});
    }

    check(rotated_heading > 0.0F && nearly_equal(state.heading_offset_radians, rotated_heading) &&
              nearly_equal(state.vehicle.pose.forward, rotated_forward),
          "a stationary attached ship keeps its steered heading instead of snapping to tangent");
}

void test_horizontal_corner_requires_steering() {
    const hover::game::SampledTrack track = make_flat_track();
    hover::game::ShipDefinition ship = hover::game::ships::prototype_01_definition();
    ship.handling.coasting_deceleration_metres_per_second_squared = 0.0F;
    const hover::game::ResolvedTrackPath path{primary_path, track};
    const float corner_approach = oval_definition().straight_length_metres - 1.0F;
    hover::game::TrackVehicleState unsteered = hover::game::make_track_vehicle_state(
        {.distance_along_path_metres = corner_approach}, ship, path);
    hover::game::TrackVehicleState steering_left = unsteered;
    unsteered.vehicle.forward_speed_metres_per_second = 60.0F;
    steering_left.vehicle.forward_speed_metres_per_second = 60.0F;

    for (int tick = 0; tick < 120; ++tick) {
        hover::game::simulate_track_vehicle(
            unsteered, hover::game::TrackVehicleTick{{}, ship, path, tick_seconds});
        hover::game::simulate_track_vehicle(
            steering_left,
            hover::game::TrackVehicleTick{{.steering = -1.0F}, ship, path, tick_seconds});
    }

    const hover::game::TrackFrame unsteered_frame =
        hover::game::sample_track(track, unsteered.location.distance_along_path_metres);
    const float outside_limit = unsteered_frame.half_width_metres -
                                ship.collision.local_bounds.center.x -
                                ship.collision.local_bounds.half_extents.x;
    check(nearly_equal(unsteered.location.lateral_offset_metres, outside_limit),
          "entering a horizontal corner without steering carries the ship into the outside edge");
    check(std::abs(steering_left.location.lateral_offset_metres) <
              std::abs(unsteered.location.lateral_offset_metres) * 0.5F,
          "steering into the corner keeps the ship substantially closer to its chosen line");
}

void test_lane_distance_accounts_for_curve_radius() {
    const hover::game::SampledTrack track = make_flat_track();
    hover::game::ShipDefinition ship = hover::game::ships::prototype_01_definition();
    ship.handling.coasting_deceleration_metres_per_second_squared = 0.0F;
    const hover::game::ResolvedTrackPath path{primary_path, track};
    const hover::game::tracks::OvalTrackDefinition oval = oval_definition();
    const float turn_midpoint =
        oval.straight_length_metres + oval.turn_radius_metres * std::numbers::pi_v<float> * 0.5F;
    hover::game::TrackVehicleState inside = hover::game::make_track_vehicle_state(
        {.distance_along_path_metres = turn_midpoint, .lateral_offset_metres = -6.0F}, ship, path);
    hover::game::TrackVehicleState outside = hover::game::make_track_vehicle_state(
        {.distance_along_path_metres = turn_midpoint, .lateral_offset_metres = 6.0F}, ship, path);
    inside.vehicle.forward_speed_metres_per_second = 60.0F;
    outside.vehicle.forward_speed_metres_per_second = 60.0F;

    hover::game::simulate_track_vehicle(
        inside, hover::game::TrackVehicleTick{{.steering = -1.0F}, ship, path, tick_seconds});
    hover::game::simulate_track_vehicle(
        outside, hover::game::TrackVehicleTick{{.steering = -1.0F}, ship, path, tick_seconds});

    check(inside.location.distance_along_path_metres > outside.location.distance_along_path_metres,
          "equal world speed advances farther in centerline distance on the shorter inside lane");
}

void test_vertical_loop_pitch_does_not_require_horizontal_steering() {
    const hover::game::SampledTrack track = make_vertical_loop_track();
    check(hover::game::is_valid(track), "vertical loop fixture is a valid sampled path");
    hover::game::ShipDefinition ship = hover::game::ships::prototype_01_definition();
    ship.handling.coasting_deceleration_metres_per_second_squared = 0.0F;
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::TrackVehicleState state = hover::game::make_track_vehicle_state({}, ship, path);
    state.vehicle.forward_speed_metres_per_second = 60.0F;

    for (int tick = 0; tick < 90; ++tick) {
        hover::game::simulate_track_vehicle(
            state, hover::game::TrackVehicleTick{{}, ship, path, tick_seconds});
    }
    const hover::game::TrackFrame frame =
        hover::game::sample_track(track, state.location.distance_along_path_metres);

    check(std::abs(state.heading_offset_radians) < tolerance &&
              nearly_equal(state.vehicle.pose.forward, frame.tangent) &&
              nearly_equal(state.vehicle.pose.up, frame.normal) && state.vehicle.pose.up.y < 0.2F,
          "surface pitch carries an unsteered ship into a vertical loop without adding path yaw");
}

void test_attached_boost_uses_shared_dynamics_and_events() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::TrackVehicleState state = hover::game::make_track_vehicle_state({}, ship, path);

    const hover::game::VehicleTickEvents activation = hover::game::simulate_track_vehicle(
        state,
        hover::game::TrackVehicleTick{{.throttle = 1.0F, .boost = true}, ship, path, tick_seconds});
    const hover::game::VehicleTickEvents held = hover::game::simulate_track_vehicle(
        state,
        hover::game::TrackVehicleTick{{.throttle = 1.0F, .boost = true}, ship, path, tick_seconds});

    check(activation.boost_activated && !held.boost_activated && state.vehicle.boosting,
          "attached movement preserves one-shot shared boost state and feedback events");
}

void test_banked_track_controls_vehicle_and_camera_orientation() {
    const hover::game::tracks::OvalTrackDefinition oval = oval_definition();
    constexpr float maximum_bank = 0.4886921906F;
    const hover::game::SampledTrack track =
        hover::game::tracks::make_sampled_speedway(hover::game::tracks::SpeedwayTrackBuild{
            {oval, maximum_bank, 20.0F},
            512U,
        });
    const float turn_midpoint =
        oval.straight_length_metres + oval.turn_radius_metres * std::numbers::pi_v<float> * 0.5F;
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    const hover::game::TrackVehicleState state = hover::game::make_track_vehicle_state(
        {.distance_along_path_metres = turn_midpoint}, ship, path);
    const hover::game::TrackFrame frame = hover::game::sample_track(track, turn_midpoint);

    check(nearly_equal(state.vehicle.pose.up, frame.normal) &&
              nearly_equal(state.vehicle.pose.forward, frame.tangent) &&
              state.vehicle.pose.up.y < 0.95F,
          "banked sampled frames orient the attached ship instead of assuming world-up");
    const hover::math::Mat4 model = hover::game::model_matrix(state.vehicle.pose);
    const hover::math::Vec4 rendered_up = hover::math::transform(model, {0.0F, 1.0F, 0.0F, 0.0F});
    check(
        nearly_equal(hover::math::Vec3{rendered_up.x, rendered_up.y, rendered_up.z}, frame.normal),
        "the render transform preserves the attached surface orientation");
}

hover::game::TrackVehicleState simulate_with_render_rate(const hover::game::SampledTrack& track,
                                                         double frames_per_second) {
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::TrackVehicleState state = hover::game::make_track_vehicle_state({}, ship, path);
    hover::core::FixedStepAccumulator accumulator{
        hover::core::FixedStepConfig{1.0 / 90.0, 0.25, 8}};
    constexpr double duration_seconds = 1.0;
    const double frame_seconds = 1.0 / frames_per_second;
    double submitted_seconds = 0.0;
    while (submitted_seconds < duration_seconds) {
        const double frame_end = std::min(submitted_seconds + frame_seconds, duration_seconds);
        const hover::core::FixedStepPlan plan = accumulator.advance(frame_end - submitted_seconds);
        submitted_seconds = frame_end;
        for (std::uint32_t tick = 0; tick < plan.tick_count; ++tick) {
            hover::game::simulate_track_vehicle(
                state, hover::game::TrackVehicleTick{
                           {.steering = 0.35F, .throttle = 1.0F}, ship, path, tick_seconds});
        }
    }
    return state;
}

void test_attached_simulation_is_render_rate_independent() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::TrackVehicleState reference = simulate_with_render_rate(track, 90.0);
    constexpr std::array render_rates{24.0, 60.0, 120.0, 240.0, 244.0, 360.0};
    for (const double render_rate : render_rates) {
        const hover::game::TrackVehicleState candidate =
            simulate_with_render_rate(track, render_rate);
        check(nearly_equal(candidate.location.distance_along_path_metres,
                           reference.location.distance_along_path_metres) &&
                  nearly_equal(candidate.location.lateral_offset_metres,
                               reference.location.lateral_offset_metres) &&
                  nearly_equal(candidate.vehicle.forward_speed_metres_per_second,
                               reference.vehicle.forward_speed_metres_per_second),
              "render scheduling does not change attached distance, steering, or speed");
    }
}

} // namespace

int main() {
    test_spawn_uses_generic_path_frame();
    test_distance_wraps_and_forward_motion_follows_path();
    test_steering_moves_laterally_and_respects_road_width();
    test_drift_uses_the_same_controls_with_lower_lateral_grip();
    test_heading_persists_without_speed_or_steering();
    test_horizontal_corner_requires_steering();
    test_lane_distance_accounts_for_curve_radius();
    test_vertical_loop_pitch_does_not_require_horizontal_steering();
    test_attached_boost_uses_shared_dynamics_and_events();
    test_banked_track_controls_vehicle_and_camera_orientation();
    test_attached_simulation_is_render_rate_independent();

    if (failure_count != 0) {
        std::cerr << failure_count << " track-vehicle-simulation test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All track-vehicle-simulation tests passed\n";
    return EXIT_SUCCESS;
}
