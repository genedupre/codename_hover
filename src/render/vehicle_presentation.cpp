#include "render/vehicle_presentation.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace hover::render {

math::Vec3 sample_full_speed_vibration(double elapsed_seconds, float speed_ratio) {
    if (!std::isfinite(elapsed_seconds) || !std::isfinite(speed_ratio)) {
        return math::Vec3{0.0F, 0.0F, 0.0F};
    }

    constexpr float onset_speed_ratio = 0.97F;
    const float linear_intensity =
        std::clamp((speed_ratio - onset_speed_ratio) / (1.0F - onset_speed_ratio), 0.0F, 1.0F);
    const float intensity = linear_intensity * linear_intensity * (3.0F - 2.0F * linear_intensity);
    const double first_phase = elapsed_seconds * 17.0 * 2.0 * std::numbers::pi;
    const double second_phase = elapsed_seconds * 23.0 * 2.0 * std::numbers::pi + 0.9;

    return math::Vec3{
        intensity * 0.012F * static_cast<float>(std::sin(first_phase)),
        intensity * 0.008F * static_cast<float>(std::sin(second_phase)),
        intensity * 0.003F * static_cast<float>(std::sin(first_phase + second_phase)),
    };
}

} // namespace hover::render
