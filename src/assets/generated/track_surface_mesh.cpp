#include "assets/generated/track_surface_mesh.hpp"

#include "assets/generated/mesh_builder.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

namespace hover::assets::generated {
namespace {

constexpr math::Vec3 side_color{0.055F, 0.10F, 0.16F};
constexpr math::Vec3 center_color{0.07F, 0.24F, 0.30F};
constexpr math::Vec3 start_color{0.68F, 0.30F, 0.08F};
constexpr float center_band_half_width_metres = 0.45F;

void add_band(MeshBuilder& builder, const game::TrackFrame& current, const game::TrackFrame& next,
              float current_left_offset_metres, float current_right_offset_metres,
              float next_left_offset_metres, float next_right_offset_metres, math::Vec3 color) {
    const auto point = [](const game::TrackFrame& frame, float lateral_metres) {
        return game::point_on_track_frame(
            frame, game::TrackOffset{.lateral_metres = lateral_metres, .height_metres = 0.0F});
    };

    builder.add_quad(Quad{
        point(current, current_left_offset_metres),
        point(next, next_left_offset_metres),
        point(next, next_right_offset_metres),
        point(current, current_right_offset_metres),
        color,
    });
}

} // namespace

render::MeshData make_track_surface_mesh(const game::SampledTrack& track) {
    assert(game::is_valid(track));

    MeshBuilder builder;
    for (std::size_t index = 0; index < track.frames.size(); ++index) {
        const game::TrackFrame& current = track.frames[index];
        const game::TrackFrame& next = track.frames[(index + 1U) % track.frames.size()];
        const float current_band_half_width =
            std::min(center_band_half_width_metres, current.half_width_metres * 0.25F);
        const float next_band_half_width =
            std::min(center_band_half_width_metres, next.half_width_metres * 0.25F);
        const math::Vec3 segment_side_color = index == 0U ? start_color : side_color;
        const math::Vec3 segment_center_color = index == 0U ? start_color : center_color;

        add_band(builder, current, next, -current.half_width_metres, -current_band_half_width,
                 -next.half_width_metres, -next_band_half_width, segment_side_color);
        add_band(builder, current, next, -current_band_half_width, current_band_half_width,
                 -next_band_half_width, next_band_half_width, segment_center_color);
        add_band(builder, current, next, current_band_half_width, current.half_width_metres,
                 next_band_half_width, next.half_width_metres, segment_side_color);
    }

    return std::move(builder).build();
}

} // namespace hover::assets::generated
