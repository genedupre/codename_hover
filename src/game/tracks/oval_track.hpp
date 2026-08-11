#pragma once

#include "game/track.hpp"

#include <cstdint>

namespace hover::game::tracks {

struct OvalTrackDefinition {
    float straight_length_metres;
    float turn_radius_metres;
    float half_width_metres;
    float elevation_metres;
};

struct OvalTrackBuild {
    OvalTrackDefinition definition;
    std::uint32_t sample_count;
};

[[nodiscard]] bool is_valid(const OvalTrackDefinition& definition);
[[nodiscard]] float oval_track_length(const OvalTrackDefinition& definition);
[[nodiscard]] SampledTrack make_sampled_oval(OvalTrackBuild build);

} // namespace hover::game::tracks
