#include "assets/generated/engine_pulse_mesh.hpp"

#include "assets/generated/mesh_builder.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace hover::assets::generated {

render::MeshData make_engine_pulse_mesh() {
    constexpr std::array base_ring{
        math::Vec3{0.0F, 0.22F, 0.0F},
        math::Vec3{0.28F, 0.0F, 0.0F},
        math::Vec3{0.0F, -0.22F, 0.0F},
        math::Vec3{-0.28F, 0.0F, 0.0F},
    };
    constexpr std::array tip_ring{
        math::Vec3{0.0F, 0.035F, -1.35F},
        math::Vec3{0.045F, 0.0F, -1.35F},
        math::Vec3{0.0F, -0.035F, -1.35F},
        math::Vec3{-0.045F, 0.0F, -1.35F},
    };
    constexpr std::array colors{
        math::Vec3{1.0F, 0.22F, 0.025F},
        math::Vec3{1.0F, 0.62F, 0.055F},
    };
    constexpr math::Vec3 tip_center{0.0F, 0.0F, -1.35F};

    MeshBuilder builder;
    for (std::size_t side = 0; side < base_ring.size(); ++side) {
        const std::size_t next = (side + 1U) % base_ring.size();
        builder.add_quad(Quad{
            base_ring[side],
            tip_ring[side],
            tip_ring[next],
            base_ring[next],
            colors[side % colors.size()],
        });
        builder.add_triangle(Triangle{
            tip_ring[side],
            tip_center,
            tip_ring[next],
            colors[(side + 1U) % colors.size()],
        });
    }
    return std::move(builder).build();
}

} // namespace hover::assets::generated
