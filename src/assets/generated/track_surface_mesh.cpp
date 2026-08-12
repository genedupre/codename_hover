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
constexpr math::Vec3 wall_color{0.08F, 0.36F, 0.48F};
constexpr float center_band_half_width_metres = 0.45F;
constexpr float wall_height_metres = 2.0F;

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

void add_wall(MeshBuilder& builder, const game::TrackFrame& current, const game::TrackFrame& next,
              float edge_direction, bool reverse_winding) {
    const math::Vec3 current_base = game::point_on_track_frame(
        current,
        {.lateral_metres = edge_direction * current.half_width_metres, .height_metres = 0.0F});
    const math::Vec3 next_base = game::point_on_track_frame(
        next, {.lateral_metres = edge_direction * next.half_width_metres, .height_metres = 0.0F});
    const math::Vec3 current_top = current_base + current.normal * wall_height_metres;
    const math::Vec3 next_top = next_base + next.normal * wall_height_metres;
    if (reverse_winding) {
        builder.add_quad(Quad{current_base, current_top, next_top, next_base, wall_color});
    } else {
        builder.add_quad(Quad{current_base, next_base, next_top, current_top, wall_color});
    }
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

        const game::TrackSegmentProperties properties = track.segment_properties[index];
        if (properties.left_edge == game::TrackEdgePolicy::solid_wall) {
            add_wall(builder, current, next, -1.0F, true);
        }
        if (properties.right_edge == game::TrackEdgePolicy::solid_wall) {
            add_wall(builder, current, next, 1.0F, false);
        }
    }

    return std::move(builder).build();
}

} // namespace hover::assets::generated
