#include "core/fixed_step.hpp"
#include "game/ships/prototype_01.hpp"
#include "game/tracks/oval_track.hpp"
#include "game/tracks/speedway_track.hpp"
#include "game/world_track_vehicle_simulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <span>
#include <string_view>
#include <vector>

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

hover::game::SampledTrack make_handling_lab_track() {
    return hover::game::tracks::make_sampled_oval({
        hover::game::tracks::OvalTrackDefinition{
            .straight_length_metres = 6'000.0F,
            .turn_radius_metres = 1'000.0F,
            .half_width_metres = 800.0F,
            .elevation_metres = 0.0F,
        },
        2'048U,
    });
}

struct ScriptStep {
    std::uint32_t tick_count;
    hover::input::PlayerInput input;
};

struct TraceSample {
    hover::game::WorldTrackVehicleState state;
    hover::game::WorldTrackVehicleTickResult result;
};

std::vector<TraceSample> run_script(const hover::game::SampledTrack& track,
                                    const hover::game::ShipDefinition& ship,
                                    std::span<const ScriptStep> steps, float initial_speed = 0.0F) {
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    state.physical.velocity = state.physical.basis.forward * initial_speed;
    state.vehicle.forward_speed_metres_per_second = initial_speed;

    std::vector<TraceSample> trace;
    for (const ScriptStep& step : steps) {
        for (std::uint32_t tick = 0; tick < step.tick_count; ++tick) {
            const hover::game::WorldTrackVehicleTickResult result =
                hover::game::simulate_world_track_vehicle(
                    state,
                    hover::game::WorldTrackVehicleTick{step.input, ship, path, tick_seconds});
            trace.push_back(TraceSample{state, result});
        }
    }
    return trace;
}

bool nearly_equal(const hover::game::WorldTrackVehicleTelemetry& left,
                  const hover::game::WorldTrackVehicleTelemetry& right) {
    return nearly_equal(left.world_speed_metres_per_second, right.world_speed_metres_per_second) &&
           nearly_equal(left.local_forward_speed_metres_per_second,
                        right.local_forward_speed_metres_per_second) &&
           nearly_equal(left.local_lateral_speed_metres_per_second,
                        right.local_lateral_speed_metres_per_second) &&
           nearly_equal(left.local_normal_speed_metres_per_second,
                        right.local_normal_speed_metres_per_second) &&
           nearly_equal(left.signed_slip_angle_radians, right.signed_slip_angle_radians) &&
           nearly_equal(left.steering_direction_change_radians,
                        right.steering_direction_change_radians) &&
           nearly_equal(left.steering_direction_change_ratio,
                        right.steering_direction_change_ratio) &&
           nearly_equal(left.drift_direction, right.drift_direction) &&
           nearly_equal(left.drift_force_fraction, right.drift_force_fraction) &&
           nearly_equal(left.selected_grip_deceleration_metres_per_second_squared,
                        right.selected_grip_deceleration_metres_per_second_squared) &&
           nearly_equal(left.available_grip_deceleration_metres_per_second_squared,
                        right.available_grip_deceleration_metres_per_second_squared) &&
           nearly_equal(left.grip_demand_deceleration_metres_per_second_squared,
                        right.grip_demand_deceleration_metres_per_second_squared) &&
           nearly_equal(left.traction_saturation_ratio, right.traction_saturation_ratio) &&
           nearly_equal(left.forward_damping_deceleration_metres_per_second_squared,
                        right.forward_damping_deceleration_metres_per_second_squared) &&
           nearly_equal(left.lateral_damping_deceleration_metres_per_second_squared,
                        right.lateral_damping_deceleration_metres_per_second_squared) &&
           nearly_equal(left.normal_damping_deceleration_metres_per_second_squared,
                        right.normal_damping_deceleration_metres_per_second_squared) &&
           nearly_equal(left.propulsion_curve_multiplier, right.propulsion_curve_multiplier) &&
           nearly_equal(left.sustained_slip_seconds, right.sustained_slip_seconds) &&
           nearly_equal(left.sustained_slip_intensity, right.sustained_slip_intensity) &&
           nearly_equal(left.requested_propulsion_acceleration_metres_per_second_squared,
                        right.requested_propulsion_acceleration_metres_per_second_squared) &&
           nearly_equal(left.applied_propulsion_acceleration_metres_per_second_squared,
                        right.applied_propulsion_acceleration_metres_per_second_squared) &&
           nearly_equal(left.propulsion_fraction, right.propulsion_fraction) &&
           nearly_equal(left.post_boost_return_deceleration_metres_per_second_squared,
                        right.post_boost_return_deceleration_metres_per_second_squared) &&
           nearly_equal(left.height_above_surface_metres, right.height_above_surface_metres) &&
           nearly_equal(left.surface_normal_speed_metres_per_second,
                        right.surface_normal_speed_metres_per_second) &&
           nearly_equal(left.wall_impact_speed_metres_per_second,
                        right.wall_impact_speed_metres_per_second) &&
           left.contact_mode == right.contact_mode &&
           left.edge_constraint_activated == right.edge_constraint_activated;
}

bool nearly_equal(const TraceSample& left, const TraceSample& right) {
    return nearly_equal(left.state.physical.position, right.state.physical.position) &&
           nearly_equal(left.state.physical.velocity, right.state.physical.velocity) &&
           nearly_equal(left.state.physical.basis.forward, right.state.physical.basis.forward) &&
           nearly_equal(left.state.physical.basis.up, right.state.physical.basis.up) &&
           nearly_equal(left.state.course.location.distance_along_path_metres,
                        right.state.course.location.distance_along_path_metres) &&
           nearly_equal(left.state.course.location.lateral_offset_metres,
                        right.state.course.location.lateral_offset_metres) &&
           nearly_equal(left.state.vehicle.forward_speed_metres_per_second,
                        right.state.vehicle.forward_speed_metres_per_second) &&
           nearly_equal(left.state.handling.sustained_slip_seconds,
                        right.state.handling.sustained_slip_seconds) &&
           nearly_equal(left.state.handling.sustained_slip_intensity,
                        right.state.handling.sustained_slip_intensity) &&
           nearly_equal(
               left.state.handling.applied_propulsion_acceleration_metres_per_second_squared,
               right.state.handling.applied_propulsion_acceleration_metres_per_second_squared) &&
           nearly_equal(left.state.vehicle.boost_seconds_remaining,
                        right.state.vehicle.boost_seconds_remaining) &&
           left.state.vehicle.boosting == right.state.vehicle.boosting &&
           left.state.vehicle.boost_input_was_down == right.state.vehicle.boost_input_was_down &&
           left.result.events.boost_activated == right.result.events.boost_activated &&
           left.result.events.wall_impact == right.result.events.wall_impact &&
           left.result.events.support_lost == right.result.events.support_lost &&
           left.result.events.landed == right.result.events.landed &&
           left.result.events.recovered == right.result.events.recovered &&
           nearly_equal(left.result.events.wall_impact_speed_metres_per_second,
                        right.result.events.wall_impact_speed_metres_per_second) &&
           nearly_equal(left.result.telemetry, right.result.telemetry);
}

void check_trace_is_valid_and_repeatable(const hover::game::SampledTrack& track,
                                         const hover::game::ShipDefinition& ship,
                                         std::span<const ScriptStep> script, float initial_speed,
                                         std::string_view description) {
    const std::vector<TraceSample> first = run_script(track, ship, script, initial_speed);
    const std::vector<TraceSample> second = run_script(track, ship, script, initial_speed);
    const bool valid = std::ranges::all_of(first, [](const TraceSample& sample) {
        return hover::game::is_valid(sample.state) &&
               hover::game::is_valid(sample.result.telemetry);
    });
    const bool repeatable =
        first.size() == second.size() &&
        std::ranges::equal(first, second, [](const TraceSample& left, const TraceSample& right) {
            return nearly_equal(left, right);
        });
    check(!first.empty() && valid && repeatable, description);
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
    ship.handling.world_forward_damping_per_second = 0.0F;
    ship.handling.world_lateral_damping_per_second = 0.0F;
    ship.handling.world_normal_damping_per_second = 0.0F;
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
    hover::game::ShipDefinition ship = test_ship();
    ship.handling.world_lateral_damping_per_second = 0.0F;
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

void test_speed_slip_and_recovery_inputs_shape_available_traction() {
    const hover::game::SampledTrack track = make_handling_lab_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    const float base_speed = ship.handling.base_maximum_forward_speed_metres_per_second;
    const float boost_speed = base_speed * ship.handling.boost_maximum_speed_multiplier;

    const auto sample_grip = [&](float speed, float slip_intensity,
                                 hover::input::PlayerInput input) {
        hover::game::WorldTrackVehicleState state =
            hover::game::make_world_track_vehicle_state({}, ship, path);
        const hover::math::Vec3 right = hover::math::normalized(
            hover::math::cross(state.physical.basis.up, state.physical.basis.forward));
        state.physical.velocity = state.physical.basis.forward * speed + right * 20.0F;
        state.vehicle.forward_speed_metres_per_second = speed;
        state.handling.sustained_slip_intensity = slip_intensity;
        return hover::game::simulate_world_track_vehicle(
                   state, hover::game::WorldTrackVehicleTick{input, ship, path, tick_seconds})
            .telemetry;
    };

    const hover::game::WorldTrackVehicleTelemetry low =
        sample_grip(base_speed * 0.5F, 0.0F, {.throttle = 1.0F});
    const hover::game::WorldTrackVehicleTelemetry boosted =
        sample_grip(boost_speed, 0.0F, {.throttle = 1.0F});
    const hover::game::WorldTrackVehicleTelemetry slipping =
        sample_grip(boost_speed, 1.0F, {.throttle = 1.0F});
    const hover::game::WorldTrackVehicleTelemetry lifted = sample_grip(boost_speed, 0.0F, {});
    const hover::game::WorldTrackVehicleTelemetry braking =
        sample_grip(boost_speed, 0.0F, {.brake = 1.0F});

    check(nearly_equal(low.available_grip_deceleration_metres_per_second_squared,
                       ship.handling.world_lateral_grip_deceleration_metres_per_second_squared,
                       0.01F) &&
              boosted.available_grip_deceleration_metres_per_second_squared <
                  low.available_grip_deceleration_metres_per_second_squared &&
              slipping.available_grip_deceleration_metres_per_second_squared <
                  boosted.available_grip_deceleration_metres_per_second_squared,
          "boost speed and sustained slip progressively reduce available traction");
    check(lifted.available_grip_deceleration_metres_per_second_squared >
                  boosted.available_grip_deceleration_metres_per_second_squared &&
              braking.available_grip_deceleration_metres_per_second_squared >
                  lifted.available_grip_deceleration_metres_per_second_squared &&
              slipping.traction_saturation_ratio > boosted.traction_saturation_ratio,
          "lifting and braking recover traction while accumulated slip increases saturation");
}

void test_directional_drift_and_both_held_policy() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState left =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    hover::game::WorldTrackVehicleState right = left;
    hover::game::WorldTrackVehicleState both = left;

    const hover::game::WorldTrackVehicleTickResult left_result =
        hover::game::simulate_world_track_vehicle(
            left,
            hover::game::WorldTrackVehicleTick{{.drift_left = true}, ship, path, tick_seconds});
    const hover::game::WorldTrackVehicleTickResult right_result =
        hover::game::simulate_world_track_vehicle(
            right,
            hover::game::WorldTrackVehicleTick{{.drift_right = true}, ship, path, tick_seconds});
    hover::game::simulate_world_track_vehicle(
        both, hover::game::WorldTrackVehicleTick{
                  {.drift_left = true, .drift_right = true}, ship, path, tick_seconds});

    check(left.physical.velocity.x < 0.0F && right.physical.velocity.x > 0.0F &&
              nearly_equal(left.physical.velocity.x, -right.physical.velocity.x),
          "LB/L1 and RB/R1 produce equal opposite lateral drift with neutral steering");
    check(left_result.telemetry.signed_slip_angle_radians < 0.0F &&
              right_result.telemetry.signed_slip_angle_radians > 0.0F &&
              nearly_equal(left_result.telemetry.signed_slip_angle_radians,
                           -right_result.telemetry.signed_slip_angle_radians),
          "signed slip telemetry mirrors equal opposite left and right drift");
    check(nearly_equal(both.physical.velocity, {0.0F, 0.0F, 0.0F}),
          "holding both drift directions cancels force and uses normal grip");
}

void test_drift_force_fades_as_same_direction_slide_builds() {
    const hover::game::SampledTrack track = make_flat_track();
    hover::game::ShipDefinition ship = test_ship();
    ship.handling.world_drift_grip_deceleration_metres_per_second_squared = 0.0F;
    ship.handling.world_lateral_damping_per_second = 0.0F;
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

    for (int tick = 0; tick < 60; ++tick) {
        hover::game::simulate_world_track_vehicle(
            planted,
            hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
        hover::game::simulate_world_track_vehicle(
            drifting, hover::game::WorldTrackVehicleTick{
                          {.throttle = 1.0F, .drift_right = true}, ship, path, tick_seconds});
    }

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

void test_banked_basis_and_solid_wall_response() {
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
    const hover::game::WorldTrackVehicleTickResult edge_result =
        hover::game::simulate_world_track_vehicle(
            edge, hover::game::WorldTrackVehicleTick{{}, ship, flat_path, tick_seconds});
    check(edge.course.location.lateral_offset_metres <= maximum_lateral + tolerance &&
              hover::math::dot(edge.physical.velocity, edge.course.frame.binormal) <= tolerance &&
              edge_result.events.wall_impact && edge_result.telemetry.edge_constraint_activated &&
              edge_result.events.wall_impact_speed_metres_per_second > 0.0F,
          "a solid wall corrects penetration, reflects outward velocity, and emits impact data");
}

void test_hover_forces_takeoff_landing_and_penetration() {
    const hover::game::SampledTrack track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    const float target_height = ship.handling.track_ride_height_metres;

    hover::game::WorldTrackVehicleState stable =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    for (int tick = 0; tick < 240; ++tick) {
        hover::game::simulate_world_track_vehicle(
            stable, hover::game::WorldTrackVehicleTick{{}, ship, path, tick_seconds});
    }
    check(stable.contact.mode == hover::game::VehicleContactMode::supported &&
              nearly_equal(stable.course.height_above_surface_metres, target_height, 0.001F) &&
              nearly_equal(stable.physical.velocity.y, 0.0F, 0.001F),
          "gravity and hover force balance at the target ride height without drift");

    hover::game::WorldTrackVehicleState below =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    below.physical.position.y -= 0.10F;
    below.course.height_above_surface_metres -= 0.10F;
    hover::game::simulate_world_track_vehicle(
        below, hover::game::WorldTrackVehicleTick{{}, ship, path, tick_seconds});
    hover::game::WorldTrackVehicleState above =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    above.physical.position.y += 0.20F;
    above.course.height_above_surface_metres += 0.20F;
    hover::game::simulate_world_track_vehicle(
        above, hover::game::WorldTrackVehicleTick{{}, ship, path, tick_seconds});
    check(below.physical.velocity.y > 0.0F && above.physical.velocity.y < 0.0F &&
              !nearly_equal(above.course.height_above_surface_metres, target_height, 0.01F),
          "hover force corrects height error without snapping the ship to its target");

    hover::game::WorldTrackVehicleState takeoff =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    takeoff.physical.velocity = takeoff.physical.basis.up * 6.0F;
    const hover::game::WorldTrackVehicleTickResult takeoff_result =
        hover::game::simulate_world_track_vehicle(
            takeoff, hover::game::WorldTrackVehicleTick{{}, ship, path, tick_seconds});
    check(takeoff.contact.mode == hover::game::VehicleContactMode::airborne &&
              takeoff_result.events.support_lost && takeoff.physical.velocity.y > 0.0F,
          "sufficient outward normal speed leaves support without discarding momentum");

    hover::game::WorldTrackVehicleState landing =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    landing.contact.mode = hover::game::VehicleContactMode::airborne;
    landing.physical.position.y = target_height + 0.20F;
    landing.course.height_above_surface_metres = target_height + 0.20F;
    landing.physical.velocity = landing.physical.basis.up * -1.0F;
    const hover::game::WorldTrackVehicleTickResult landing_result =
        hover::game::simulate_world_track_vehicle(
            landing, hover::game::WorldTrackVehicleTick{{}, ship, path, tick_seconds});
    check(landing.contact.mode == hover::game::VehicleContactMode::supported &&
              landing_result.events.landed && landing.physical.velocity.y < 0.0F,
          "a descending airborne ship reacquires an eligible road without a height teleport");

    hover::game::WorldTrackVehicleState penetrating =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    penetrating.physical.position.y = 0.20F;
    penetrating.course.height_above_surface_metres = 0.20F;
    penetrating.physical.velocity = penetrating.physical.basis.up * -5.0F;
    hover::game::simulate_world_track_vehicle(
        penetrating, hover::game::WorldTrackVehicleTick{{}, ship, path, tick_seconds});
    const float minimum_height =
        ship.collision.local_bounds.half_extents.y - ship.collision.local_bounds.center.y;
    check(nearly_equal(penetrating.course.height_above_surface_metres, minimum_height, 0.001F) &&
              penetrating.physical.velocity.y >= 0.0F &&
              !nearly_equal(minimum_height, target_height),
          "surface penetration corrects only the hull clearance and removes inward velocity");
}

void test_mirrored_walls_and_open_edge_recovery() {
    const hover::game::SampledTrack solid_track = make_flat_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath solid_path{primary_path, solid_track};
    const float maximum_lateral =
        oval_definition().half_width_metres - ship.collision.local_bounds.half_extents.x;
    hover::game::WorldTrackVehicleState left = hover::game::make_world_track_vehicle_state(
        {.lateral_offset_metres = -maximum_lateral + 0.01F}, ship, solid_path);
    hover::game::WorldTrackVehicleState right = hover::game::make_world_track_vehicle_state(
        {.lateral_offset_metres = maximum_lateral - 0.01F}, ship, solid_path);
    left.physical.velocity = {-50.0F, 0.0F, 100.0F};
    right.physical.velocity = {50.0F, 0.0F, 100.0F};
    const hover::game::WorldTrackVehicleTickResult left_result =
        hover::game::simulate_world_track_vehicle(
            left, hover::game::WorldTrackVehicleTick{{}, ship, solid_path, tick_seconds});
    const hover::game::WorldTrackVehicleTickResult right_result =
        hover::game::simulate_world_track_vehicle(
            right, hover::game::WorldTrackVehicleTick{{}, ship, solid_path, tick_seconds});
    check(left_result.events.wall_impact && right_result.events.wall_impact &&
              left.physical.velocity.x > 0.0F && right.physical.velocity.x < 0.0F &&
              nearly_equal(left.physical.velocity.x, -right.physical.velocity.x, 0.01F) &&
              left.vehicle.forward_speed_metres_per_second < 90.0F &&
              right.vehicle.forward_speed_metres_per_second < 90.0F,
          "left and right walls mirror recoil while retaining only part of scrape speed");

    hover::game::SampledTrack open_track = make_flat_track();
    for (hover::game::TrackSegmentProperties& properties : open_track.segment_properties) {
        properties.left_edge = hover::game::TrackEdgePolicy::open;
        properties.right_edge = hover::game::TrackEdgePolicy::open;
    }
    const hover::game::ResolvedTrackPath open_path{primary_path, open_track};
    hover::game::WorldTrackVehicleState falling =
        hover::game::make_world_track_vehicle_state({}, ship, open_path);
    const hover::math::Vec3 recovery_position = falling.contact.last_safe_physical.position;
    falling.physical.position = hover::game::point_on_track_frame(
        falling.course.frame, {11.9F, ship.handling.track_ride_height_metres});
    falling.course.location.lateral_offset_metres = 11.9F;
    falling.physical.velocity = falling.course.frame.binormal * 100.0F;
    const hover::game::WorldTrackVehicleTickResult departure =
        hover::game::simulate_world_track_vehicle(
            falling, hover::game::WorldTrackVehicleTick{{}, ship, open_path, tick_seconds});
    check(falling.contact.mode == hover::game::VehicleContactMode::falling &&
              departure.events.support_lost && !departure.events.wall_impact &&
              falling.course.location.lateral_offset_metres >
                  open_track.frames.front().half_width_metres,
          "an open edge does not clamp or reflect a departing ship");

    bool recovered = false;
    for (int tick = 0; tick < 240 && !recovered; ++tick) {
        const hover::game::WorldTrackVehicleTickResult result =
            hover::game::simulate_world_track_vehicle(
                falling, hover::game::WorldTrackVehicleTick{{}, ship, open_path, tick_seconds});
        recovered = result.events.recovered;
    }
    check(recovered && falling.contact.mode == hover::game::VehicleContactMode::supported &&
              nearly_equal(falling.physical.position, recovery_position, 0.01F) &&
              nearly_equal(falling.vehicle.forward_speed_metres_per_second,
                           ship.handling.base_maximum_forward_speed_metres_per_second *
                               ship.handling.world_recovery_speed_fraction,
                           0.01F) &&
              !falling.vehicle.boosting &&
              nearly_equal(falling.handling.sustained_slip_intensity, 0.0F),
          "falling recovers once to the last safe pose with reset transient handling state");
}

void test_telemetry_reconstructs_authoritative_velocity() {
    const hover::game::SampledTrack track = make_handling_lab_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    state.physical.velocity = state.physical.basis.forward * 180.0F;
    state.vehicle.forward_speed_metres_per_second = 180.0F;

    const hover::game::WorldTrackVehicleTickResult result =
        hover::game::simulate_world_track_vehicle(
            state, hover::game::WorldTrackVehicleTick{
                       {.steering = 0.75F, .throttle = 1.0F, .drift_right = true},
                       ship,
                       path,
                       tick_seconds});
    const hover::math::Vec3 right = hover::math::normalized(
        hover::math::cross(state.physical.basis.up, state.physical.basis.forward));
    const hover::math::Vec3 reconstructed =
        state.physical.basis.forward * result.telemetry.local_forward_speed_metres_per_second +
        right * result.telemetry.local_lateral_speed_metres_per_second +
        state.physical.basis.up * result.telemetry.local_normal_speed_metres_per_second;

    check(hover::game::is_valid(result.telemetry) &&
              nearly_equal(reconstructed, state.physical.velocity, 0.01F) &&
              nearly_equal(
                  result.telemetry.world_speed_metres_per_second,
                  std::sqrt(hover::math::dot(state.physical.velocity, state.physical.velocity)),
                  0.01F),
          "telemetry local components reconstruct the completed tick's world velocity");
}

void test_local_axis_damping_uses_time_correct_exponential_rates() {
    const hover::game::SampledTrack track = make_handling_lab_track();
    hover::game::ShipDefinition ship = test_ship();
    ship.handling.world_lateral_grip_deceleration_metres_per_second_squared = 0.0F;
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    state.contact.mode = hover::game::VehicleContactMode::airborne;
    const hover::math::Vec3 right = hover::math::normalized(
        hover::math::cross(state.physical.basis.up, state.physical.basis.forward));
    state.physical.velocity =
        state.physical.basis.forward * 100.0F + right * 20.0F + state.physical.basis.up * 10.0F;
    state.vehicle.forward_speed_metres_per_second = 100.0F;

    const hover::game::WorldTrackVehicleTickResult result =
        hover::game::simulate_world_track_vehicle(
            state, hover::game::WorldTrackVehicleTick{{}, ship, path, tick_seconds});
    const float expected_forward =
        100.0F * std::exp(-ship.handling.world_forward_damping_per_second * tick_seconds);
    const float expected_lateral =
        20.0F * std::exp(-ship.handling.world_lateral_damping_per_second * tick_seconds);
    const float expected_normal_deceleration =
        (10.0F - 10.0F * std::exp(-ship.handling.world_normal_damping_per_second * tick_seconds)) /
        tick_seconds;

    check(nearly_equal(result.telemetry.local_forward_speed_metres_per_second, expected_forward,
                       0.01F) &&
              nearly_equal(result.telemetry.local_lateral_speed_metres_per_second, expected_lateral,
                           0.01F) &&
              nearly_equal(result.telemetry.normal_damping_deceleration_metres_per_second_squared,
                           expected_normal_deceleration, 0.01F),
          "world velocity is damped in local axes with elapsed-time exponential rates");
}

void test_speed_curve_and_propulsion_response() {
    const hover::game::SampledTrack track = make_handling_lab_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState low =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    hover::game::WorldTrackVehicleState high = low;
    const float base_speed = ship.handling.base_maximum_forward_speed_metres_per_second;
    high.physical.velocity = high.physical.basis.forward * base_speed;
    high.vehicle.forward_speed_metres_per_second = base_speed;

    const hover::game::WorldTrackVehicleTickResult low_first =
        hover::game::simulate_world_track_vehicle(
            low, hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
    const hover::game::WorldTrackVehicleTickResult high_first =
        hover::game::simulate_world_track_vehicle(
            high, hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
    const float low_applied_first =
        low_first.telemetry.applied_propulsion_acceleration_metres_per_second_squared;
    const hover::game::WorldTrackVehicleTickResult low_second =
        hover::game::simulate_world_track_vehicle(
            low, hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});

    check(
        low_first.telemetry.requested_propulsion_acceleration_metres_per_second_squared >
                high_first.telemetry.requested_propulsion_acceleration_metres_per_second_squared &&
            nearly_equal(low_first.telemetry.propulsion_curve_multiplier, 1.0F) &&
            high_first.telemetry.propulsion_curve_multiplier < 0.82F,
        "speed-shaped propulsion requests less acceleration near base maximum speed");
    check(low_applied_first > 0.0F &&
              low_applied_first <
                  low_first.telemetry.requested_propulsion_acceleration_metres_per_second_squared &&
              low_second.telemetry.applied_propulsion_acceleration_metres_per_second_squared >
                  low_applied_first,
          "positive propulsion rises smoothly toward its requested target");

    for (int tick = 0; tick < 20; ++tick) {
        hover::game::simulate_world_track_vehicle(
            low, hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
    }
    const hover::game::WorldTrackVehicleTickResult reduced =
        hover::game::simulate_world_track_vehicle(
            low, hover::game::WorldTrackVehicleTick{{.throttle = 0.25F}, ship, path, tick_seconds});
    const float reduced_target =
        reduced.telemetry.requested_propulsion_acceleration_metres_per_second_squared *
        reduced.telemetry.propulsion_fraction;
    const hover::game::WorldTrackVehicleTickResult braking =
        hover::game::simulate_world_track_vehicle(
            low, hover::game::WorldTrackVehicleTick{{.brake = 1.0F}, ship, path, tick_seconds});
    check(
        nearly_equal(reduced.telemetry.applied_propulsion_acceleration_metres_per_second_squared,
                     reduced_target) &&
            nearly_equal(
                braking.telemetry.applied_propulsion_acceleration_metres_per_second_squared, 0.0F),
        "lower propulsion targets apply immediately and braking clears positive response");
}

void test_sustained_slip_builds_and_releases() {
    const hover::game::SampledTrack track = make_handling_lab_track();
    hover::game::ShipDefinition ship = test_ship();
    ship.handling.world_lateral_damping_per_second = 0.0F;
    ship.handling.world_lateral_grip_deceleration_metres_per_second_squared = 0.0F;
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    const hover::math::Vec3 right = hover::math::normalized(
        hover::math::cross(state.physical.basis.up, state.physical.basis.forward));
    state.physical.velocity =
        state.physical.basis.forward * 100.0F +
        right * (ship.handling.world_slip_speed_threshold_metres_per_second + 2.0F);
    state.vehicle.forward_speed_metres_per_second = 100.0F;

    const int buildup_ticks = static_cast<int>(
        std::ceil(ship.handling.world_sustained_slip_buildup_seconds / tick_seconds));
    for (int tick = 0; tick < buildup_ticks; ++tick) {
        hover::game::simulate_world_track_vehicle(
            state,
            hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
    }
    check(state.handling.sustained_slip_seconds >=
                  ship.handling.world_sustained_slip_buildup_seconds - tick_seconds &&
              nearly_equal(state.handling.sustained_slip_intensity, 1.0F, 0.01F),
          "lateral speed above the threshold builds full sustained-slip response");

    const float intensity_before_release = state.handling.sustained_slip_intensity;
    state.physical.velocity = state.physical.basis.forward * 100.0F;
    const hover::game::WorldTrackVehicleTickResult release =
        hover::game::simulate_world_track_vehicle(
            state,
            hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
    check(nearly_equal(state.handling.sustained_slip_seconds, 0.0F) &&
              release.telemetry.sustained_slip_intensity < intensity_before_release &&
              release.telemetry.sustained_slip_intensity > 0.0F,
          "dropping below the threshold resets buildup but releases intensity gradually");

    const int release_ticks = static_cast<int>(
        std::ceil(ship.handling.world_sustained_slip_release_seconds / tick_seconds));
    for (int tick = 0; tick < release_ticks; ++tick) {
        hover::game::simulate_world_track_vehicle(
            state,
            hover::game::WorldTrackVehicleTick{{.throttle = 1.0F}, ship, path, tick_seconds});
    }
    check(nearly_equal(state.handling.sustained_slip_intensity, 0.0F),
          "sustained-slip intensity reaches zero after its configured release time");
}

struct FixedRateSchedule {
    float tick_seconds;
    int tick_count;
};

hover::game::WorldTrackVehicleState simulate_at_fixed_rate(const hover::game::SampledTrack& track,
                                                           const hover::game::ShipDefinition& ship,
                                                           FixedRateSchedule schedule) {
    const hover::game::ResolvedTrackPath path{primary_path, track};
    hover::game::WorldTrackVehicleState state =
        hover::game::make_world_track_vehicle_state({}, ship, path);
    state.physical.velocity = state.physical.basis.forward * 120.0F;
    state.vehicle.forward_speed_metres_per_second = 120.0F;
    for (int tick = 0; tick < schedule.tick_count; ++tick) {
        hover::game::simulate_world_track_vehicle(
            state, hover::game::WorldTrackVehicleTick{
                       {.throttle = 1.0F}, ship, path, schedule.tick_seconds});
    }
    return state;
}

void test_60_and_120_hz_grounded_responses_converge() {
    const hover::game::SampledTrack track = make_handling_lab_track();
    const hover::game::ShipDefinition ship = test_ship();
    const hover::game::WorldTrackVehicleState at_60 =
        simulate_at_fixed_rate(track, ship, {1.0F / 60.0F, 60});
    const hover::game::WorldTrackVehicleState at_120 =
        simulate_at_fixed_rate(track, ship, {1.0F / 120.0F, 120});

    check(nearly_equal(at_60.vehicle.forward_speed_metres_per_second,
                       at_120.vehicle.forward_speed_metres_per_second, 0.5F) &&
              nearly_equal(at_60.physical.position, at_120.physical.position, 1.5F) &&
              nearly_equal(
                  at_60.handling.applied_propulsion_acceleration_metres_per_second_squared,
                  at_120.handling.applied_propulsion_acceleration_metres_per_second_squared, 0.5F),
          "60 and 120 Hz grounded damping and propulsion converge over equal elapsed time");
}

void test_scripted_handling_traces_are_deterministic() {
    const hover::game::SampledTrack track = make_handling_lab_track();
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    const float base_speed = ship.handling.base_maximum_forward_speed_metres_per_second;

    const std::array straight_acceleration{
        ScriptStep{120U, {.throttle = 1.0F}},
    };
    const std::array full_speed_steering{
        ScriptStep{60U, {.steering = 1.0F, .throttle = 1.0F}},
    };
    const std::array boost_into_steering{
        ScriptStep{1U, {.steering = 1.0F, .throttle = 1.0F, .boost = true}},
        ScriptStep{59U, {.steering = 1.0F, .throttle = 1.0F}},
    };
    const std::array drift_entry_sustain_release{
        ScriptStep{1U, {.throttle = 1.0F, .drift_right = true}},
        ScriptStep{59U, {.throttle = 1.0F, .drift_right = true}},
        ScriptStep{60U, {.steering = -0.35F, .throttle = 1.0F}},
        ScriptStep{30U, {}},
    };
    const std::array brake_during_boost_and_drift{
        ScriptStep{1U, {.throttle = 1.0F, .drift_left = true, .boost = true}},
        ScriptStep{29U, {.throttle = 1.0F, .drift_left = true}},
        ScriptStep{30U, {.brake = 1.0F, .drift_left = true}},
    };

    check_trace_is_valid_and_repeatable(track, ship, straight_acceleration, 0.0F,
                                        "straight acceleration trace is valid and repeatable");
    check_trace_is_valid_and_repeatable(track, ship, full_speed_steering, base_speed,
                                        "full-speed steering trace is valid and repeatable");
    check_trace_is_valid_and_repeatable(track, ship, boost_into_steering, base_speed,
                                        "boost-turn trace is valid and repeatable");
    check_trace_is_valid_and_repeatable(track, ship, drift_entry_sustain_release, base_speed,
                                        "drift entry, release, and coast trace is repeatable");
    check_trace_is_valid_and_repeatable(track, ship, brake_during_boost_and_drift, base_speed,
                                        "boosted drift braking trace is valid and repeatable");

    const std::vector<TraceSample> straight = run_script(track, ship, straight_acceleration, 0.0F);
    const std::vector<TraceSample> steering =
        run_script(track, ship, full_speed_steering, base_speed);
    const std::vector<TraceSample> boosted =
        run_script(track, ship, boost_into_steering, base_speed);
    const std::vector<TraceSample> drift =
        run_script(track, ship, drift_entry_sustain_release, base_speed);

    check(std::abs(straight.back().result.telemetry.local_lateral_speed_metres_per_second) <=
                  tolerance &&
              std::abs(straight.back().result.telemetry.signed_slip_angle_radians) <= tolerance,
          "straight trace remains free of lateral velocity and slip");
    check(std::abs(steering.back().result.telemetry.local_lateral_speed_metres_per_second) > 1.0F &&
              steering.back().result.telemetry.sustained_slip_intensity > 0.0F,
          "full-speed steering trace develops measurable slip and sustained-slip response");
    check(boosted.front().result.events.boost_activated &&
              boosted.back().result.telemetry.world_speed_metres_per_second >
                  steering.back().result.telemetry.world_speed_metres_per_second,
          "boost-turn trace records activation and a distinct speed response");
    check(
        drift.front().result.telemetry.drift_force_fraction >
                drift[59].result.telemetry.drift_force_fraction &&
            drift[60].result.telemetry.drift_direction == 0.0F &&
            std::abs(drift[60].result.telemetry.local_lateral_speed_metres_per_second) > 0.5F &&
            nearly_equal(
                drift.back()
                    .result.telemetry.requested_propulsion_acceleration_metres_per_second_squared,
                0.0F) &&
            nearly_equal(
                drift.back()
                    .result.telemetry.applied_propulsion_acceleration_metres_per_second_squared,
                0.0F) &&
            drift.back().result.telemetry.forward_damping_deceleration_metres_per_second_squared >
                0.0F,
        "drift trace captures force fade, persistent release momentum, and throttle-release loss");
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

    const hover::game::WorldTrackVehicleTickResult activation =
        hover::game::simulate_world_track_vehicle(
            state, hover::game::WorldTrackVehicleTick{
                       {.throttle = 1.0F, .boost = true}, ship, path, tick_seconds});
    const float boosted_speed = state.vehicle.forward_speed_metres_per_second;
    check(activation.events.boost_activated && state.vehicle.boosting && boosted_speed > base_speed,
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
    test_speed_slip_and_recovery_inputs_shape_available_traction();
    test_directional_drift_and_both_held_policy();
    test_drift_force_fades_as_same_direction_slide_builds();
    test_drift_suppresses_propulsion_and_loses_forward_speed();
    test_sustained_high_speed_steering_turns_slip_into_speed_loss();
    test_world_integration_derives_progress_and_wraps_seam();
    test_banked_basis_and_solid_wall_response();
    test_hover_forces_takeoff_landing_and_penetration();
    test_mirrored_walls_and_open_edge_recovery();
    test_telemetry_reconstructs_authoritative_velocity();
    test_local_axis_damping_uses_time_correct_exponential_rates();
    test_speed_curve_and_propulsion_response();
    test_sustained_slip_builds_and_releases();
    test_60_and_120_hz_grounded_responses_converge();
    test_scripted_handling_traces_are_deterministic();
    test_boost_and_brake_update_world_forward_velocity();
    test_world_simulation_is_independent_from_render_schedule();

    if (failure_count != 0) {
        std::cerr << failure_count << " world-track vehicle simulation test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All world-track vehicle simulation tests passed\n";
    return EXIT_SUCCESS;
}
