#include "game/tracks/oval_track.hpp"

#include <cassert>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

namespace hover::game::tracks {
namespace {

TrackFrame sample_oval_centerline(const OvalTrackDefinition& definition, float distance_metres) {
    const float straight = definition.straight_length_metres;
    const float half_straight = straight * 0.5F;
    const float radius = definition.turn_radius_metres;
    const float turn_length = std::numbers::pi_v<float> * radius;
    const float height = definition.elevation_metres;
    const float width = definition.half_width_metres;

    if (distance_metres < straight) {
        return TrackFrame{distance_metres,    {radius, height, -half_straight + distance_metres},
                          {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F},
                          {1.0F, 0.0F, 0.0F}, width};
    }

    if (distance_metres < straight + turn_length) {
        const float angle = (distance_metres - straight) / radius;
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);
        return TrackFrame{
            distance_metres,       {radius * cosine, height, half_straight + radius * sine},
            {-sine, 0.0F, cosine}, {0.0F, 1.0F, 0.0F},
            {cosine, 0.0F, sine},  width};
    }

    if (distance_metres < 2.0F * straight + turn_length) {
        const float distance_on_straight = distance_metres - straight - turn_length;
        return TrackFrame{
            distance_metres,     {-radius, height, half_straight - distance_on_straight},
            {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F},
            {-1.0F, 0.0F, 0.0F}, width};
    }

    const float angle =
        std::numbers::pi_v<float> + (distance_metres - 2.0F * straight - turn_length) / radius;
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    return TrackFrame{
        distance_metres,       {radius * cosine, height, -half_straight + radius * sine},
        {-sine, 0.0F, cosine}, {0.0F, 1.0F, 0.0F},
        {cosine, 0.0F, sine},  width};
}

} // namespace

bool is_valid(const OvalTrackDefinition& definition) {
    return std::isfinite(definition.straight_length_metres) &&
           definition.straight_length_metres > 0.0F &&
           std::isfinite(definition.turn_radius_metres) &&
           definition.turn_radius_metres > definition.half_width_metres &&
           std::isfinite(definition.half_width_metres) && definition.half_width_metres > 0.0F &&
           std::isfinite(definition.elevation_metres);
}

float oval_track_length(const OvalTrackDefinition& definition) {
    assert(is_valid(definition));
    return 2.0F * definition.straight_length_metres +
           2.0F * std::numbers::pi_v<float> * definition.turn_radius_metres;
}

SampledTrack make_sampled_oval(OvalTrackBuild build) {
    assert(is_valid(build.definition));
    assert(build.sample_count >= 8U);

    const float length = oval_track_length(build.definition);
    std::vector<TrackFrame> frames;
    frames.reserve(build.sample_count);
    for (std::uint32_t index = 0; index < build.sample_count; ++index) {
        const float distance =
            length * static_cast<float>(index) / static_cast<float>(build.sample_count);
        frames.push_back(sample_oval_centerline(build.definition, distance));
    }

    SampledTrack result{length, std::move(frames)};
    assert(hover::game::is_valid(result));
    return result;
}

} // namespace hover::game::tracks
