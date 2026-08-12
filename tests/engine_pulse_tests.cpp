#include "render/engine_pulse.hpp"
#include "render/vehicle_presentation.hpp"

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
        hover::render::sample_engine_pulse(1.0, 0.0F, 0.5F, false);
    const hover::render::EnginePulseSample braking =
        hover::render::sample_engine_pulse(1.0, -1.0F, 0.5F, false);
    const hover::render::EnginePulseSample accelerating =
        hover::render::sample_engine_pulse(1.0, 1.0F, 0.5F, false);

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
        hover::render::sample_engine_pulse(0.0, 0.25F, 0.5F, false);
    const hover::render::EnginePulseSample full =
        hover::render::sample_engine_pulse(0.0, 1.0F, 0.5F, false);
    const hover::render::EnginePulseSample stopped =
        hover::render::sample_engine_pulse(0.0, 1.0F, 0.0F, false);
    const hover::render::EnginePulseSample fast =
        hover::render::sample_engine_pulse(0.0, 1.0F, 1.0F, false);
    const hover::render::EnginePulseSample later =
        hover::render::sample_engine_pulse(0.173, 1.0F, 0.5F, false);

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

void test_boost_has_a_distinct_large_flare_response() {
    const hover::render::EnginePulseSample normal =
        hover::render::sample_engine_pulse(0.0, 1.0F, 1.0F, false);
    const hover::render::EnginePulseSample boost =
        hover::render::sample_engine_pulse(0.0, 1.0F, 1.0F, true);

    check(!normal.boost_flare_visible && boost.boost_flare_visible &&
              boost.boost_flare_scale > 0.0F,
          "boost activates geometry that normal acceleration does not draw");
    check(boost.radial_scale > normal.radial_scale && boost.length_scale > normal.length_scale,
          "boost makes the core and outer plume react substantially");
}

void test_invalid_presentation_input_fails_closed() {
    const hover::render::EnginePulseSample invalid_time =
        hover::render::sample_engine_pulse(std::numeric_limits<double>::infinity(), 1.0F, 1.0F,
                                           false);
    const hover::render::EnginePulseSample invalid_intensity =
        hover::render::sample_engine_pulse(1.0, std::numeric_limits<float>::quiet_NaN(), 1.0F,
                                           false);
    const hover::render::EnginePulseSample invalid_speed =
        hover::render::sample_engine_pulse(1.0, 1.0F,
                                           std::numeric_limits<float>::infinity(), false);
    const float invalid_envelope = hover::render::advance_engine_pulse_intensity(
        1.0F, 0.0F, std::numeric_limits<double>::quiet_NaN());

    check(!invalid_time.visible && !invalid_intensity.visible && !invalid_speed.visible &&
              invalid_envelope == 0.0F,
          "non-finite presentation input hides the pulse safely");
}

void test_full_speed_vibration_is_subtle_and_speed_gated() {
    const hover::math::Vec3 below_onset =
        hover::render::sample_full_speed_vibration(0.25, 0.95F);
    const hover::math::Vec3 full_speed =
        hover::render::sample_full_speed_vibration(0.25, 1.0F);
    const hover::math::Vec3 boosted =
        hover::render::sample_full_speed_vibration(0.25, 1.28F);

    check(below_onset.x == 0.0F && below_onset.y == 0.0F && below_onset.z == 0.0F,
          "ship vibration remains off below the full-speed threshold");
    check(std::abs(full_speed.x) <= 0.012F && std::abs(full_speed.y) <= 0.008F &&
              std::abs(full_speed.z) <= 0.003F,
          "full-speed vibration remains a subtle local-space offset");
    check(std::abs(boosted.x - full_speed.x) <= 0.0001F &&
              std::abs(boosted.y - full_speed.y) <= 0.0001F &&
              std::abs(boosted.z - full_speed.z) <= 0.0001F,
          "boosted speed saturates rather than amplifying the subtle vibration");
}

} // namespace

int main() {
    test_visibility_tracks_positive_propulsion();
    test_release_envelope_shrinks_before_switching_off();
    test_speed_intensity_and_time_shape_the_pulse();
    test_boost_has_a_distinct_large_flare_response();
    test_invalid_presentation_input_fails_closed();
    test_full_speed_vibration_is_subtle_and_speed_gated();

    if (failure_count != 0) {
        std::cerr << failure_count << " engine-pulse test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All engine-pulse tests passed\n";
    return EXIT_SUCCESS;
}
