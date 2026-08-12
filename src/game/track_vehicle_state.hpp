#pragma once

#include <cstdint>

namespace hover::game {

// Identifies the currently followed path within one loaded course. Zero is
// reserved so default-constructed and unassigned locations fail validation.
struct TrackPathId {
    std::uint32_t value = 0;

    bool operator==(const TrackPathId&) const = default;
};

struct TrackLocation {
    TrackPathId path;
    float distance_along_path_metres = 0.0F;
    float lateral_offset_metres = 0.0F;
};

// State used while a ship is associated with a track surface. Normal values use
// the sampled frame's local normal, not world Y, so the representation also
// works while the path is banked, vertical, or inverted.
struct TrackVehicleState {
    TrackLocation location;
    float forward_speed_metres_per_second = 0.0F;
    float lateral_velocity_metres_per_second = 0.0F;
    float normal_offset_metres = 0.0F;
    float normal_velocity_metres_per_second = 0.0F;
};

[[nodiscard]] bool is_valid(TrackPathId path);
[[nodiscard]] bool is_valid(const TrackVehicleState& state);

} // namespace hover::game
