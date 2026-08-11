#include "assets/generated/prototype_01_mesh.hpp"

#include "assets/generated/mesh_builder.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace hover::assets::generated {
namespace {

using math::Vec3;

constexpr Vec3 hull_top{0.20F, 0.10F, 0.56F};
constexpr Vec3 hull_side{0.11F, 0.28F, 0.72F};
constexpr Vec3 hull_lower{0.045F, 0.075F, 0.16F};
constexpr Vec3 wing_top{0.43F, 0.12F, 0.72F};
constexpr Vec3 wing_side{0.15F, 0.13F, 0.42F};
constexpr Vec3 canopy{0.015F, 0.28F, 0.36F};
constexpr Vec3 engine_body{0.12F, 0.18F, 0.40F};
constexpr Vec3 engine_side{0.05F, 0.08F, 0.20F};
constexpr Vec3 exhaust{1.0F, 0.24F, 0.035F};

struct OutwardTriangle {
    Vec3 first;
    Vec3 second;
    Vec3 third;
    Vec3 interior;
    Vec3 color;
};

struct OutwardQuad {
    Vec3 first;
    Vec3 second;
    Vec3 third;
    Vec3 fourth;
    Vec3 interior;
    Vec3 color;
};

void add_outward_triangle(MeshBuilder& builder, const OutwardTriangle& surface) {
    Vec3 second = surface.second;
    Vec3 third = surface.third;
    const Vec3 normal = math::cross(second - surface.first, third - surface.first);
    const Vec3 face_center{
        (surface.first.x + second.x + third.x) / 3.0F,
        (surface.first.y + second.y + third.y) / 3.0F,
        (surface.first.z + second.z + third.z) / 3.0F,
    };
    if (math::dot(normal, face_center - surface.interior) < 0.0F) {
        std::swap(second, third);
    }
    builder.add_triangle(Triangle{surface.first, second, third, surface.color});
}

void add_outward_quad(MeshBuilder& builder, const OutwardQuad& surface) {
    add_outward_triangle(builder, OutwardTriangle{surface.first, surface.second, surface.third,
                                                  surface.interior, surface.color});
    add_outward_triangle(builder, OutwardTriangle{surface.first, surface.third, surface.fourth,
                                                  surface.interior, surface.color});
}

using Ring = std::array<Vec3, 4>;

struct RingConnection {
    const Ring& front;
    const Ring& rear;
    Vec3 interior;
    std::array<Vec3, 4> colors;
};

void connect_rings(MeshBuilder& builder, const RingConnection& connection) {
    for (std::size_t side = 0; side < connection.front.size(); ++side) {
        const std::size_t next = (side + 1U) % connection.front.size();
        add_outward_quad(builder, OutwardQuad{connection.front[side], connection.front[next],
                                              connection.rear[next], connection.rear[side],
                                              connection.interior, connection.colors[side]});
    }
}

void add_hull(MeshBuilder& builder) {
    constexpr Vec3 interior{0.0F, -0.01F, 0.0F};
    constexpr Vec3 nose{0.0F, -0.02F, 2.90F};
    constexpr Ring front_ring{
        Vec3{0.0F, 0.26F, 1.65F},
        Vec3{0.58F, -0.03F, 1.65F},
        Vec3{0.0F, -0.28F, 1.65F},
        Vec3{-0.58F, -0.03F, 1.65F},
    };
    constexpr Ring middle_ring{
        Vec3{0.0F, 0.43F, 0.15F},
        Vec3{1.0F, -0.03F, 0.15F},
        Vec3{0.0F, -0.38F, 0.15F},
        Vec3{-1.0F, -0.03F, 0.15F},
    };
    constexpr Ring rear_ring{
        Vec3{0.0F, 0.24F, -1.65F},
        Vec3{0.78F, -0.03F, -1.65F},
        Vec3{0.0F, -0.32F, -1.65F},
        Vec3{-0.78F, -0.03F, -1.65F},
    };
    constexpr Ring tail_ring{
        Vec3{0.0F, 0.12F, -2.15F},
        Vec3{0.62F, -0.04F, -2.15F},
        Vec3{0.0F, -0.25F, -2.15F},
        Vec3{-0.62F, -0.04F, -2.15F},
    };
    constexpr std::array surface_colors{hull_top, hull_side, hull_lower, hull_side};

    for (std::size_t side = 0; side < front_ring.size(); ++side) {
        const std::size_t next = (side + 1U) % front_ring.size();
        add_outward_triangle(builder, OutwardTriangle{nose, front_ring[side], front_ring[next],
                                                      interior, surface_colors[side]});
    }

    connect_rings(builder, RingConnection{front_ring, middle_ring, interior, surface_colors});
    connect_rings(builder, RingConnection{middle_ring, rear_ring, interior, surface_colors});
    connect_rings(builder, RingConnection{rear_ring, tail_ring, interior, surface_colors});

    constexpr Vec3 tail_center{0.0F, -0.04F, -2.15F};
    for (std::size_t side = 0; side < tail_ring.size(); ++side) {
        const std::size_t next = (side + 1U) % tail_ring.size();
        add_outward_triangle(builder,
                             OutwardTriangle{tail_center, tail_ring[side], tail_ring[next],
                                             interior, side == 1U ? hull_lower : hull_side});
    }
}

void add_wing(MeshBuilder& builder, float side_sign) {
    const std::array<Vec3, 4> top{
        Vec3{side_sign * 0.55F, -0.04F, 1.20F},
        Vec3{side_sign * 1.95F, -0.04F, 0.45F},
        Vec3{side_sign * 1.72F, -0.04F, -1.65F},
        Vec3{side_sign * 0.70F, -0.04F, -1.30F},
    };
    std::array<Vec3, 4> bottom = top;
    for (Vec3& point : bottom) {
        point.y = -0.20F;
    }
    const Vec3 interior{side_sign * 1.18F, -0.12F, -0.25F};

    add_outward_quad(builder, OutwardQuad{top[0], top[1], top[2], top[3], interior, wing_top});
    add_outward_quad(builder,
                     OutwardQuad{bottom[0], bottom[1], bottom[2], bottom[3], interior, hull_lower});
    for (std::size_t edge = 0; edge < top.size(); ++edge) {
        const std::size_t next = (edge + 1U) % top.size();
        add_outward_quad(builder, OutwardQuad{top[edge], top[next], bottom[next], bottom[edge],
                                              interior, wing_side});
    }
}

void add_engine(MeshBuilder& builder, float center_x) {
    constexpr float front_half_width = 0.27F;
    constexpr float rear_half_width = 0.38F;
    const Ring front{
        Vec3{center_x - front_half_width, 0.16F, -0.78F},
        Vec3{center_x + front_half_width, 0.16F, -0.78F},
        Vec3{center_x + front_half_width, -0.27F, -0.78F},
        Vec3{center_x - front_half_width, -0.27F, -0.78F},
    };
    const Ring rear{
        Vec3{center_x - rear_half_width, 0.12F, -2.48F},
        Vec3{center_x + rear_half_width, 0.12F, -2.48F},
        Vec3{center_x + rear_half_width, -0.30F, -2.48F},
        Vec3{center_x - rear_half_width, -0.30F, -2.48F},
    };
    const Vec3 interior{center_x, -0.07F, -1.60F};
    constexpr std::array engine_colors{engine_body, engine_side, engine_side, engine_body};
    connect_rings(builder, RingConnection{front, rear, interior, engine_colors});

    const Vec3 front_center{center_x, -0.055F, -0.78F};
    const Vec3 rear_center{center_x, -0.09F, -2.48F};
    for (std::size_t side = 0; side < front.size(); ++side) {
        const std::size_t next = (side + 1U) % front.size();
        add_outward_triangle(builder, OutwardTriangle{front_center, front[side], front[next],
                                                      interior, engine_body});
        add_outward_triangle(
            builder, OutwardTriangle{rear_center, rear[side], rear[next], interior, exhaust});
    }
}

void add_canopy(MeshBuilder& builder) {
    constexpr Vec3 interior{0.0F, 0.40F, 0.22F};
    constexpr Vec3 front_left{-0.36F, 0.25F, 1.08F};
    constexpr Vec3 front_right{0.36F, 0.25F, 1.08F};
    constexpr Vec3 rear_left{-0.42F, 0.30F, -0.70F};
    constexpr Vec3 rear_right{0.42F, 0.30F, -0.70F};
    constexpr Vec3 roof_front{0.0F, 0.55F, 0.92F};
    constexpr Vec3 roof_rear{0.0F, 0.63F, -0.48F};

    add_outward_triangle(builder,
                         OutwardTriangle{front_left, front_right, roof_front, interior, canopy});
    add_outward_triangle(builder,
                         OutwardTriangle{rear_right, rear_left, roof_rear, interior, canopy});
    add_outward_quad(builder,
                     OutwardQuad{front_left, roof_front, roof_rear, rear_left, interior, canopy});
    add_outward_quad(builder,
                     OutwardQuad{front_right, rear_right, roof_rear, roof_front, interior, canopy});
    add_outward_quad(
        builder, OutwardQuad{front_left, rear_left, rear_right, front_right, interior, hull_top});
}

} // namespace

render::MeshData make_prototype_01_mesh() {
    MeshBuilder builder;
    add_hull(builder);
    add_wing(builder, 1.0F);
    add_wing(builder, -1.0F);
    add_engine(builder, 0.72F);
    add_engine(builder, -0.72F);
    add_canopy(builder);
    return std::move(builder).build();
}

} // namespace hover::assets::generated
