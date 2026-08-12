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
    const hover::render::EnginePulseSample idle =
        hover::render::sample_engine_pulse(1.0, 0.0F, 0.5F);
    const hover::render::EnginePulseSample braking =
        hover::render::sample_engine_pulse(1.0, -1.0F, 0.5F);
    const hover::render::EnginePulseSample accelerating =
        hover::render::sample_engine_pulse(1.0, 1.0F, 0.5F);

    check(!idle.visible && idle.radial_scale == 0.0F && idle.length_scale == 0.0F,
          "idle propulsion produces no engine pulse");
    check(!braking.visible && braking.radial_scale == 0.0F && braking.length_scale == 0.0F,
          "negative propulsion produces no engine pulse");
    check(accelerating.visible && accelerating.radial_scale > 0.0F &&
              accelerating.length_scale > 0.0F,
          "positive propulsion produces a visible engine pulse");
}

void test_release_envelope_shrinks_before_switching_off() {
    const float ignited = hover::render::advance_engine_pulse_intensity(0.0F, 1.0F, 0.016);
    const float releasing = hover::render::advance_engine_pulse_intensity(ignited, 0.0F, 0.10);
    const float released = hover::render::advance_engine_pulse_intensity(releasing, 0.0F, 0.10);

    check(ignited == 1.0F, "engine pulse ignition responds immediately");
    check(releasing > 0.0F && releasing < ignited,
          "released propulsion leaves a shrinking pulse briefly visible");
    check(released == 0.0F, "engine pulse switches off after its short release envelope");

    float stepped_release = 1.0F;
    for (int step = 0; step < 10; ++step) {
        stepped_release =
            hover::render::advance_engine_pulse_intensity(stepped_release, 0.0F, 0.01);
    }
    check(std::abs(stepped_release - releasing) <= 0.0001F,
          "release duration does not depend on presentation frame rate");
}

void test_speed_intensity_and_time_shape_the_pulse() {
    const hover::render::EnginePulseSample partial =
        hover::render::sample_engine_pulse(0.0, 0.25F, 0.5F);
    const hover::render::EnginePulseSample full =
        hover::render::sample_engine_pulse(0.0, 1.0F, 0.5F);
    const hover::render::EnginePulseSample stopped =
        hover::render::sample_engine_pulse(0.0, 1.0F, 0.0F);
    const hover::render::EnginePulseSample fast =
        hover::render::sample_engine_pulse(0.0, 1.0F, 1.0F);
    const hover::render::EnginePulseSample later =
        hover::render::sample_engine_pulse(0.173, 1.0F, 0.5F);

    check(full.radial_scale > partial.radial_scale,
          "stronger propulsion produces a wider pulse at the same speed and phase");
    check(fast.length_scale > stopped.length_scale,
          "higher vehicle speed produces a longer pulse at the same propulsion and phase");
    check(fast.radial_scale > stopped.radial_scale,
          "higher vehicle speed produces a wider pulse at the same propulsion and phase");
    check(fast.length_scale > stopped.length_scale * 5.0F,
          "top speed has a substantially stronger effect on plume length");
    check(std::abs(full.radial_scale - later.radial_scale) > 0.001F &&
              std::abs(full.length_scale - later.length_scale) > 0.001F,
          "two frequencies vary pulse width and length over presentation time");
}

void test_invalid_presentation_input_fails_closed() {
    const hover::render::EnginePulseSample invalid_time =
        hover::render::sample_engine_pulse(std::numeric_limits<double>::infinity(), 1.0F, 1.0F);
    const hover::render::EnginePulseSample invalid_intensity =
        hover::render::sample_engine_pulse(1.0, std::numeric_limits<float>::quiet_NaN(), 1.0F);
    const hover::render::EnginePulseSample invalid_speed =
        hover::render::sample_engine_pulse(1.0, 1.0F, std::numeric_limits<float>::infinity());
    const float invalid_envelope = hover::render::advance_engine_pulse_intensity(
        1.0F, 0.0F, std::numeric_limits<double>::quiet_NaN());

    check(!invalid_time.visible && !invalid_intensity.visible && !invalid_speed.visible &&
              invalid_envelope == 0.0F,
          "non-finite presentation input hides the pulse safely");
}

} // namespace

int main() {
    test_visibility_tracks_positive_propulsion();
    test_release_envelope_shrinks_before_switching_off();
    test_speed_intensity_and_time_shape_the_pulse();
    test_invalid_presentation_input_fails_closed();

    if (failure_count != 0) {
        std::cerr << failure_count << " engine-pulse test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All engine-pulse tests passed\n";
    return EXIT_SUCCESS;
}
