#include "assets/generated/engine_pulse_mesh.hpp"

#include "assets/generated/mesh_builder.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace hover::assets::generated {
namespace {

struct PlumeSpec {
    float horizontal_radius;
    float vertical_radius;
    float length_metres;
    math::Vec3 color;
    float opacity;
};

render::MeshData make_plume(const PlumeSpec& spec) {
    const std::array base_ring{
        math::Vec3{0.0F, spec.vertical_radius, 0.0F},
        math::Vec3{spec.horizontal_radius, 0.0F, 0.0F},
        math::Vec3{0.0F, -spec.vertical_radius, 0.0F},
        math::Vec3{-spec.horizontal_radius, 0.0F, 0.0F},
    };
    const float tip_horizontal_radius = spec.horizontal_radius * 0.12F;
    const float tip_vertical_radius = spec.vertical_radius * 0.12F;
    const std::array tip_ring{
        math::Vec3{0.0F, tip_vertical_radius, -spec.length_metres},
        math::Vec3{tip_horizontal_radius, 0.0F, -spec.length_metres},
        math::Vec3{0.0F, -tip_vertical_radius, -spec.length_metres},
        math::Vec3{-tip_horizontal_radius, 0.0F, -spec.length_metres},
    };
    const math::Vec3 tip_center{0.0F, 0.0F, -spec.length_metres};

    MeshBuilder builder;
    for (std::size_t side = 0; side < base_ring.size(); ++side) {
        const std::size_t next = (side + 1U) % base_ring.size();
        builder.add_quad(Quad{
            base_ring[side],
            tip_ring[side],
            tip_ring[next],
            base_ring[next],
            spec.color,
            spec.opacity,
        });
        builder.add_triangle(Triangle{
            tip_ring[side],
            tip_center,
            tip_ring[next],
            spec.color,
            spec.opacity,
        });
    }
    return std::move(builder).build();
}

} // namespace

render::MeshData make_engine_pulse_outer_mesh() {
    return make_plume(PlumeSpec{
        .horizontal_radius = 0.38F,
        .vertical_radius = 0.30F,
        .length_metres = 2.85F,
        .color = {0.30F, 0.78F, 1.0F},
        .opacity = 0.5F,
    });
}

render::MeshData make_engine_pulse_core_mesh() {
    return make_plume(PlumeSpec{
        .horizontal_radius = 0.20F,
        .vertical_radius = 0.15F,
        .length_metres = 2.35F,
        .color = {0.68F, 0.93F, 1.0F},
        .opacity = 1.0F,
    });
}

render::MeshData make_engine_boost_flare_mesh() {
    constexpr math::Vec3 flare_color{0.72F, 0.96F, 1.0F};
    constexpr float opacity = 0.65F;
    constexpr std::array ring{
        math::Vec3{0.0F, 0.62F, -0.35F},
        math::Vec3{0.78F, 0.0F, -0.35F},
        math::Vec3{0.0F, -0.62F, -0.35F},
        math::Vec3{-0.78F, 0.0F, -0.35F},
    };
    constexpr math::Vec3 front_center{0.0F, 0.0F, 0.15F};
    constexpr math::Vec3 rear_center{0.0F, 0.0F, -1.45F};

    MeshBuilder builder;
    for (std::size_t side = 0; side < ring.size(); ++side) {
        const std::size_t next = (side + 1U) % ring.size();
        builder.add_triangle(
            Triangle{front_center, ring[side], ring[next], flare_color, opacity});
        builder.add_triangle(
            Triangle{rear_center, ring[next], ring[side], flare_color, opacity});
    }
    return std::move(builder).build();
}

} // namespace hover::assets::generated
