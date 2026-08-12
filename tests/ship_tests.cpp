#include "assets/generated/engine_pulse_mesh.hpp"
#include "assets/generated/prototype_01_mesh.hpp"
#include "game/ship_definition.hpp"
#include "game/ships/prototype_01.hpp"
#include "render/mesh_data.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

constexpr float tolerance = 0.0001F;
int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

void test_first_ship_definition() {
    const hover::game::ShipDefinition& ship = hover::game::ships::prototype_01_definition();

    check(hover::game::is_valid(ship), "Prototype 01 has a valid gameplay definition");
    check(ship.visual_mesh_id == hover::game::ships::prototype_01_mesh_id,
          "Prototype 01 references its generated visual mesh");
    check(ship.handling.normal_lateral_grip_per_second >
              ship.handling.drift_lateral_grip_per_second,
          "Prototype 01 loses lateral grip while drifting");
    check(ship.handling.braking_deceleration_metres_per_second_squared == 180.0F,
          "Prototype 01 has the more aggressive braking response");
    check(ship.handling.coasting_deceleration_metres_per_second_squared == 90.0F,
          "Prototype 01 has the more aggressive coasting slowdown");
    check(ship.handling.steering_rate_radians_per_second > 1.65F,
          "Prototype 01 has the faster provisional steering rate");
    check(ship.handling.maximum_lateral_speed_metres_per_second > 0.0F &&
              ship.handling.track_ride_height_metres > 0.0F,
          "Prototype 01 defines attached-surface movement dimensions explicitly");
    check(ship.handling.world_lateral_grip_deceleration_metres_per_second_squared >
                  ship.handling.world_drift_grip_deceleration_metres_per_second_squared &&
              ship.handling.world_drift_lateral_acceleration_metres_per_second_squared > 0.0F &&
              ship.handling.world_drift_force_fade_lateral_speed_metres_per_second > 0.0F &&
              ship.handling.world_drift_steering_multiplier > 1.0F &&
              ship.handling.world_steering_propulsion_loss_fraction > 0.0F &&
              ship.handling.world_drift_propulsion_loss_fraction > 0.0F &&
              ship.handling.world_drift_forward_deceleration_metres_per_second_squared > 0.0F,
          "Prototype 01 defines world-space grip and directional drift explicitly");
    check(ship.handling.world_lateral_grip_deceleration_metres_per_second_squared == 300.0F,
          "Prototype 01 retains less normal grip at maximum and boosted speeds");
    check(ship.handling.world_slip_speed_threshold_metres_per_second > 0.0F &&
              ship.handling.world_slip_forward_deceleration_per_lateral_speed > 0.0F,
          "Prototype 01 turns sustained lateral slip into forward speed loss");
    check(ship.presentation.maximum_turn_roll_radians > 0.0F &&
              ship.presentation.turn_roll_response_per_second > 0.0F,
          "Prototype 01 defines its visual turn-roll behavior explicitly");
    check(ship.handling.boost_maximum_speed_multiplier > 1.0F &&
              ship.handling.boost_acceleration_metres_per_second_squared >
                  ship.handling.forward_acceleration_metres_per_second_squared &&
              ship.handling.boost_excess_speed_decay_metres_per_second_squared > 0.0F &&
              ship.handling.boost_duration_seconds > 0.0F &&
              ship.handling.boost_throttle_release_tail_seconds > 0.0F &&
              ship.handling.boost_throttle_release_tail_seconds <
                  ship.handling.boost_duration_seconds,
          "Prototype 01 boost exceeds normal speed and defines its duration and return rate");
    check(ship.collision.maximum_energy == 100.0F,
          "Prototype 01 establishes the baseline energy scale");
}

void test_first_ship_mesh() {
    const hover::render::MeshData mesh = hover::assets::generated::make_prototype_01_mesh();
    const hover::render::MeshData canopy =
        hover::assets::generated::make_prototype_01_canopy_mesh();
    const hover::render::MeshData driver =
        hover::assets::generated::make_prototype_01_driver_mesh();
    const hover::game::LocalBoxCollider collider =
        hover::game::ships::prototype_01_definition().collision.local_bounds;

    check(hover::render::is_valid(mesh), "Prototype 01 produces valid indexed triangles");
    check(hover::render::is_valid(canopy), "Prototype 01 produces valid canopy triangles");
    check(hover::render::is_valid(driver), "Prototype 01 produces valid driver triangles");
    check(mesh.vertices.size() >= 150U, "Prototype 01 is more detailed than the bootstrap mesh");
    check(mesh.vertices.size() < 1000U, "Prototype 01 remains deliberately low-poly");

    bool all_normals_are_unit_length = true;
    bool all_vertices_are_inside_collider = true;
    for (const hover::render::MeshData* part : {&mesh, &canopy, &driver}) {
        for (const hover::render::Vertex& vertex : part->vertices) {
            const float normal_length = std::sqrt(hover::math::dot(vertex.normal, vertex.normal));
            all_normals_are_unit_length =
                all_normals_are_unit_length && std::abs(normal_length - 1.0F) <= tolerance;

            const hover::math::Vec3 relative = vertex.position - collider.center;
            all_vertices_are_inside_collider =
                all_vertices_are_inside_collider &&
                std::abs(relative.x) <= collider.half_extents.x + tolerance &&
                std::abs(relative.y) <= collider.half_extents.y + tolerance &&
                std::abs(relative.z) <= collider.half_extents.z + tolerance;
        }
    }

    bool canopy_is_half_transparent = true;
    for (const hover::render::Vertex& vertex : canopy.vertices) {
        canopy_is_half_transparent =
            canopy_is_half_transparent && std::abs(vertex.opacity - 0.5F) <= tolerance;
    }
    bool driver_is_opaque = true;
    for (const hover::render::Vertex& vertex : driver.vertices) {
        driver_is_opaque = driver_is_opaque && std::abs(vertex.opacity - 1.0F) <= tolerance;
    }

    check(all_normals_are_unit_length, "Prototype 01 has flat unit-length face normals");
    check(all_vertices_are_inside_collider,
          "Prototype 01 visual geometry fits inside its local collision box");
    check(canopy_is_half_transparent, "Prototype 01 canopy has 50 percent opacity");
    check(driver_is_opaque, "Prototype 01 driver silhouette is opaque beneath the canopy");
}

void test_engine_pulse_mesh() {
    const hover::render::MeshData outer = hover::assets::generated::make_engine_pulse_outer_mesh();
    const hover::render::MeshData core = hover::assets::generated::make_engine_pulse_core_mesh();
    const hover::render::MeshData boost_flare =
        hover::assets::generated::make_engine_boost_flare_mesh();
    check(hover::render::is_valid(outer) && hover::render::is_valid(core) &&
              hover::render::is_valid(boost_flare),
          "both engine pulse layers and the boost flare produce valid indexed triangles");
    check(outer.vertices.size() == 36U && outer.indices.size() == 36U &&
              core.vertices.size() == 36U && core.indices.size() == 36U,
          "each engine pulse layer remains a deliberately tiny low-poly plume");

    bool outer_is_transparent_and_rearward = true;
    for (const hover::render::Vertex& vertex : outer.vertices) {
        outer_is_transparent_and_rearward =
            outer_is_transparent_and_rearward && vertex.position.z <= 0.0F &&
            vertex.position.z >= -2.85F && std::abs(vertex.opacity - 0.5F) <= tolerance;
    }
    bool core_is_opaque_and_rearward = true;
    for (const hover::render::Vertex& vertex : core.vertices) {
        core_is_opaque_and_rearward = core_is_opaque_and_rearward && vertex.position.z <= 0.0F &&
                                      vertex.position.z >= -2.35F &&
                                      std::abs(vertex.opacity - 1.0F) <= tolerance;
    }
    check(outer_is_transparent_and_rearward,
          "outer engine pulse extends rearward with 50 percent opacity");
    check(core_is_opaque_and_rearward, "light-blue engine core is opaque and extends rearward");

    bool flare_is_translucent = boost_flare.vertices.size() == 24U;
    for (const hover::render::Vertex& vertex : boost_flare.vertices) {
        flare_is_translucent =
            flare_is_translucent && std::abs(vertex.opacity - 0.65F) <= tolerance;
    }
    check(flare_is_translucent,
          "boost adds a tiny translucent flare mesh distinct from normal exhaust");
}

} // namespace

int main() {
    test_first_ship_definition();
    test_first_ship_mesh();
    test_engine_pulse_mesh();

    if (failure_count != 0) {
        std::cerr << failure_count << " ship test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All ship tests passed\n";
    return EXIT_SUCCESS;
}
