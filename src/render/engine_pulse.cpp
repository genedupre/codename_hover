#include "render/engine_pulse.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace hover::render {

EnginePulseSample sample_engine_pulse(double elapsed_seconds, float propulsion_intensity) {
    if (!std::isfinite(elapsed_seconds) || !std::isfinite(propulsion_intensity)) {
        return EnginePulseSample{false, 0.0F};
    }
    const float intensity = std::clamp(propulsion_intensity, 0.0F, 1.0F);
    if (intensity <= 0.001F) {
        return EnginePulseSample{false, 0.0F};
    }

    const double first_phase = elapsed_seconds * 8.0 * 2.0 * std::numbers::pi;
    const double second_phase = elapsed_seconds * 13.0 * 2.0 * std::numbers::pi + 0.7;
    const float pulse = 1.0F + 0.11F * static_cast<float>(std::sin(first_phase)) +
                        0.045F * static_cast<float>(std::sin(second_phase));
    const float base_scale = 0.34F + 0.72F * std::sqrt(intensity);
    return EnginePulseSample{true, base_scale * pulse};
}

} // namespace hover::render
