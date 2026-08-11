#pragma once

#include "hover_math.hpp"

#include <vector>

namespace hover::game {

struct TrackFrame {
    float distance_metres;
    math::Vec3 center;
    math::Vec3 tangent;
    math::Vec3 normal;
    math::Vec3 binormal;
    float half_width_metres;
};

struct SampledTrack {
    float length_metres;
    std::vector<TrackFrame> frames;
};

struct TrackOffset {
    float lateral_metres;
    float height_metres;
};

[[nodiscard]] bool is_valid(const TrackFrame& frame);
[[nodiscard]] bool is_valid(const SampledTrack& track);
[[nodiscard]] float wrap_track_distance(const SampledTrack& track, float distance_metres);
[[nodiscard]] TrackFrame sample_track(const SampledTrack& track, float distance_metres);
[[nodiscard]] math::Vec3 point_on_track_frame(const TrackFrame& frame, TrackOffset offset);

} // namespace hover::game
