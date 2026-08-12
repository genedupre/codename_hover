#pragma once

#include "hover_math.hpp"

namespace hover::render {

// Cosmetic motion only. The offset is applied in ship-local space after its
// interpolated yaw and turn roll, so it composes with steering presentation and
// never changes simulation or camera state.
[[nodiscard]] math::Vec3 sample_full_speed_vibration(double elapsed_seconds, float speed_ratio);

} // namespace hover::render
