#include "core/fixed_step.hpp"
#include "game/ships/prototype_01.hpp"
#include "game/vehicle_simulation.hpp"
#include "input/player_input.hpp"

#include <algorithm>
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
    const hover::game::VehicleState after_30_fps_rendering =
        simulate_with_render_schedule(RenderSchedule{test_duration_seconds, 30.0});
    const hover::game::VehicleState after_144_fps_rendering =
        simulate_with_render_schedule(RenderSchedule{test_duration_seconds, 144.0});

    check(nearly_equal(after_30_fps_rendering.forward_speed_metres_per_second,
                       after_144_fps_rendering.forward_speed_metres_per_second),
          "render rate does not change one second of acceleration");
    check(nearly_equal(after_30_fps_rendering.pose.position.z,
                       after_144_fps_rendering.pose.position.z),
          "render rate does not change one second of travel");
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
    check(state.pose.position.x > 0.0F && state.pose.position.z > 0.0F,
          "turned vehicle moves along its local forward direction");

    const hover::game::VehiclePose halfway = hover::game::interpolate(
        hover::game::VehiclePose{}, hover::game::VehiclePose{{10.0F, 2.0F, 4.0F}, 1.0F}, 0.5F);
    check(nearly_equal(halfway.position.x, 5.0F) && nearly_equal(halfway.position.y, 1.0F) &&
              nearly_equal(halfway.position.z, 2.0F) && nearly_equal(halfway.yaw_radians, 0.5F),
          "render pose interpolates position and heading");
}

} // namespace

int main() {
    test_render_rate_independence();
    test_catch_up_limit();
    test_vehicle_pose_and_interpolation();

    if (failure_count != 0) {
        std::cerr << failure_count << " simulation test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All simulation tests passed\n";
    return EXIT_SUCCESS;
}
