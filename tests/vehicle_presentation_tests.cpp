#include "render/vehicle_presentation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

constexpr float tolerance = 0.0001F;
int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

void test_boost_camera_requires_speed_then_latches_for_the_burst() {
    hover::render::BoostCameraFeedbackState feedback{};
    hover::render::advance_boost_camera_feedback(feedback, 0.1, true, 0.64F);
    check(feedback.intensity == 0.0F && !feedback.activated_for_current_boost,
          "low-speed boost does not activate camera feedback");

    hover::render::advance_boost_camera_feedback(feedback, 0.05, true, 0.65F);
    check(std::abs(feedback.intensity - 0.5F) <= tolerance && feedback.activated_for_current_boost,
          "reaching the minimum speed during boost begins the camera response");

    hover::render::advance_boost_camera_feedback(feedback, 0.05, true, 0.30F);
    check(std::abs(feedback.intensity - 1.0F) <= tolerance && feedback.activated_for_current_boost,
          "camera feedback stays latched through the active boost after activation");
}

void test_boost_camera_releases_smoothly_and_frame_rate_independently() {
    hover::render::BoostCameraFeedbackState one_step{1.0F, true};
    hover::render::BoostCameraFeedbackState ten_steps{1.0F, true};
    hover::render::advance_boost_camera_feedback(one_step, 0.1, false, 1.1F);
    for (int step = 0; step < 10; ++step) {
        hover::render::advance_boost_camera_feedback(ten_steps, 0.01, false, 1.1F);
    }

    check(one_step.intensity > 0.0F && one_step.intensity < 1.0F,
          "camera feedback eases out after boost instead of snapping off");
    check(std::abs(one_step.intensity - ten_steps.intensity) <= tolerance,
          "camera release duration does not depend on presentation frame rate");
    check(!one_step.activated_for_current_boost && !ten_steps.activated_for_current_boost,
          "ending boost clears the activation latch for the next burst");
}

void test_boost_camera_sample_expands_the_speed_view() {
    const hover::render::BoostCameraSample normal = hover::render::sample_boost_camera(0.0F);
    const hover::render::BoostCameraSample boosted = hover::render::sample_boost_camera(1.0F);

    check(boosted.follow_distance_metres > normal.follow_distance_metres,
          "boost camera pulls back from the ship");
    check(boosted.look_ahead_metres > normal.look_ahead_metres,
          "boost camera looks farther along the direction of travel");
    check(boosted.vertical_field_of_view_radians > normal.vertical_field_of_view_radians,
          "boost camera widens its field of view");
}

void test_invalid_boost_camera_input_fails_closed() {
    hover::render::BoostCameraFeedbackState feedback{1.0F, true};
    hover::render::advance_boost_camera_feedback(feedback, std::numeric_limits<double>::quiet_NaN(),
                                                 true, 1.0F);
    const hover::render::BoostCameraSample sample =
        hover::render::sample_boost_camera(std::numeric_limits<float>::infinity());
    const hover::render::BoostCameraSample normal = hover::render::sample_boost_camera(0.0F);

    check(feedback.intensity == 0.0F && !feedback.activated_for_current_boost,
          "invalid camera timing resets presentation state safely");
    check(std::abs(sample.follow_distance_metres - normal.follow_distance_metres) <= tolerance &&
              std::abs(sample.look_ahead_metres - normal.look_ahead_metres) <= tolerance &&
              std::abs(sample.vertical_field_of_view_radians -
                       normal.vertical_field_of_view_radians) <= tolerance,
          "invalid camera intensity samples the normal camera");
}

} // namespace

int main() {
    test_boost_camera_requires_speed_then_latches_for_the_burst();
    test_boost_camera_releases_smoothly_and_frame_rate_independently();
    test_boost_camera_sample_expands_the_speed_view();
    test_invalid_boost_camera_input_fails_closed();

    if (failure_count != 0) {
        std::cerr << failure_count << " vehicle-presentation test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All vehicle-presentation tests passed\n";
    return EXIT_SUCCESS;
}
