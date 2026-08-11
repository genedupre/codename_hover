#include "assets/generated/presentation_pad.hpp"

#include "assets/generated/mesh_builder.hpp"

#include <utility>

namespace hover::assets::generated {

render::MeshData make_presentation_pad_mesh() {
    constexpr float height = -0.62F;
    constexpr float front = -3.20F;
    constexpr float rear = 3.60F;
    constexpr math::Vec3 side_color{0.055F, 0.10F, 0.16F};
    constexpr math::Vec3 center_color{0.07F, 0.24F, 0.30F};

    MeshBuilder builder;
    builder.add_quad(Quad{{-3.4F, height, front},
                          {-3.4F, height, rear},
                          {-0.32F, height, rear},
                          {-0.32F, height, front},
                          side_color});
    builder.add_quad(Quad{{-0.32F, height, front},
                          {-0.32F, height, rear},
                          {0.32F, height, rear},
                          {0.32F, height, front},
                          center_color});
    builder.add_quad(Quad{{0.32F, height, front},
                          {0.32F, height, rear},
                          {3.4F, height, rear},
                          {3.4F, height, front},
                          side_color});
    return std::move(builder).build();
}

} // namespace hover::assets::generated
