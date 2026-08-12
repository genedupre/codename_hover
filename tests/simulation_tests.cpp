#include "core/fixed_step.hpp"
#include "game/ships/prototype_01.hpp"
#include "game/vehicle_simulation.hpp"
#include "input/player_input.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

constexpr float tolerance = 0.001F;
int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

bool nearly_equal(float left, float right) { return std::abs(left - right) <= tolerance; }

struct RenderSchedule {
    double duration_seconds;
    double frames_per_second;
};

hover::game::VehicleState simulate_with_render_schedule(RenderSchedule schedule) {
    constexpr double tick_seconds = 1.0 / 90.0;
    hover::core::FixedStepAccumulator accumulator{
        hover::core::FixedStepConfig{tick_seconds, 0.25, 30}};
    hover::game::VehicleState state{};
    const hover::input::PlayerInput input{.throttle = 1.0F};

    const double nominal_frame_seconds = 1.0 / schedule.frames_per_second;
    double submitted_seconds = 0.0;
    while (submitted_seconds < schedule.duration_seconds) {
        const double frame_end =
            std::min(submitted_seconds + nominal_frame_seconds, schedule.duration_seconds);
        const hover::core::FixedStepPlan plan = accumulator.advance(frame_end - submitted_seconds);
        submitted_seconds = frame_end;
        for (std::uint32_t tick = 0; tick < plan.tick_count; ++tick) {
            hover::game::simulate_vehicle(
                state,
                hover::game::VehicleTick{input, hover::game::ships::prototype_01_definition(),
                                         static_cast<float>(tick_seconds)});
        }
    }
    return state;
}

void test_render_rate_independence() {
    constexpr double test_duration_seconds = 1.0;
    constexpr std::array render_rates{24.0,  30.0,  60.0,  90.0,  120.0, 144.0,
                                      165.0, 240.0, 244.0, 360.0, 500.0};
    const hover::game::VehicleState reference =
        simulate_with_render_schedule(RenderSchedule{test_duration_seconds, 90.0});

    for (double render_rate : render_rates) {
        const hover::game::VehicleState candidate =
            simulate_with_render_schedule(RenderSchedule{test_duration_seconds, render_rate});
        check(nearly_equal(candidate.forward_speed_metres_per_second,
                           reference.forward_speed_metres_per_second),
              "render rate does not change one second of acceleration");
        check(nearly_equal(candidate.pose.position.z, reference.pose.position.z),
              "render rate does not change one second of travel");
    }
}

void test_catch_up_limit() {
    hover::core::FixedStepAccumulator accumulator{
        hover::core::FixedStepConfig{1.0 / 90.0, 0.25, 8}};
    const hover::core::FixedStepPlan plan = accumulator.advance(2.0);

    check(plan.tick_count == 8U, "long frames obey the maximum tick count");
    check(plan.dropped_time, "long frames report dropped accumulated time");
    check(plan.interpolation_alpha >= 0.0F && plan.interpolation_alpha < 1.0F,
          "catch-up keeps interpolation alpha in range");
}

void test_vehicle_pose_and_interpolation() {
    hover::game::VehicleState state{};
    const hover::input::PlayerInput input{.steering = 1.0F, .throttle = 1.0F};
    hover::game::simulate_vehicle(
        state, hover::game::VehicleTick{input, hover::game::ships::prototype_01_definition(),
                                        1.0F / 90.0F});

    check(state.forward_speed_metres_per_second > 0.0F, "throttle accelerates the vehicle");
    check(state.pose.yaw_radians > 0.0F, "positive steering turns the vehicle right");
    check(state.pose.turn_roll_radians < 0.0F,
          "a right turn visually lowers the ship's right wing");
    check(state.pose.position.x > 0.0F && state.pose.position.z > 0.0F,
          "turned vehicle moves along its local forward direction");

    const hover::game::VehiclePose halfway = hover::game::interpolate(
        hover::game::VehiclePose{},
        hover::game::VehiclePose{{10.0F, 2.0F, 4.0F}, 1.0F, -0.2F}, 0.5F);
    check(nearly_equal(halfway.position.x, 5.0F) && nearly_equal(halfway.position.y, 1.0F) &&
              nearly_equal(halfway.position.z, 2.0F) && nearly_equal(halfway.yaw_radians, 0.5F) &&
              nearly_equal(halfway.turn_roll_radians, -0.1F),
          "render pose interpolates position, heading, and turn roll");
}

void test_coasting_and_speed_scaled_turn_roll() {
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();
    hover::game::VehicleState coasting{};
    coasting.forward_speed_metres_per_second = 100.0F;
    hover::game::simulate_vehicle(
        coasting, hover::game::VehicleTick{{}, ship, 1.0F});
    check(nearly_equal(coasting.forward_speed_metres_per_second, 76.0F),
          "one second without propulsion applies the stronger coasting slowdown");

    hover::game::VehicleState slow_turn{};
    slow_turn.forward_speed_metres_per_second =
        ship.handling.maximum_forward_speed_metres_per_second * 0.1F;
    hover::game::VehicleState fast_turn{};
    fast_turn.forward_speed_metres_per_second =
        ship.handling.maximum_forward_speed_metres_per_second;
    const hover::input::PlayerInput right_turn{.steering = 1.0F, .throttle = 1.0F};
    constexpr float tick_seconds = 1.0F / 90.0F;
    hover::game::simulate_vehicle(slow_turn,
                                  hover::game::VehicleTick{right_turn, ship, tick_seconds});
    hover::game::simulate_vehicle(fast_turn,
                                  hover::game::VehicleTick{right_turn, ship, tick_seconds});

    check(fast_turn.pose.turn_roll_radians < slow_turn.pose.turn_roll_radians &&
              slow_turn.pose.turn_roll_radians < 0.0F,
          "turn roll is stronger at high speed and follows steering direction");

    const float initial_roll = fast_turn.pose.turn_roll_radians;
    hover::game::simulate_vehicle(
        fast_turn, hover::game::VehicleTick{{.throttle = 1.0F}, ship, tick_seconds});
    check(std::abs(fast_turn.pose.turn_roll_radians) < std::abs(initial_roll),
          "ship begins returning level when steering is released");
}

} // namespace

int main() {
    test_render_rate_independence();
    test_catch_up_limit();
    test_vehicle_pose_and_interpolation();
    test_coasting_and_speed_scaled_turn_roll();

    if (failure_count != 0) {
        std::cerr << failure_count << " simulation test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All simulation tests passed\n";
    return EXIT_SUCCESS;
}
