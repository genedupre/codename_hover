#pragma once

#include "game/track.hpp"
#include "game/track_vehicle_simulation.hpp"

namespace hover::game {

struct PhysicalVehicleBasis {
    math::Vec3 forward{0.0F, 0.0F, 1.0F};
    math::Vec3 up{0.0F, 1.0F, 0.0F};
};

struct PhysicalVehicleState {
    math::Vec3 position{0.0F, 0.0F, 0.0F};
    math::Vec3 velocity{0.0F, 0.0F, 0.0F};
    PhysicalVehicleBasis basis;
};

struct ProjectedCourseReference {
    TrackLocation location;
    float height_above_surface_metres = 0.0F;
    TrackFrame frame{};
};

struct WorldTrackVehicleState {
    PhysicalVehicleState physical;
    ProjectedCourseReference course;
    VehicleState vehicle;
};

struct WorldTrackVehicleSpawn {
    float distance_along_path_metres = 0.0F;
    float lateral_offset_metres = 0.0F;
};

struct WorldTrackVehicleTick {
    input::PlayerInput input;
    const ShipDefinition& definition;
    ResolvedTrackPath path;
    float tick_seconds;
};

[[nodiscard]] bool is_valid(const PhysicalVehicleBasis& basis);
[[nodiscard]] bool is_valid(const PhysicalVehicleState& state);
[[nodiscard]] bool is_valid(const ProjectedCourseReference& reference);
[[nodiscard]] bool is_valid(const WorldTrackVehicleState& state);

[[nodiscard]] WorldTrackVehicleState
make_world_track_vehicle_state(WorldTrackVehicleSpawn spawn, const ShipDefinition& definition,
                               ResolvedTrackPath path);
VehicleTickEvents simulate_world_track_vehicle(WorldTrackVehicleState& state,
                                               const WorldTrackVehicleTick& tick);

} // namespace hover::game
