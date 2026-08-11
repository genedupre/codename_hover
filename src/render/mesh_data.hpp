#pragma once

#include "hover_math.hpp"

#include <cstdint>
#include <vector>

namespace hover::render {

struct Vertex {
    math::Vec3 position;
    math::Vec3 normal;
    math::Vec3 color;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

[[nodiscard]] inline bool is_valid(const MeshData& mesh) {
    if (mesh.vertices.empty() || mesh.indices.empty() || mesh.indices.size() % 3U != 0U) {
        return false;
    }

    for (const std::uint32_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            return false;
        }
    }
    return true;
}

} // namespace hover::render
