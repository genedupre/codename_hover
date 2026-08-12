#include "core/fixed_step.hpp"
#include "game/ships/prototype_01.hpp"
#include "game/tracks/oval_track.hpp"
#include "game/tracks/speedway_track.hpp"
#include "game/world_track_vehicle_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <string_view>

namespace {

constexpr float tolerance = 0.002F;
constexpr float tick_seconds = static_cast<float>(hover::core::simulation_tick_seconds);
constexpr hover::game::TrackPathId primary_path{1U};
int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

bool nearly_equal(float left, float right, float allowed = tolerance) {
    return std::abs(left - right) <= allowed;
}

bool nearly_equal(hover::math::Vec3 left, hover::math::Vec3 right, float allowed = tolerance) {
    return nearly_equal(left.x, right.x, allowed) && nearly_equal(left.y, right.y, allowed) &&
           nearly_equal(left.z, right.z, allowed);
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

hover::game::SampledTrack make_banked_track() {
    return hover::game::tracks::make_sampled_speedway(hover::game::tracks::SpeedwayTrackBuild{
        .definition =
            hover::game::tracks::SpeedwayTrackDefinition{
                .oval = oval_definition(),
                .maximum_bank_radians = 0.4886921906F,
                .bank_transition_metres = 20.0F,
            },
        .sample_count = 512U,
    });
}

hover::game::ShipDefinition test_ship() {
    hover::game::ShipDefinition ship = hover::game::ships::prototype_01_definition();
    ship.handling.coasting_deceleration_metres_per_second_squared = 0.0F;
    return ship;
}

void test_spawn_initializes_world_state_from_course() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    const hover::game::WorldTrackVehicleState state = hover::game::make_world_track_vehicle_state(
        {.distance_along_path_metres = 10.0F, .lateral_offset_metres = 3.0F}, ship, path);
    const hover::game::TrackFrame frame = hover::game::sample_track(track, 10.0F);

    check(hover::game::is_valid(state), "world-track spawn produces valid physical state");
    check(state.course.location.path == primary_path &&
              nearly_equal(state.course.location.distance_along_path_metres, 10.0F) &&
              nearly_equal(state.course.location.lateral_offset_metres, 3.0F),
          "spawn records a derived reference on the resolved path");
    check(nearly_equal(state.physical.position,
                       hover::game::point_on_track_frame(
                           frame, {3.0F, ship.handling.track_ride_height_metres})) &&
              nearly_equal(state.physical.basis.forward, frame.tangent) &&
              nearly_equal(state.physical.basis.up, frame.normal) &&
              nearly_equal(state.physical.velocity, {0.0F, 0.0F, 0.0F}),
          "spawn initializes world position, basis, and velocity from the course frame");
}

void test_steering_rotates_orientation_without_rotating_momentum() {
    const hover::game::SampledTrack track = make_flat_track();
    hover::game::ShipDefinition ship = test_ship();
    ship.handling.world_lateral_grip_deceleration_metres_per_second_squared = 0.0F;
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    state.physical.velocity = {0.0F, 0.0F, 100.0F};
    state.vehicle.forward_speed_metres_per_second = 100.0F;
    const hover::math::Vec3 initial_velocity = state.physical.velocity;

    hover::game::simulate_world_track_vehicle(
        state, hover::game::WorldTrackVehicleTick{{.steering = 1.0F}, ship, path, tick_seconds});

    check(state.physical.basis.forward.x > 0.0F,
          "positive steering rotates the physical ship toward track-right");
    check(nearly_equal(state.physical.velocity, initial_velocity),
          "steering alone does not directly rotate world momentum");
}

void test_grip_removes_lateral_velocity_by_a_bounded_amount() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    state.physical.velocity = {20.0F, 0.0F, 100.0F};
    state.vehicle.forward_speed_metres_per_second = 100.0F;

    hover::game::simulate_world_track_vehicle(
        state, hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});

    const float expected_lateral =
        20.0F -
        ship.handling.world_lateral_grip_deceleration_metres_per_second_squared * tick_seconds;
    check(nearly_equal(state.physical.velocity.x, expected_lateral),
          "normal grip removes at most its configured lateral speed per fixed tick");
}

void test_fixed_grip_loses_directional_authority_as_speed_rises() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};

    const auto steer_at_speed = [&](float speed) {
        hover::game::WorldTrackVehicleState state =
            hover::game::make_world_track_vehicle_state({}, ship, path);
        state.physical.velocity = state.physical.basis.forward * speed;
        state.vehicle.forward_speed_metres_per_second = speed;
        hover::game::simulate_world_track_vehicle(
            state, hover::game::WorldTrackVehicleTick{
                       {.steering = 1.0F, .throttle = 1.0F}, ship, path, tick_seconds});
        const hover::math::Vec3 vehicle_right = hover::math::normalized(
            hover::math::cross(state.physical.basis.up, state.physical.basis.forward));
        return std::abs(hover::math::dot(state.physical.velocity, vehicle_right));
    };

    const float low_speed_slip =
        steer_at_speed(ship.handling.base_maximum_forward_speed_metres_per_second * 0.35F);
    const float base_maximum_slip =
        steer_at_speed(ship.handling.base_maximum_forward_speed_metres_per_second);
    const float boosted_maximum_slip =
        steer_at_speed(ship.handling.base_maximum_forward_speed_metres_per_second *
                       ship.handling.boost_maximum_speed_multiplier);

    check(low_speed_slip <= tolerance && base_maximum_slip > low_speed_slip + 0.5F &&
              boosted_maximum_slip > base_maximum_slip + 0.5F,
          "fixed grip holds a low-speed turn but leaves progressively more slip at base and "
          "boost maximum speed");
}

void test_directional_drift_and_both_held_policy() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState left =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    hover::game::WorldTrackVehicleState right = left;
    hover::game::WorldTrackVehicleState both = left;

    hover::game::simulate_world_track_vehicle(
        left, hover::game::WorldTrackVehicleTick{{.drift_left = true}, ship, path, tick_seconds});
    hover::game::simulate_world_track_vehicle(
        right, hover::game::WorldTrackVehicleTick{{.drift_right = true}, ship, path, tick_seconds});
    hover::game::simulate_world_track_vehicle(
        both, hover::game::WorldTrackVehicleTick{
                  {.drift_left = true, .drift_right = true}, ship, path, tick_seconds});

    check(left.physical.velocity.x < 0.0F && right.physical.velocity.x > 0.0F &&
              nearly_equal(left.physical.velocity.x, -right.physical.velocity.x),
          "LB/L1 and RB/R1 produce equal opposite lateral drift with neutral steering");
    check(nearly_equal(both.physical.velocity, {0.0F, 0.0F, 0.0F}),
          "holding both drift directions cancels force and uses normal grip");
}

void test_drift_force_fades_as_same_direction_slide_builds() {
    const hover::game::SampledTrack track = make_flat_track();
    hover::game::ShipDefinition ship = test_ship();
    ship.handling.world_drift_grip_deceleration_metres_per_second_squared = 0.0F;
    ship.handling.world_drift_forward_deceleration_metres_per_second_squared = 0.0F;
    ship.handling.world_slip_forward_deceleration_per_lateral_speed = 0.0F;
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState fresh =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    hover::game::WorldTrackVehicleState saturated = fresh;
    saturated.physical.velocity =
        hover::math::cross(saturated.physical.basis.up, saturated.physical.basis.forward) *
        ship.handling.world_drift_force_fade_lateral_speed_metres_per_second;

    hover::game::simulate_world_track_vehicle(
        fresh, hover::game::WorldTrackVehicleTick{{.drift_right = true}, ship, path, tick_seconds});
    hover::game::simulate_world_track_vehicle(
        saturated,
        hover::game::WorldTrackVehicleTick{{.drift_right = true}, ship, path, tick_seconds});

    check(fresh.physical.velocity.x > 0.5F &&
              nearly_equal(saturated.physical.velocity.x,
                           ship.handling.world_drift_force_fade_lateral_speed_metres_per_second),
          "drift adds its full side force from rest and fades it at the configured slide speed");
}

void test_drift_suppresses_propulsion_and_loses_forward_speed() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState planted =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    planted.physical.velocity =
        planted.physical.basis.forward * ship.handling.base_maximum_forward_speed_metres_per_second;
    planted.vehicle.forward_speed_metres_per_second =
        ship.handling.base_maximum_forward_speed_metres_per_second;
    hover::game::WorldTrackVehicleState drifting = planted;

    hover::game::simulate_world_track_vehicle(
        planted, hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
    hover::game::simulate_world_track_vehicle(
        drifting, hover::game::WorldTrackVehicleTick{
                      {.throttle = 1.0F, .drift_right = true}, ship, path, tick_seconds});

    check(drifting.vehicle.forward_speed_metres_per_second <
              planted.vehicle.forward_speed_metres_per_second - 0.4F,
          "holding a drift at maximum speed suppresses propulsion and slows the ship");
}

void test_sustained_high_speed_steering_turns_slip_into_speed_loss() {
    hover::game::tracks::OvalTrackDefinition definition = oval_definition();
    definition.straight_length_metres = 2'000.0F;
    definition.turn_radius_metres = 250.0F;
    definition.half_width_metres = 200.0F;
    const hover::game::SampledTrack track =
        hover::game::tracks::make_sampled_oval({definition, 2'048U});
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState straight =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    straight.physical.velocity = straight.physical.basis.forward *
                                 ship.handling.base_maximum_forward_speed_metres_per_second;
    straight.vehicle.forward_speed_metres_per_second =
        ship.handling.base_maximum_forward_speed_metres_per_second;
    hover::game::WorldTrackVehicleState turning = straight;

    for (int tick = 0; tick < 60; ++tick) {
        hover::game::simulate_world_track_vehicle(
            straight,
            hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
        hover::game::simulate_world_track_vehicle(
            turning, hover::game::WorldTrackVehicleTick{
                         {.steering = 1.0F, .throttle = 1.0F}, ship, path, tick_seconds});
    }

    check(turning.vehicle.forward_speed_metres_per_second <
              straight.vehicle.forward_speed_metres_per_second - 1.0F,
          "sustained maximum-speed steering converts accumulated lateral slip into speed loss");
    if (!(turning.vehicle.forward_speed_metres_per_second <
          straight.vehicle.forward_speed_metres_per_second - 1.0F)) {
        std::cerr << "  straight speed=" << straight.vehicle.forward_speed_metres_per_second
                  << ", turning speed=" << turning.vehicle.forward_speed_metres_per_second
                  << ", turning velocity x=" << turning.physical.velocity.x << '\n';
    }
}

void test_world_integration_derives_progress_and_wraps_seam() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state = hover::game::make_world_track_vehicle_state(
        {.distance_along_path_metres = track.length_metres - 0.25F}, ship, path);
    state.physical.velocity = state.physical.basis.forward * 100.0F;
    state.vehicle.forward_speed_metres_per_second = 100.0F;

    hover::game::simulate_world_track_vehicle(
        state, hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});

    check(state.course.location.distance_along_path_metres < 2.0F,
          "world integration projects progress through the closed course seam");
    check(nearly_equal(state.physical.position,
                       hover::game::point_on_track_frame(
                           state.course.frame, {state.course.location.lateral_offset_metres,
                                                ship.handling.track_ride_height_metres}),
                       0.01F),
          "supported position is reconstructed only after progress is derived by projection");
}

void test_banked_basis_and_temporary_edge_constraint() {
    const hover::game::SampledTrack banked_track = make_banked_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath banked_path{primary_path, banked_track};
    const hover::game::tracks::OvalTrackDefinition oval = oval_definition();
    const float turn_midpoint =
        oval.straight_length_metres + oval.turn_radius_metres * std::numbers::pi_v<float> * 0.5F;
    hover::game::WorldTrackVehicleState banked = hover::game::make_world_track_vehicle_state(
        {.distance_along_path_metres = turn_midpoint}, ship, banked_path);
    hover::game::simulate_world_track_vehicle(
        banked, hover::game::WorldTrackVehicleTick{{}, ship, banked_path, tick_seconds});
    check(nearly_equal(banked.physical.basis.up, banked.course.frame.normal) &&
              banked.physical.basis.up.y < 0.95F && hover::game::is_valid(banked.physical.basis),
          "supported physical basis remains orthonormal on a banked surface");

    const hover::game::SampledTrack flat_track = make_flat_track();
    const hover::game::ResolvedTrackPath flat_path{primary_path, flat_track};
    const float maximum_lateral = oval.half_width_metres - ship.collision.local_bounds.center.x -
                                  ship.collision.local_bounds.half_extents.x;
    hover::game::WorldTrackVehicleState edge = hover::game::make_world_track_vehicle_state(
        {.lateral_offset_metres = maximum_lateral - 0.01F}, ship, flat_path);
    edge.physical.velocity = {50.0F, 0.0F, 0.0F};
    hover::game::simulate_world_track_vehicle(
        edge, hover::game::WorldTrackVehicleTick{{}, ship, flat_path, tick_seconds});
    check(edge.course.location.lateral_offset_metres <= maximum_lateral + tolerance &&
              hover::math::dot(edge.physical.velocity, edge.course.frame.binormal) <= tolerance,
          "temporary edge safety constraint removes only outward lateral velocity");
}

void test_boost_and_brake_update_world_forward_velocity() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    const float base_speed = ship.handling.base_maximum_forward_speed_metres_per_second;
    state.physical.velocity = state.physical.basis.forward * base_speed;
    state.vehicle.forward_speed_metres_per_second = base_speed;

    const hover::game::VehicleTickEvents activation = hover::game::simulate_world_track_vehicle(
        state, hover::game::WorldTrackVehicleTick{
                   {.throttle = 1.0F, .boost = true}, ship, path, tick_seconds});
    const float boosted_speed = state.vehicle.forward_speed_metres_per_second;
    check(activation.boost_activated && state.vehicle.boosting && boosted_speed > base_speed,
          "boost activation increases authoritative world forward velocity");

    hover::game::simulate_world_track_vehicle(
        state, hover::game::WorldTrackVehicleTick{{.brake = 1.0F}, ship, path, tick_seconds});
    check(!state.vehicle.boosting && state.vehicle.forward_speed_metres_per_second < boosted_speed,
          "braking cancels boost and decreases authoritative world forward velocity");

    const hover::game::SampledTrack long_straight_track =
        hover::game::tracks::make_sampled_oval(hover::game::tracks::OvalTrackBuild{
            hover::game::tracks::OvalTrackDefinition{
                .straight_length_metres = 2000.0F,
                .turn_radius_metres = 40.0F,
                .half_width_metres = 12.0F,
                .elevation_metres = 0.0F,
            },
            2048U,
        });
    const hover::game::ResolvedTrackPath long_straight_path{primary_path, long_straight_track};
    hover::game::WorldTrackVehicleState returning =
        hover::game::make_world_track_vehicle_state({}, ship, long_straight_path);
    returning.physical.velocity = returning.physical.basis.forward * base_speed;
    returning.vehicle.forward_speed_metres_per_second = base_speed;
    hover::game::simulate_world_track_vehicle(
        returning, hover::game::WorldTrackVehicleTick{
                       {.throttle = 1.0F, .boost = true}, ship, long_straight_path, tick_seconds});
    for (int tick_index = 0; tick_index < static_cast<int>(3.0F / tick_seconds); ++tick_index) {
        hover::game::simulate_world_track_vehicle(
            returning, hover::game::WorldTrackVehicleTick{
                           {.throttle = 1.0F}, ship, long_straight_path, tick_seconds});
    }
    check(!returning.vehicle.boosting &&
              nearly_equal(returning.vehicle.forward_speed_metres_per_second, base_speed, 0.05F),
          "world-space boost expires and excess forward speed returns to the ship baseline");
}

hover::game::WorldTrackVehicleState
simulate_with_render_rate(const hover::game::SampledTrack& track, double frames_per_second) {
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    hover::core::FixedStepAccumulator accumulator{hover::core::FixedStepConfig{
        hover::core::simulation_tick_seconds,
        0.25,
        30,
    }};
    constexpr double duration_seconds = 1.0;
    const double frame_seconds = 1.0 / frames_per_second;
    double submitted_seconds = 0.0;
    while (submitted_seconds < duration_seconds) {
        const double frame_end = std::min(submitted_seconds + frame_seconds, duration_seconds);
        const hover::core::FixedStepPlan plan = accumulator.advance(frame_end - submitted_seconds);
        submitted_seconds = frame_end;
        for (std::uint32_t tick = 0; tick < plan.tick_count; ++tick) {
            hover::game::simulate_world_track_vehicle(
                state, hover::game::WorldTrackVehicleTick{
                           {.steering = -0.2F, .throttle = 1.0F, .drift_left = true},
                           ship,
                           path,
                           tick_seconds});
        }
    }
    return state;
}

void test_world_simulation_is_independent_from_render_schedule() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::WorldTrackVehicleState reference = simulate_with_render_rate(track, 120.0);
    for (const double frames_per_second : {24.0, 30.0, 60.0, 90.0, 144.0, 240.0, 360.0}) {
        const hover::game::WorldTrackVehicleState state =
            simulate_with_render_rate(track, frames_per_second);
        check(nearly_equal(state.physical.position, reference.physical.position, 0.01F) &&
                  nearly_equal(state.physical.velocity, reference.physical.velocity, 0.01F) &&
                  nearly_equal(state.physical.basis.forward, reference.physical.basis.forward,
                               0.001F) &&
                  nearly_equal(state.course.location.distance_along_path_metres,
                               reference.course.location.distance_along_path_metres, 0.01F),
              "world physics is identical under the tested render schedule");
    }
}

} // namespace

int main() {
    test_spawn_initializes_world_state_from_course();
    test_steering_rotates_orientation_without_rotating_momentum();
    test_grip_removes_lateral_velocity_by_a_bounded_amount();
    test_fixed_grip_loses_directional_authority_as_speed_rises();
    test_directional_drift_and_both_held_policy();
    test_drift_force_fades_as_same_direction_slide_builds();
    test_drift_suppresses_propulsion_and_loses_forward_speed();
    test_sustained_high_speed_steering_turns_slip_into_speed_loss();
    test_world_integration_derives_progress_and_wraps_seam();
    test_banked_basis_and_temporary_edge_constraint();
    test_boost_and_brake_update_world_forward_velocity();
    test_world_simulation_is_independent_from_render_schedule();

    if (failure_count != 0) {
        std::cerr << failure_count << " world-track vehicle simulation test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All world-track vehicle simulation tests passed\n";
    return EXIT_SUCCESS;
}
