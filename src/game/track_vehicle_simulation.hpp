#pragma once

#include "game/track.hpp"
#include "game/track_vehicle_state.hpp"

namespace hover::game {

// A course owns geometry and resolves a stable path ID before simulation. The
// vehicle stores only the ID, allowing a future course graph to switch paths at
// splits and joins without putting owning pointers in per-vehicle state.
struct ResolvedTrackPath {
    TrackPathId id;
    const SampledTrack& geometry;
};

struct TrackVehicleSpawn {
    float distance_along_path_metres = 0.0F;
    float lateral_offset_metres = 0.0F;
};

struct TrackVehicleTick {
    input::PlayerInput input;
    const ShipDefinition& definition;
    ResolvedTrackPath path;
    float tick_seconds;
};

[[nodiscard]] TrackVehicleState make_track_vehicle_state(TrackVehicleSpawn spawn,
                                                         const ShipDefinition& definition,
                                                         ResolvedTrackPath path);
VehicleTickEvents simulate_track_vehicle(TrackVehicleState& state, const TrackVehicleTick& tick);

} // namespace hover::game
