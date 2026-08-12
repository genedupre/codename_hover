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

float length(math::Vec3 value) { return std::sqrt(math::dot(value, value)); }

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

TrackProjection project_point_onto_track(const SampledTrack& track, math::Vec3 world_point,
                                         float hint_distance_metres, float search_radius_metres) {
    assert(is_valid(track));
    assert(is_finite(world_point));
    assert(std::isfinite(hint_distance_metres));
    assert(search_radius_metres >= 0.0F);

    const float hint = wrap_track_distance(track, hint_distance_metres);
    const float radius = std::isfinite(search_radius_metres)
                             ? std::min(search_radius_metres, track.length_metres * 0.5F)
                             : track.length_metres * 0.5F;
    const float window_start = hint - radius;
    const float window_end = hint + radius;

    float best_distance_squared = std::numeric_limits<float>::infinity();
    float best_hint_delta = 0.0F;
    float best_unwrapped_distance = hint;

    for (std::size_t index = 0; index < track.frames.size(); ++index) {
        const TrackFrame& start = track.frames[index];
        const TrackFrame& end = track.frames[(index + 1U) % track.frames.size()];
        const float segment_start = start.distance_metres;
        const float segment_end =
            index + 1U < track.frames.size() ? end.distance_metres : track.length_metres;
        const float segment_distance = segment_end - segment_start;
        const math::Vec3 chord = end.center - start.center;
        const float chord_length_squared = math::dot(chord, chord);

        for (const float loop_offset : {-track.length_metres, 0.0F, track.length_metres}) {
            const float unwrapped_start = segment_start + loop_offset;
            const float unwrapped_end = segment_end + loop_offset;
            const float allowed_start = std::max(unwrapped_start, window_start);
            const float allowed_end = std::min(unwrapped_end, window_end);
            if (allowed_start > allowed_end) {
                continue;
            }

            const float minimum_alpha = (allowed_start - unwrapped_start) / segment_distance;
            const float maximum_alpha = (allowed_end - unwrapped_start) / segment_distance;
            float alpha = minimum_alpha;
            if (chord_length_squared > 0.000001F) {
                alpha =
                    std::clamp(math::dot(world_point - start.center, chord) / chord_length_squared,
                               minimum_alpha, maximum_alpha);
            }

            const math::Vec3 candidate_center = start.center + chord * alpha;
            const math::Vec3 displacement = world_point - candidate_center;
            const float distance_squared = math::dot(displacement, displacement);
            const float unwrapped_distance = unwrapped_start + segment_distance * alpha;
            const float hint_delta = unwrapped_distance - hint;
            constexpr float tie_tolerance = 0.000001F;
            const bool closer = distance_squared < best_distance_squared - tie_tolerance;
            const bool nearer_hint =
                std::abs(distance_squared - best_distance_squared) <= tie_tolerance &&
                std::abs(hint_delta) < std::abs(best_hint_delta);
            if (closer || nearer_hint) {
                best_distance_squared = distance_squared;
                best_hint_delta = hint_delta;
                best_unwrapped_distance = unwrapped_distance;
            }
        }
    }

    assert(std::isfinite(best_distance_squared));
    const TrackFrame frame = sample_track(track, best_unwrapped_distance);
    const math::Vec3 displacement = world_point - frame.center;
    return TrackProjection{
        .frame = frame,
        .offset =
            TrackOffset{
                .lateral_metres = math::dot(displacement, frame.binormal),
                .height_metres = math::dot(displacement, frame.normal),
            },
        .signed_distance_from_hint_metres = best_hint_delta,
        .centerline_distance_metres = length(displacement),
    };
}

} // namespace hover::game
