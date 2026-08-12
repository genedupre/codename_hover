#include "game/ship_definition.hpp"

namespace hover::game {

bool is_valid(const ShipDefinition& definition) {
    const HandlingProfile& handling = definition.handling;
    const ShipPresentationProfile& presentation = definition.presentation;
    const CollisionProfile& collision = definition.collision;
    const math::Vec3 half_extents = collision.local_bounds.half_extents;

    return !definition.id.empty() && !definition.display_name.empty() &&
           !definition.visual_mesh_id.empty() &&
           handling.base_maximum_forward_speed_metres_per_second > 0.0F &&
           handling.forward_acceleration_metres_per_second_squared > 0.0F &&
           handling.braking_deceleration_metres_per_second_squared > 0.0F &&
           handling.coasting_deceleration_metres_per_second_squared >= 0.0F &&
           handling.steering_rate_radians_per_second > 0.0F &&
           handling.maximum_lateral_speed_metres_per_second > 0.0F &&
           handling.normal_lateral_grip_per_second >= 0.0F &&
           handling.drift_lateral_grip_per_second >= 0.0F &&
           handling.world_lateral_grip_deceleration_metres_per_second_squared >= 0.0F &&
           handling.world_drift_grip_deceleration_metres_per_second_squared >= 0.0F &&
           handling.world_traction_risk_start_speed_fraction >= 0.0F &&
           handling.world_traction_risk_start_speed_fraction < 1.0F &&
           handling.world_high_speed_grip_multiplier > 0.0F &&
           handling.world_high_speed_grip_multiplier <= 1.0F &&
           handling.world_sustained_slip_grip_multiplier > 0.0F &&
           handling.world_sustained_slip_grip_multiplier <= 1.0F &&
           handling.world_lift_off_grip_multiplier >= 1.0F &&
           handling.world_braking_grip_multiplier >= handling.world_lift_off_grip_multiplier &&
           handling.world_drift_lateral_acceleration_metres_per_second_squared >= 0.0F &&
           handling.world_drift_force_fade_lateral_speed_metres_per_second > 0.0F &&
           handling.world_drift_steering_multiplier > 0.0F &&
           handling.world_steering_propulsion_loss_fraction >= 0.0F &&
           handling.world_steering_propulsion_loss_fraction <= 1.0F &&
           handling.world_drift_propulsion_loss_fraction >= 0.0F &&
           handling.world_drift_propulsion_loss_fraction <= 1.0F &&
           handling.world_forward_damping_per_second >= 0.0F &&
           handling.world_lateral_damping_per_second >= 0.0F &&
           handling.world_normal_damping_per_second >= 0.0F &&
           handling.world_propulsion_curve_knee_speed_fraction >= 0.0F &&
           handling.world_propulsion_curve_knee_speed_fraction < 1.0F &&
           handling.world_propulsion_high_speed_multiplier > 0.0F &&
           handling.world_propulsion_high_speed_multiplier <= 1.0F &&
           handling.world_propulsion_response_rate_at_rest_per_second > 0.0F &&
           handling.world_propulsion_response_rate_at_base_speed_per_second >=
               handling.world_propulsion_response_rate_at_rest_per_second &&
           handling.world_slip_speed_threshold_metres_per_second >= 0.0F &&
           handling.world_sustained_slip_buildup_seconds > 0.0F &&
           handling.world_sustained_slip_release_seconds > 0.0F &&
           handling.world_sustained_slip_full_propulsion_multiplier >= 0.0F &&
           handling.world_sustained_slip_full_propulsion_multiplier <= 1.0F &&
           handling.world_boost_excess_speed_decay_metres_per_second_squared > 0.0F &&
           handling.track_ride_height_metres > 0.0F &&
           handling.world_gravity_acceleration_metres_per_second_squared > 0.0F &&
           handling.world_hover_spring_per_second_squared > 0.0F &&
           handling.world_hover_damping_per_second > 0.0F &&
           handling.world_maximum_hover_acceleration_metres_per_second_squared >=
               handling.world_gravity_acceleration_metres_per_second_squared &&
           handling.world_support_detach_height_metres >
               handling.world_support_landing_height_metres &&
           handling.world_support_landing_height_metres > 0.0F &&
           handling.world_takeoff_normal_speed_metres_per_second > 0.0F &&
           handling.world_supported_up_response_per_second > 0.0F &&
           handling.world_airborne_gravity_up_response_per_second > 0.0F &&
           handling.world_wall_restitution >= 0.0F && handling.world_wall_restitution < 1.0F &&
           handling.world_wall_tangent_retention > 0.0F &&
           handling.world_wall_tangent_retention <= 1.0F &&
           handling.world_recovery_delay_seconds > 0.0F &&
           handling.world_recovery_drop_metres > 0.0F &&
           handling.world_recovery_speed_fraction >= 0.0F &&
           handling.world_recovery_speed_fraction < 1.0F &&
           handling.world_recovery_safe_margin_metres >= 0.0F &&
           handling.boost_maximum_speed_multiplier > 1.0F &&
           handling.boost_acceleration_metres_per_second_squared > 0.0F &&
           handling.boost_excess_speed_decay_metres_per_second_squared > 0.0F &&
           handling.boost_duration_seconds > 0.0F &&
           handling.boost_throttle_release_tail_seconds > 0.0F &&
           handling.boost_throttle_release_tail_seconds <= handling.boost_duration_seconds &&
           presentation.maximum_turn_roll_radians >= 0.0F &&
           presentation.turn_roll_response_per_second > 0.0F && half_extents.x > 0.0F &&
           half_extents.y > 0.0F && half_extents.z > 0.0F && collision.relative_mass > 0.0F &&
           collision.maximum_energy > 0.0F && collision.collision_damage_multiplier > 0.0F;
}

} // namespace hover::game
