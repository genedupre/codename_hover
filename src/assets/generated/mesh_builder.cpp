#include "assets/generated/mesh_builder.hpp"

#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>

namespace hover::assets::generated {

void MeshBuilder::add_triangle(const Triangle& triangle) {
    assert(mesh_.vertices.size() <= std::numeric_limits<std::uint32_t>::max() - 3U);
    const math::Vec3 normal = math::normalized(
        math::cross(triangle.second - triangle.first, triangle.third - triangle.first));
    const std::uint32_t first_index = static_cast<std::uint32_t>(mesh_.vertices.size());

    mesh_.vertices.push_back(render::Vertex{triangle.first, normal, triangle.color});
    mesh_.vertices.push_back(render::Vertex{triangle.second, normal, triangle.color});
    mesh_.vertices.push_back(render::Vertex{triangle.third, normal, triangle.color});
    mesh_.indices.push_back(first_index);
    mesh_.indices.push_back(first_index + 1U);
    mesh_.indices.push_back(first_index + 2U);
}

void MeshBuilder::add_quad(const Quad& quad) {
    add_triangle(Triangle{quad.first, quad.second, quad.third, quad.color});
    add_triangle(Triangle{quad.first, quad.third, quad.fourth, quad.color});
}

render::MeshData MeshBuilder::build() && { return std::move(mesh_); }

} // namespace hover::assets::generated
