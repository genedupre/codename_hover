#include "assets/generated/presentation_pad.hpp"

#include "assets/generated/mesh_builder.hpp"

#include <utility>

namespace hover::assets::generated {

render::MeshData make_presentation_pad_mesh() {
    constexpr float height = -0.62F;
    constexpr float front = -250.0F;
    constexpr float rear = 5000.0F;
    constexpr float half_width = 100.0F;
    constexpr math::Vec3 side_color{0.055F, 0.10F, 0.16F};
    constexpr math::Vec3 center_color{0.07F, 0.24F, 0.30F};
    constexpr math::Vec3 marker_color{0.11F, 0.20F, 0.27F};

    MeshBuilder builder;
    builder.add_quad(Quad{{-half_width, height, front},
                          {-half_width, height, rear},
                          {-0.45F, height, rear},
                          {-0.45F, height, front},
                          side_color});
    builder.add_quad(Quad{{-0.45F, height, front},
                          {-0.45F, height, rear},
                          {0.45F, height, rear},
                          {0.45F, height, front},
                          center_color});
    builder.add_quad(Quad{{0.45F, height, front},
                          {0.45F, height, rear},
                          {half_width, height, rear},
                          {half_width, height, front},
                          side_color});

    constexpr float marker_height = height + 0.004F;
    constexpr float marker_half_depth = 0.20F;
    constexpr float first_marker_z = -200.0F;
    constexpr float marker_spacing = 50.0F;
    constexpr int marker_count = 105;
    for (int marker_index = 0; marker_index < marker_count; ++marker_index) {
        const float marker_z = first_marker_z + static_cast<float>(marker_index) * marker_spacing;
        builder.add_quad(Quad{{-half_width, marker_height, marker_z - marker_half_depth},
                              {-half_width, marker_height, marker_z + marker_half_depth},
                              {half_width, marker_height, marker_z + marker_half_depth},
                              {half_width, marker_height, marker_z - marker_half_depth},
                              marker_color});
    }
    return std::move(builder).build();
}

} // namespace hover::assets::generated
