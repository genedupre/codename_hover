#include "game/tracks/speedway_track.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

namespace hover::game::tracks {
namespace {

float distance_into_turn(const SpeedwayTrackDefinition& definition, float distance_metres) {
    const float straight = definition.oval.straight_length_metres;
    const float turn_length = std::numbers::pi_v<float> * definition.oval.turn_radius_metres;

    if (distance_metres >= straight && distance_metres < straight + turn_length) {
        return distance_metres - straight;
    }
    const float second_turn_start = 2.0F * straight + turn_length;
    if (distance_metres >= second_turn_start) {
        return distance_metres - second_turn_start;
    }
    return -1.0F;
}

TrackSegmentProperties properties_at(const SpeedwayTrackDefinition& definition,
                                     float distance_metres) {
    const float straight = definition.oval.straight_length_metres;
    const float turn_length = std::numbers::pi_v<float> * definition.oval.turn_radius_metres;
    if (distance_metres >= straight && distance_metres < straight + turn_length) {
        return definition.first_turn_properties;
    }
    const float second_turn_start = 2.0F * straight + turn_length;
    if (distance_metres >= second_turn_start) {
        return definition.second_turn_properties;
    }
    return definition.straight_properties;
}

float bank_angle(const SpeedwayTrackDefinition& definition, float distance_metres) {
    const float turn_distance = distance_into_turn(definition, distance_metres);
    if (turn_distance < 0.0F) {
        return 0.0F;
    }

    const float turn_length = std::numbers::pi_v<float> * definition.oval.turn_radius_metres;
    const float distance_from_turn_edge = std::min(turn_distance, turn_length - turn_distance);
    const float linear_blend =
        std::clamp(distance_from_turn_edge / definition.bank_transition_metres, 0.0F, 1.0F);
    const float smooth_blend = linear_blend * linear_blend * (3.0F - 2.0F * linear_blend);
    return definition.maximum_bank_radians * smooth_blend;
}

void apply_bank(TrackFrame& frame, float radians) {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    const math::Vec3 flat_normal = frame.normal;
    const math::Vec3 flat_binormal = frame.binormal;

    // Both stadium turns bend left. Positive banking lowers the inside edge and
    // raises track-right, while preserving normal x tangent = binormal.
    frame.normal = flat_normal * cosine - flat_binormal * sine;
    frame.binormal = flat_binormal * cosine + flat_normal * sine;
}

} // namespace

bool is_valid(const SpeedwayTrackDefinition& definition) {
    const float half_turn_length =
        std::numbers::pi_v<float> * definition.oval.turn_radius_metres * 0.5F;
    return is_valid(definition.oval) && std::isfinite(definition.maximum_bank_radians) &&
           definition.maximum_bank_radians > 0.0F &&
           definition.maximum_bank_radians < std::numbers::pi_v<float> * 0.5F &&
           std::isfinite(definition.bank_transition_metres) &&
           definition.bank_transition_metres > 0.0F &&
           definition.bank_transition_metres <= half_turn_length;
}

SampledTrack make_sampled_speedway(SpeedwayTrackBuild build) {
    assert(is_valid(build.definition));
    SampledTrack track =
        make_sampled_oval(OvalTrackBuild{build.definition.oval, build.sample_count});
    for (std::size_t index = 0; index < track.frames.size(); ++index) {
        TrackFrame& frame = track.frames[index];
        apply_bank(frame, bank_angle(build.definition, frame.distance_metres));

        const TrackFrame& next = track.frames[(index + 1U) % track.frames.size()];
        const float segment_end =
            index + 1U < track.frames.size() ? next.distance_metres : track.length_metres;
        const float midpoint = frame.distance_metres + (segment_end - frame.distance_metres) * 0.5F;
        track.segment_properties[index] = properties_at(build.definition, midpoint);
    }
    assert(hover::game::is_valid(track));
    return track;
}

} // namespace hover::game::tracks
