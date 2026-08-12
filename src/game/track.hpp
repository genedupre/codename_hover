#pragma once

#include "hover_math.hpp"

#include <cstddef>
#include <limits>
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

enum class TrackEdgePolicy {
    solid_wall,
    open,
};

struct TrackSegmentProperties {
    TrackEdgePolicy left_edge = TrackEdgePolicy::solid_wall;
    TrackEdgePolicy right_edge = TrackEdgePolicy::solid_wall;
};

struct SampledTrack {
    float length_metres;
    std::vector<TrackFrame> frames;
    // One entry for the chord beginning at the frame with the same index. The
    // final entry describes the closing chord back to frame zero.
    std::vector<TrackSegmentProperties> segment_properties;
};

struct TrackOffset {
    float lateral_metres;
    float height_metres;
};

// The closest point on one path inside a bounded along-path search window.
// The hint keeps crossings, stacked track, and future route branches from
// silently selecting an unrelated piece of otherwise nearby geometry.
struct TrackProjection {
    TrackFrame frame;
    TrackOffset offset;
    std::size_t segment_index;
    float signed_distance_from_hint_metres;
    float centerline_distance_metres;
};

[[nodiscard]] bool is_valid(const TrackFrame& frame);
[[nodiscard]] bool is_valid(const SampledTrack& track);
[[nodiscard]] float wrap_track_distance(const SampledTrack& track, float distance_metres);
[[nodiscard]] TrackFrame sample_track(const SampledTrack& track, float distance_metres);
[[nodiscard]] math::Vec3 point_on_track_frame(const TrackFrame& frame, TrackOffset offset);
[[nodiscard]] TrackProjection
project_point_onto_track(const SampledTrack& track, math::Vec3 world_point,
                         float hint_distance_metres,
                         float search_radius_metres = std::numeric_limits<float>::infinity());

} // namespace hover::game
