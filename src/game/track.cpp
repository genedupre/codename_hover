#include "game/track.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>

namespace hover::game {
namespace {

constexpr float frame_tolerance = 0.002F;

bool is_finite(math::Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool is_unit(math::Vec3 value) {
    return std::abs(math::dot(value, value) - 1.0F) <= frame_tolerance;
}

bool frames_can_interpolate(const TrackFrame& start, const TrackFrame& end) {
    return math::dot(start.tangent, end.tangent) > 0.0F &&
           math::dot(start.normal, end.normal) > 0.0F &&
           math::dot(start.binormal, end.binormal) > 0.0F;
}

math::Vec3 interpolate(math::Vec3 start, math::Vec3 end, float alpha) {
    return start * (1.0F - alpha) + end * alpha;
}

TrackFrame interpolate_frame(const TrackFrame& start, const TrackFrame& end, float alpha,
                             float distance_metres) {
    const math::Vec3 tangent = math::normalized(interpolate(start.tangent, end.tangent, alpha));
    const math::Vec3 blended_normal = interpolate(start.normal, end.normal, alpha);
    const math::Vec3 orthogonal_normal =
        blended_normal - tangent * math::dot(blended_normal, tangent);
    const math::Vec3 normal = math::normalized(orthogonal_normal);
    const math::Vec3 binormal = math::normalized(math::cross(normal, tangent));

    return TrackFrame{
        distance_metres, interpolate(start.center, end.center, alpha),
        tangent,         normal,
        binormal,        start.half_width_metres * (1.0F - alpha) + end.half_width_metres * alpha,
    };
}

} // namespace

bool is_valid(const TrackFrame& frame) {
    if (!std::isfinite(frame.distance_metres) || frame.distance_metres < 0.0F ||
        !std::isfinite(frame.half_width_metres) || frame.half_width_metres <= 0.0F ||
        !is_finite(frame.center) || !is_finite(frame.tangent) || !is_finite(frame.normal) ||
        !is_finite(frame.binormal)) {
        return false;
    }

    return is_unit(frame.tangent) && is_unit(frame.normal) && is_unit(frame.binormal) &&
           std::abs(math::dot(frame.tangent, frame.normal)) <= frame_tolerance &&
           std::abs(math::dot(frame.tangent, frame.binormal)) <= frame_tolerance &&
           std::abs(math::dot(frame.normal, frame.binormal)) <= frame_tolerance &&
           math::dot(math::cross(frame.normal, frame.tangent), frame.binormal) >=
               1.0F - frame_tolerance;
}

bool is_valid(const SampledTrack& track) {
    if (!std::isfinite(track.length_metres) || track.length_metres <= 0.0F ||
        track.frames.size() < 2 || track.frames.front().distance_metres != 0.0F) {
        return false;
    }

    const TrackFrame* previous_frame = nullptr;
    float previous_distance = -1.0F;
    for (const TrackFrame& frame : track.frames) {
        if (!is_valid(frame) || frame.distance_metres <= previous_distance ||
            frame.distance_metres >= track.length_metres ||
            (previous_frame != nullptr && !frames_can_interpolate(*previous_frame, frame))) {
            return false;
        }
        previous_frame = &frame;
        previous_distance = frame.distance_metres;
    }
    return frames_can_interpolate(track.frames.back(), track.frames.front());
}

float wrap_track_distance(const SampledTrack& track, float distance_metres) {
    assert(track.length_metres > 0.0F);
    assert(std::isfinite(distance_metres));
    float wrapped = std::fmod(distance_metres, track.length_metres);
    if (wrapped < 0.0F) {
        wrapped += track.length_metres;
    }
    return wrapped;
}

TrackFrame sample_track(const SampledTrack& track, float distance_metres) {
    assert(track.length_metres > 0.0F);
    assert(track.frames.size() >= 2);
    assert(track.frames.front().distance_metres == 0.0F);
    assert(std::isfinite(distance_metres));
    const float wrapped_distance = wrap_track_distance(track, distance_metres);
    const auto upper =
        std::ranges::upper_bound(track.frames, wrapped_distance, {}, &TrackFrame::distance_metres);

    if (upper == track.frames.end()) {
        const TrackFrame& start = track.frames.back();
        const TrackFrame& end = track.frames.front();
        const float interval = track.length_metres - start.distance_metres;
        const float alpha = (wrapped_distance - start.distance_metres) / interval;
        return interpolate_frame(start, end, alpha, wrapped_distance);
    }

    if (upper == track.frames.begin()) {
        return track.frames.front();
    }

    const TrackFrame& start = *std::prev(upper);
    const TrackFrame& end = *upper;
    const float alpha =
        (wrapped_distance - start.distance_metres) / (end.distance_metres - start.distance_metres);
    return interpolate_frame(start, end, alpha, wrapped_distance);
}

math::Vec3 point_on_track_frame(const TrackFrame& frame, TrackOffset offset) {
    return frame.center + frame.binormal * offset.lateral_metres +
           frame.normal * offset.height_metres;
}

} // namespace hover::game
