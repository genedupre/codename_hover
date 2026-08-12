#include "render/engine_pulse.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace hover::render {

float advance_engine_pulse_intensity(float current_intensity, float requested_intensity,
                                     double elapsed_seconds) {
    if (!std::isfinite(current_intensity) || !std::isfinite(requested_intensity) ||
        !std::isfinite(elapsed_seconds) || elapsed_seconds < 0.0) {
        return 0.0F;
    }

    const float current = std::clamp(current_intensity, 0.0F, 1.0F);
    const float requested = std::clamp(requested_intensity, 0.0F, 1.0F);
    if (requested >= current) {
        return requested;
    }

    constexpr float release_per_second = 5.0F;
    const float release = static_cast<float>(elapsed_seconds) * release_per_second;
    return std::max(requested, current - release);
}

EnginePulseSample sample_engine_pulse(double elapsed_seconds, float propulsion_intensity,
                                      float speed_ratio) {
    if (!std::isfinite(elapsed_seconds) || !std::isfinite(propulsion_intensity) ||
        !std::isfinite(speed_ratio)) {
        return EnginePulseSample{false, 0.0F, 0.0F};
    }
    const float intensity = std::clamp(propulsion_intensity, 0.0F, 1.0F);
    if (intensity <= 0.002F) {
        return EnginePulseSample{false, 0.0F, 0.0F};
    }

    const float speed = std::clamp(speed_ratio, 0.0F, 1.0F);
    const float shaped_speed = std::pow(speed, 1.35F);
    const double primary_frequency_hz = 7.0 + static_cast<double>(speed) * 9.0;
    const double secondary_frequency_hz = 11.0 + static_cast<double>(speed) * 13.0;
    const double first_phase = elapsed_seconds * primary_frequency_hz * 2.0 * std::numbers::pi;
    const double second_phase =
        elapsed_seconds * secondary_frequency_hz * 2.0 * std::numbers::pi + 0.7;
    const float pulse = 1.0F + 0.11F * static_cast<float>(std::sin(first_phase)) +
                        0.045F * static_cast<float>(std::sin(second_phase));
    const float release_scale = std::sqrt(intensity);
    const float radial_scale = (0.78F + 0.52F * shaped_speed) * release_scale * pulse;
    const float length_scale = (0.40F + 2.10F * shaped_speed) * release_scale * pulse;
    return EnginePulseSample{true, radial_scale, length_scale};
}

} // namespace hover::render
