#pragma once

#include "game/tracks/oval_track.hpp"

#include <cstdint>

namespace hover::game::tracks {

struct SpeedwayTrackDefinition {
    OvalTrackDefinition oval;
    float maximum_bank_radians;
    float bank_transition_metres;
};

struct SpeedwayTrackBuild {
    SpeedwayTrackDefinition definition;
    std::uint32_t sample_count;
};

[[nodiscard]] bool is_valid(const SpeedwayTrackDefinition& definition);
[[nodiscard]] SampledTrack make_sampled_speedway(SpeedwayTrackBuild build);

} // namespace hover::game::tracks
