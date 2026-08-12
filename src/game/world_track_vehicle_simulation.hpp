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

struct HandlingRuntimeState {
    float sustained_slip_seconds = 0.0F;
    float sustained_slip_intensity = 0.0F;
    float applied_propulsion_acceleration_metres_per_second_squared = 0.0F;
};

struct WorldTrackVehicleState {
    PhysicalVehicleState physical;
    ProjectedCourseReference course;
    HandlingRuntimeState handling;
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

// Observational values from one completed fixed tick. These values explain the
// authoritative simulation result but never feed back into it.
struct WorldTrackVehicleTelemetry {
    float world_speed_metres_per_second = 0.0F;
    float local_forward_speed_metres_per_second = 0.0F;
    float local_lateral_speed_metres_per_second = 0.0F;
    float local_normal_speed_metres_per_second = 0.0F;
    float signed_slip_angle_radians = 0.0F;
    float steering_direction_change_radians = 0.0F;
    float steering_direction_change_ratio = 0.0F;
    float drift_direction = 0.0F;
    float drift_force_fraction = 0.0F;
    float selected_grip_deceleration_metres_per_second_squared = 0.0F;
    float forward_damping_deceleration_metres_per_second_squared = 0.0F;
    float lateral_damping_deceleration_metres_per_second_squared = 0.0F;
    float normal_damping_deceleration_metres_per_second_squared = 0.0F;
    float propulsion_curve_multiplier = 1.0F;
    float sustained_slip_seconds = 0.0F;
    float sustained_slip_intensity = 0.0F;
    float requested_propulsion_acceleration_metres_per_second_squared = 0.0F;
    float applied_propulsion_acceleration_metres_per_second_squared = 0.0F;
    float propulsion_fraction = 1.0F;
    float post_boost_return_deceleration_metres_per_second_squared = 0.0F;
    bool edge_constraint_activated = false;
};

struct WorldTrackVehicleTickResult {
    VehicleTickEvents events;
    WorldTrackVehicleTelemetry telemetry;
};

[[nodiscard]] bool is_valid(const PhysicalVehicleBasis& basis);
[[nodiscard]] bool is_valid(const PhysicalVehicleState& state);
[[nodiscard]] bool is_valid(const ProjectedCourseReference& reference);
[[nodiscard]] bool is_valid(const HandlingRuntimeState& state);
[[nodiscard]] bool is_valid(const WorldTrackVehicleState& state);
[[nodiscard]] bool is_valid(const WorldTrackVehicleTelemetry& telemetry);

[[nodiscard]] WorldTrackVehicleState
make_world_track_vehicle_state(WorldTrackVehicleSpawn spawn, const ShipDefinition& definition,
                               ResolvedTrackPath path);
WorldTrackVehicleTickResult simulate_world_track_vehicle(WorldTrackVehicleState& state,
                                                         const WorldTrackVehicleTick& tick);

} // namespace hover::game
