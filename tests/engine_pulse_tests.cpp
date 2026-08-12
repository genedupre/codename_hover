#include "render/engine_pulse.hpp"

#include <cmath>
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

void test_visibility_tracks_positive_propulsion() {
    const hover::render::EnginePulseSample idle = hover::render::sample_engine_pulse(1.0, 0.0F);
    const hover::render::EnginePulseSample braking = hover::render::sample_engine_pulse(1.0, -1.0F);
    const hover::render::EnginePulseSample accelerating =
        hover::render::sample_engine_pulse(1.0, 1.0F);

    check(!idle.visible && idle.uniform_scale == 0.0F, "idle propulsion produces no engine pulse");
    check(!braking.visible && braking.uniform_scale == 0.0F,
          "negative propulsion produces no engine pulse");
    check(accelerating.visible && accelerating.uniform_scale > 0.0F,
          "positive propulsion produces a visible engine pulse");
}

void test_intensity_and_time_give_the_pulse_life() {
    const hover::render::EnginePulseSample partial =
        hover::render::sample_engine_pulse(0.137, 0.25F);
    const hover::render::EnginePulseSample full = hover::render::sample_engine_pulse(0.137, 1.0F);
    const hover::render::EnginePulseSample later = hover::render::sample_engine_pulse(0.173, 1.0F);

    check(full.uniform_scale > partial.uniform_scale,
          "stronger propulsion produces a larger pulse at the same phase");
    check(std::abs(full.uniform_scale - later.uniform_scale) > 0.001F,
          "two frequencies vary pulse scale over elapsed presentation time");
}

void test_invalid_presentation_input_fails_closed() {
    const hover::render::EnginePulseSample invalid_time =
        hover::render::sample_engine_pulse(std::numeric_limits<double>::infinity(), 1.0F);
    const hover::render::EnginePulseSample invalid_intensity =
        hover::render::sample_engine_pulse(1.0, std::numeric_limits<float>::quiet_NaN());

    check(!invalid_time.visible && !invalid_intensity.visible,
          "non-finite presentation input hides the pulse safely");
}

} // namespace

int main() {
    test_visibility_tracks_positive_propulsion();
    test_intensity_and_time_give_the_pulse_life();
    test_invalid_presentation_input_fails_closed();

    if (failure_count != 0) {
        std::cerr << failure_count << " engine-pulse test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All engine-pulse tests passed\n";
    return EXIT_SUCCESS;
}
