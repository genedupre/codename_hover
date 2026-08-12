#include "game/ships/prototype_01.hpp"

namespace hover::game::ships {

const ShipDefinition& prototype_01_definition() {
    static constexpr ShipDefinition definition{
        .id = "prototype_01",
        .display_name = "Prototype 01",
        .visual_mesh_id = prototype_01_mesh_id,
        .handling =
            {
                .base_maximum_forward_speed_metres_per_second = 260.0F,
                .forward_acceleration_metres_per_second_squared = 78.0F,
                .braking_deceleration_metres_per_second_squared = 180.0F,
                .coasting_deceleration_metres_per_second_squared = 90.0F,
                .steering_rate_radians_per_second = 1.90F,
                .maximum_lateral_speed_metres_per_second = 42.0F,
                .normal_lateral_grip_per_second = 7.0F,
                .drift_lateral_grip_per_second = 2.4F,
                .world_lateral_grip_deceleration_metres_per_second_squared = 300.0F,
                .world_drift_grip_deceleration_metres_per_second_squared = 55.0F,
                .world_drift_lateral_acceleration_metres_per_second_squared = 105.0F,
                .world_drift_force_fade_lateral_speed_metres_per_second = 32.0F,
                .world_drift_steering_multiplier = 1.15F,
                .world_steering_propulsion_loss_fraction = 0.35F,
                .world_drift_propulsion_loss_fraction = 0.85F,
                .world_forward_damping_per_second = 0.240481F,
                .world_lateral_damping_per_second = 0.180271F,
                .world_normal_damping_per_second = 3.71252F,
                .world_propulsion_curve_knee_speed_fraction = 0.45F,
                .world_propulsion_high_speed_multiplier = 0.81F,
                .world_propulsion_response_rate_at_rest_per_second = 12.0F,
                .world_propulsion_response_rate_at_base_speed_per_second = 60.0F,
                .world_slip_speed_threshold_metres_per_second = 8.0F,
                .world_sustained_slip_buildup_seconds = 1.6666667F,
                .world_sustained_slip_release_seconds = 0.75F,
                .world_sustained_slip_full_propulsion_multiplier = 0.50F,
                .world_boost_excess_speed_decay_metres_per_second_squared = 90.0F,
                .track_ride_height_metres = 0.62F,
                .boost_maximum_speed_multiplier = 1.28F,
                .boost_acceleration_metres_per_second_squared = 145.0F,
                .boost_excess_speed_decay_metres_per_second_squared = 170.0F,
                .boost_duration_seconds = 1.0F,
                .boost_throttle_release_tail_seconds = 0.20F,
            },
        .presentation =
            {
                .maximum_turn_roll_radians = 0.18F,
                .turn_roll_response_per_second = 8.0F,
            },
        .collision =
            {
                .local_bounds =
                    {
                        .center = {0.0F, 0.10F, 0.21F},
                        .half_extents = {2.0F, 0.58F, 2.72F},
                    },
                .relative_mass = 1.0F,
                .maximum_energy = 100.0F,
                .collision_damage_multiplier = 1.0F,
            },
    };
    return definition;
}

} // namespace hover::game::ships
