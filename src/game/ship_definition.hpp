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
    float world_traction_risk_start_speed_fraction;
    float world_high_speed_grip_multiplier;
    float world_sustained_slip_grip_multiplier;
    float world_lift_off_grip_multiplier;
    float world_braking_grip_multiplier;
    float world_drift_lateral_acceleration_metres_per_second_squared;
    float world_drift_force_fade_lateral_speed_metres_per_second;
    float world_drift_steering_multiplier;
    float world_steering_propulsion_loss_fraction;
    float world_drift_propulsion_loss_fraction;
    float world_forward_damping_per_second;
    float world_lateral_damping_per_second;
    float world_normal_damping_per_second;
    float world_propulsion_curve_knee_speed_fraction;
    float world_propulsion_high_speed_multiplier;
    float world_propulsion_response_rate_at_rest_per_second;
    float world_propulsion_response_rate_at_base_speed_per_second;
    float world_slip_speed_threshold_metres_per_second;
    float world_sustained_slip_buildup_seconds;
    float world_sustained_slip_release_seconds;
    float world_sustained_slip_full_propulsion_multiplier;
    float world_boost_excess_speed_decay_metres_per_second_squared;
    float track_ride_height_metres;
    float world_gravity_acceleration_metres_per_second_squared;
    float world_hover_spring_per_second_squared;
    float world_hover_damping_per_second;
    float world_maximum_hover_acceleration_metres_per_second_squared;
    float world_support_detach_height_metres;
    float world_support_landing_height_metres;
    float world_takeoff_normal_speed_metres_per_second;
    float world_supported_up_response_per_second;
    float world_airborne_gravity_up_response_per_second;
    float world_wall_restitution;
    float world_wall_tangent_retention;
    float world_recovery_delay_seconds;
    float world_recovery_drop_metres;
    float world_recovery_speed_fraction;
    float world_recovery_safe_margin_metres;
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
