#include "game/track_vehicle_state.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

hover::game::TrackVehicleState valid_state() {
    return hover::game::TrackVehicleState{
        .location =
            {
                .path = hover::game::TrackPathId{7U},
                .distance_along_path_metres = 120.0F,
                .lateral_offset_metres = -3.0F,
            },
        .forward_speed_metres_per_second = 80.0F,
        .lateral_velocity_metres_per_second = -2.0F,
        .normal_offset_metres = 0.62F,
        .normal_velocity_metres_per_second = 1.5F,
    };
}

void test_valid_track_bound_state() {
    const hover::game::TrackVehicleState state = valid_state();
    check(hover::game::is_valid(state),
          "assigned path and finite path-local vehicle values are valid");
    check(state.location.lateral_offset_metres < 0.0F &&
              state.lateral_velocity_metres_per_second < 0.0F &&
              state.normal_velocity_metres_per_second > 0.0F,
          "path-local lateral and normal movement retain their direction");
}

void test_unassigned_and_noncanonical_state_is_invalid() {
    hover::game::TrackVehicleState state = valid_state();
    state.location.path = {};
    check(!hover::game::is_valid(state), "an unassigned path is invalid");

    state = valid_state();
    state.location.distance_along_path_metres = -0.01F;
    check(!hover::game::is_valid(state), "stored path distance must be canonical and nonnegative");

    state = valid_state();
    state.forward_speed_metres_per_second = -0.01F;
    check(!hover::game::is_valid(state), "track-bound forward speed cannot be negative");

    state = valid_state();
    state.normal_offset_metres = -0.01F;
    check(!hover::game::is_valid(state), "normal offset cannot pass through the track surface");
}

void test_nonfinite_state_is_invalid() {
    constexpr float infinity = std::numeric_limits<float>::infinity();
    constexpr float not_a_number = std::numeric_limits<float>::quiet_NaN();

    hover::game::TrackVehicleState state = valid_state();
    state.location.lateral_offset_metres = infinity;
    check(!hover::game::is_valid(state), "infinite lateral offset is invalid");

    state = valid_state();
    state.lateral_velocity_metres_per_second = not_a_number;
    check(!hover::game::is_valid(state), "non-finite lateral velocity is invalid");

    state = valid_state();
    state.normal_velocity_metres_per_second = infinity;
    check(!hover::game::is_valid(state), "non-finite normal velocity is invalid");
}

} // namespace

int main() {
    test_valid_track_bound_state();
    test_unassigned_and_noncanonical_state_is_invalid();
    test_nonfinite_state_is_invalid();

    if (failure_count != 0) {
        std::cerr << failure_count << " track-vehicle-state test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All track-vehicle-state tests passed\n";
    return EXIT_SUCCESS;
}
