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
    check(ship.collision.maximum_energy == 100.0F,
          "Prototype 01 establishes the baseline energy scale");
}

void test_first_ship_mesh() {
    const hover::render::MeshData mesh = hover::assets::generated::make_prototype_01_mesh();
    const hover::game::LocalBoxCollider collider =
        hover::game::ships::prototype_01_definition().collision.local_bounds;

    check(hover::render::is_valid(mesh), "Prototype 01 produces valid indexed triangles");
    check(mesh.vertices.size() >= 150U, "Prototype 01 is more detailed than the bootstrap mesh");
    check(mesh.vertices.size() < 1000U, "Prototype 01 remains deliberately low-poly");

    bool all_normals_are_unit_length = true;
    bool all_vertices_are_inside_collider = true;
    for (const hover::render::Vertex& vertex : mesh.vertices) {
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

    check(all_normals_are_unit_length, "Prototype 01 has flat unit-length face normals");
    check(all_vertices_are_inside_collider,
          "Prototype 01 visual geometry fits inside its local collision box");
}

void test_engine_pulse_mesh() {
    const hover::render::MeshData mesh = hover::assets::generated::make_engine_pulse_mesh();
    check(hover::render::is_valid(mesh), "engine pulse produces valid indexed triangles");
    check(mesh.vertices.size() == 36U && mesh.indices.size() == 36U,
          "engine pulse remains a deliberately tiny low-poly plume");

    bool begins_at_socket_and_extends_rearward = true;
    for (const hover::render::Vertex& vertex : mesh.vertices) {
        begins_at_socket_and_extends_rearward = begins_at_socket_and_extends_rearward &&
                                                vertex.position.z <= 0.0F &&
                                                vertex.position.z >= -1.35F;
    }
    check(begins_at_socket_and_extends_rearward,
          "engine pulse is authored behind its local engine socket");
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
