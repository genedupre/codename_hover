#include "game/ship_definition.hpp"

namespace hover::game {

bool is_valid(const ShipDefinition& definition) {
    const HandlingProfile& handling = definition.handling;
    const CollisionProfile& collision = definition.collision;
    const math::Vec3 half_extents = collision.local_bounds.half_extents;

    return !definition.id.empty() && !definition.display_name.empty() &&
           !definition.visual_mesh_id.empty() &&
           handling.maximum_forward_speed_metres_per_second > 0.0F &&
           handling.forward_acceleration_metres_per_second_squared > 0.0F &&
           handling.braking_deceleration_metres_per_second_squared > 0.0F &&
           handling.coasting_deceleration_metres_per_second_squared >= 0.0F &&
           handling.steering_rate_radians_per_second > 0.0F &&
           handling.normal_lateral_grip_per_second >= 0.0F &&
           handling.drift_lateral_grip_per_second >= 0.0F && half_extents.x > 0.0F &&
           half_extents.y > 0.0F && half_extents.z > 0.0F && collision.relative_mass > 0.0F &&
           collision.maximum_energy > 0.0F && collision.collision_damage_multiplier > 0.0F;
}

} // namespace hover::game
