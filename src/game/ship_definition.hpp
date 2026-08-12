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
    float normal_lateral_grip_per_second;
    float drift_lateral_grip_per_second;
    float boost_maximum_speed_multiplier;
    float boost_acceleration_metres_per_second_squared;
    float boost_excess_speed_decay_metres_per_second_squared;
    float boost_duration_seconds;
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
