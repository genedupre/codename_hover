#pragma once

#include "hover_math.hpp"

namespace hover::render {

struct BoostCameraFeedbackState {
    float intensity = 0.0F;
    bool activated_for_current_boost = false;
};

struct BoostCameraSample {
    float follow_distance_metres;
    float look_ahead_metres;
    float vertical_field_of_view_radians;
};

// Cosmetic motion only. The offset is applied in ship-local space after its
// interpolated yaw and turn roll, so it composes with steering presentation and
// never changes simulation or camera state.
[[nodiscard]] math::Vec3 sample_full_speed_vibration(double elapsed_seconds, float speed_ratio);

// Camera feedback is presentation-only. An active boost must reach a minimum
// speed before it latches on; after the boost ends, intensity releases smoothly
// instead of snapping back while excess vehicle speed is still decaying.
void advance_boost_camera_feedback(BoostCameraFeedbackState& state, double elapsed_seconds,
                                   bool boosting, float speed_ratio);

[[nodiscard]] BoostCameraSample sample_boost_camera(float intensity);

} // namespace hover::render
