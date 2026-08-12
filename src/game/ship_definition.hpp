#pragma once

#include "hover_math.hpp"

#include <string_view>

namespace hover::game {

struct LocalBoxCollider {
    math::Vec3 center;
    math::Vec3 half_extents;
};

struct HandlingProfile {
    float base_maximum_forward_speed_metres_per_second;
    float forward_acceleration_metres_per_second_squared;
    float braking_deceleration_metres_per_second_squared;
    float coasting_deceleration_metres_per_second_squared;
    float steering_rate_radians_per_second;
    float maximum_lateral_speed_metres_per_second;
    float normal_lateral_grip_per_second;
    float drift_lateral_grip_per_second;
    float world_lateral_grip_deceleration_metres_per_second_squared;
    float world_drift_grip_deceleration_metres_per_second_squared;
    float world_drift_lateral_acceleration_metres_per_second_squared;
    float world_drift_force_fade_lateral_speed_metres_per_second;
    float world_drift_steering_multiplier;
    float world_steering_propulsion_loss_fraction;
    float world_drift_propulsion_loss_fraction;
    float world_drift_forward_deceleration_metres_per_second_squared;
    float world_slip_speed_threshold_metres_per_second;
    float world_slip_forward_deceleration_per_lateral_speed;
    float track_ride_height_metres;
    float boost_maximum_speed_multiplier;
    float boost_acceleration_metres_per_second_squared;
    float boost_excess_speed_decay_metres_per_second_squared;
    float boost_duration_seconds;
    float boost_throttle_release_tail_seconds;
};

struct CollisionProfile {
    LocalBoxCollider local_bounds;
    float relative_mass;
    float maximum_energy;
    float collision_damage_multiplier;
};

struct ShipPresentationProfile {
    float maximum_turn_roll_radians;
    float turn_roll_response_per_second;
};

struct ShipDefinition {
    std::string_view id;
    std::string_view display_name;
    std::string_view visual_mesh_id;
    HandlingProfile handling;
    ShipPresentationProfile presentation;
    CollisionProfile collision;
};

[[nodiscard]] bool is_valid(const ShipDefinition& definition);

} // namespace hover::game
