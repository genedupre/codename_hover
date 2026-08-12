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

void advance_boost_camera_feedback(BoostCameraFeedbackState& state, double elapsed_seconds,
                                   bool boosting, float speed_ratio) {
    if (!std::isfinite(state.intensity) || !std::isfinite(elapsed_seconds) ||
        !std::isfinite(speed_ratio) || elapsed_seconds < 0.0) {
        state = {};
        return;
    }

    constexpr float activation_speed_ratio = 0.65F;
    if (!boosting) {
        state.activated_for_current_boost = false;
    } else if (speed_ratio >= activation_speed_ratio) {
        state.activated_for_current_boost = true;
    }

    const float target = state.activated_for_current_boost ? 1.0F : 0.0F;
    const float rate_per_second = target > state.intensity ? 10.0F : 3.0F;
    const float maximum_change = static_cast<float>(elapsed_seconds) * rate_per_second;
    if (target > state.intensity) {
        state.intensity = std::min(target, state.intensity + maximum_change);
    } else {
        state.intensity = std::max(target, state.intensity - maximum_change);
    }
    state.intensity = std::clamp(state.intensity, 0.0F, 1.0F);
}

BoostCameraSample sample_boost_camera(float intensity) {
    constexpr float base_follow_distance_metres = 8.5F;
    constexpr float base_look_ahead_metres = 3.0F;
    constexpr float base_vertical_field_of_view_radians = 1.0471975512F;
    constexpr float maximum_follow_distance_increase_metres = 0.6F;
    constexpr float maximum_look_ahead_increase_metres = 0.5F;
    constexpr float maximum_field_of_view_increase_radians = 0.1396263402F;

    if (!std::isfinite(intensity)) {
        intensity = 0.0F;
    }
    const float clamped_intensity = std::clamp(intensity, 0.0F, 1.0F);
    const float shaped_intensity =
        clamped_intensity * clamped_intensity * (3.0F - 2.0F * clamped_intensity);
    return BoostCameraSample{
        base_follow_distance_metres + maximum_follow_distance_increase_metres * shaped_intensity,
        base_look_ahead_metres + maximum_look_ahead_increase_metres * shaped_intensity,
        base_vertical_field_of_view_radians +
            maximum_field_of_view_increase_radians * shaped_intensity,
    };
}

} // namespace hover::render
