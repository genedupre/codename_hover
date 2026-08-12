#include "assets/generated/track_surface_mesh.hpp"
#include "game/track.hpp"
#include "game/tracks/oval_track.hpp"
#include "game/tracks/speedway_track.hpp"
#include "hover_math.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <string_view>

namespace {

constexpr float tight_tolerance = 0.002F;
constexpr float sampled_tolerance = 0.02F;
int failure_count = 0;

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

bool nearly_equal(float left, float right, float tolerance = tight_tolerance) {
    return std::abs(left - right) <= tolerance;
}

bool nearly_equal(hover::math::Vec3 left, hover::math::Vec3 right,
                  float tolerance = tight_tolerance) {
    return nearly_equal(left.x, right.x, tolerance) && nearly_equal(left.y, right.y, tolerance) &&
           nearly_equal(left.z, right.z, tolerance);
}

hover::game::tracks::OvalTrackDefinition test_definition() {
    return hover::game::tracks::OvalTrackDefinition{
        .straight_length_metres = 100.0F,
        .turn_radius_metres = 40.0F,
        .half_width_metres = 12.0F,
        .elevation_metres = 3.0F,
    };
}

hover::game::SampledTrack make_test_track() {
    return hover::game::tracks::make_sampled_oval(
        hover::game::tracks::OvalTrackBuild{test_definition(), 512U});
}

void test_definition_and_length() {
    const hover::game::tracks::OvalTrackDefinition definition = test_definition();
    check(hover::game::tracks::is_valid(definition), "the test oval definition is valid");

    const float expected_length = 200.0F + 80.0F * std::numbers::pi_v<float>;
    check(nearly_equal(hover::game::tracks::oval_track_length(definition), expected_length),
          "oval length is two straights plus one full circle");

    hover::game::tracks::OvalTrackDefinition invalid = definition;
    invalid.turn_radius_metres = invalid.half_width_metres;
    check(!hover::game::tracks::is_valid(invalid),
          "turn radius must leave room for the track half-width");
}

void test_track_rejects_frame_flips() {
    hover::game::SampledTrack track = make_test_track();
    hover::game::TrackFrame& flipped = track.frames[10];
    flipped.tangent = flipped.tangent * -1.0F;
    flipped.binormal = flipped.binormal * -1.0F;

    check(hover::game::is_valid(flipped), "a flipped frame can still be independently orthonormal");
    check(!hover::game::is_valid(track),
          "sampled track rejects incompatible adjacent frame orientation");
}

void test_sampled_track_and_landmarks() {
    const hover::game::tracks::OvalTrackDefinition definition = test_definition();
    const hover::game::SampledTrack track = make_test_track();
    check(hover::game::is_valid(track), "generated oval is a valid sampled track");
    check(track.frames.size() == 512U, "generated oval preserves the requested sample count");

    const hover::game::TrackFrame start = hover::game::sample_track(track, 0.0F);
    check(nearly_equal(start.center, {40.0F, 3.0F, -50.0F}), "oval starts on the right straight");
    check(nearly_equal(start.tangent, {0.0F, 0.0F, 1.0F}) &&
              nearly_equal(start.normal, {0.0F, 1.0F, 0.0F}) &&
              nearly_equal(start.binormal, {1.0F, 0.0F, 0.0F}),
          "start frame faces positive Z with positive lateral offset to the right");

    const float top_turn_midpoint =
        definition.straight_length_metres +
        definition.turn_radius_metres * std::numbers::pi_v<float> * 0.5F;
    const hover::game::TrackFrame top = hover::game::sample_track(track, top_turn_midpoint);
    check(nearly_equal(top.center, {0.0F, 3.0F, 90.0F}, sampled_tolerance),
          "far turn reaches its expected midpoint");
    check(nearly_equal(top.tangent, {-1.0F, 0.0F, 0.0F}, sampled_tolerance),
          "far turn midpoint faces negative X");

    const hover::game::TrackFrame halfway =
        hover::game::sample_track(track, track.length_metres * 0.5F);
    check(nearly_equal(halfway.center, {-40.0F, 3.0F, 50.0F}, sampled_tolerance) &&
              nearly_equal(halfway.tangent, {0.0F, 0.0F, -1.0F}, sampled_tolerance),
          "halfway point begins the opposite straight");
}

void test_wrapping_and_closed_seam() {
    const hover::game::SampledTrack track = make_test_track();
    const hover::game::TrackFrame start = hover::game::sample_track(track, 0.0F);
    const hover::game::TrackFrame wrapped = hover::game::sample_track(track, track.length_metres);
    check(nearly_equal(wrapped.distance_metres, 0.0F) &&
              nearly_equal(wrapped.center, start.center) &&
              nearly_equal(wrapped.tangent, start.tangent),
          "track length wraps exactly to the start frame");

    const hover::game::TrackFrame before =
        hover::game::sample_track(track, track.length_metres - 0.01F);
    const hover::game::TrackFrame after = hover::game::sample_track(track, 0.01F);
    const hover::math::Vec3 seam_delta = after.center - before.center;
    check(hover::math::dot(seam_delta, seam_delta) < 0.001F,
          "positions remain continuous across the closed seam");
    check(hover::math::dot(before.tangent, after.tangent) > 0.999F &&
              hover::math::dot(before.binormal, after.binormal) > 0.999F,
          "orientation remains continuous across the closed seam");

    const hover::game::TrackFrame negative = hover::game::sample_track(track, -0.01F);
    check(nearly_equal(negative.center, before.center, sampled_tolerance),
          "negative distance wraps backward from the seam");
}

void test_frame_quality_and_offsets() {
    const hover::game::SampledTrack track = make_test_track();
    for (const hover::game::TrackFrame& frame : track.frames) {
        check(hover::game::is_valid(frame), "every generated frame is orthonormal and finite");
        check(hover::math::dot(hover::math::cross(frame.normal, frame.tangent), frame.binormal) >
                  0.999F,
              "every frame keeps binormal on track-right");
    }

    const float sample_spacing = track.length_metres / static_cast<float>(track.frames.size());
    for (const hover::game::TrackFrame& frame : track.frames) {
        const hover::game::TrackFrame interpolated =
            hover::game::sample_track(track, frame.distance_metres + sample_spacing * 0.5F);
        check(hover::game::is_valid(interpolated),
              "interpolation between stored samples preserves a valid frame");
    }

    const hover::game::TrackFrame start = hover::game::sample_track(track, 0.0F);
    const hover::math::Vec3 offset = hover::game::point_on_track_frame(
        start, hover::game::TrackOffset{.lateral_metres = 5.0F, .height_metres = 2.0F});
    check(nearly_equal(offset, {45.0F, 5.0F, -50.0F}),
          "track offsets apply lateral movement to the right and height along the normal");
}

void test_world_point_projection() {
    const hover::game::SampledTrack track = make_test_track();
    constexpr float target_distance = 18.0F;
    const hover::game::TrackFrame target = hover::game::sample_track(track, target_distance);
    const hover::math::Vec3 point = hover::game::point_on_track_frame(
        target, hover::game::TrackOffset{.lateral_metres = 4.5F, .height_metres = 1.25F});

    const hover::game::TrackProjection projection =
        hover::game::project_point_onto_track(track, point, 12.0F, 12.0F);
    check(nearly_equal(projection.frame.distance_metres, target_distance, sampled_tolerance),
          "projection recovers distance from a nearby world point");
    check(nearly_equal(projection.offset.lateral_metres, 4.5F, sampled_tolerance) &&
              nearly_equal(projection.offset.height_metres, 1.25F, sampled_tolerance),
          "projection recovers signed track-right and surface-normal offsets");
    check(nearly_equal(projection.signed_distance_from_hint_metres, 6.0F, sampled_tolerance),
          "projection reports signed progress from its path hint");
    check(nearly_equal(projection.centerline_distance_metres,
                       std::sqrt(4.5F * 4.5F + 1.25F * 1.25F), sampled_tolerance),
          "projection reports three-dimensional distance from the centerline");

    const hover::game::TrackProjection bounded =
        hover::game::project_point_onto_track(track, point, 12.0F, 3.0F);
    check(nearly_equal(bounded.frame.distance_metres, 15.0F, sampled_tolerance) &&
              nearly_equal(bounded.signed_distance_from_hint_metres, 3.0F, sampled_tolerance),
          "projection cannot silently escape its bounded along-path search window");
}

void test_world_point_projection_across_seam() {
    const hover::game::SampledTrack track = make_test_track();
    const float target_distance = track.length_metres - 1.0F;
    const hover::game::TrackFrame target = hover::game::sample_track(track, target_distance);
    const hover::math::Vec3 point = hover::game::point_on_track_frame(
        target, hover::game::TrackOffset{.lateral_metres = -2.0F, .height_metres = 0.75F});

    const hover::game::TrackProjection projection =
        hover::game::project_point_onto_track(track, point, 1.0F, 5.0F);
    check(nearly_equal(projection.frame.distance_metres, target_distance, sampled_tolerance),
          "hinted projection searches backward across the closed seam");
    check(nearly_equal(projection.signed_distance_from_hint_metres, -2.0F, sampled_tolerance),
          "seam projection reports the short signed path displacement");
}

void test_world_point_projection_on_banked_surface() {
    const hover::game::tracks::OvalTrackDefinition oval = test_definition();
    const hover::game::SampledTrack track =
        hover::game::tracks::make_sampled_speedway(hover::game::tracks::SpeedwayTrackBuild{
            .definition =
                hover::game::tracks::SpeedwayTrackDefinition{
                    .oval = oval,
                    .maximum_bank_radians = 0.4886921906F,
                    .bank_transition_metres = 20.0F,
                },
            .sample_count = 512U,
        });
    const float target_distance =
        oval.straight_length_metres + oval.turn_radius_metres * std::numbers::pi_v<float> * 0.5F;
    const hover::game::TrackFrame target = hover::game::sample_track(track, target_distance);
    const hover::math::Vec3 point = hover::game::point_on_track_frame(
        target, hover::game::TrackOffset{.lateral_metres = 3.0F, .height_metres = 2.0F});

    const hover::game::TrackProjection projection =
        hover::game::project_point_onto_track(track, point, target_distance + 1.0F, 4.0F);
    check(nearly_equal(projection.offset.lateral_metres, 3.0F, sampled_tolerance) &&
              nearly_equal(projection.offset.height_metres, 2.0F, sampled_tolerance),
          "projection measures offsets in the banked frame instead of world axes");
}

void test_generated_surface_mesh() {
    const hover::game::SampledTrack track = make_test_track();
    const hover::render::MeshData mesh = hover::assets::generated::make_track_surface_mesh(track);

    constexpr std::size_t vertices_per_quad = 6U;
    constexpr std::size_t bands_per_segment = 3U;
    const std::size_t expected_element_count =
        track.frames.size() * bands_per_segment * vertices_per_quad;
    check(hover::render::is_valid(mesh), "generated track surface is a valid indexed mesh");
    check(mesh.vertices.size() == expected_element_count &&
              mesh.indices.size() == expected_element_count,
          "surface includes three bands and the closing segment for every track frame");

    for (const hover::render::Vertex& vertex : mesh.vertices) {
        check(vertex.normal.y > 0.99F, "flat oval surface triangles face upward");
    }
}

void test_banked_speedway() {
    const hover::game::tracks::OvalTrackDefinition oval = test_definition();
    constexpr float maximum_bank_radians = 0.4886921906F;
    const hover::game::tracks::SpeedwayTrackDefinition definition{
        .oval = oval,
        .maximum_bank_radians = maximum_bank_radians,
        .bank_transition_metres = 20.0F,
    };
    check(hover::game::tracks::is_valid(definition), "the banked speedway definition is valid");

    hover::game::tracks::SpeedwayTrackDefinition invalid = definition;
    invalid.bank_transition_metres = oval.turn_radius_metres * std::numbers::pi_v<float> * 0.51F;
    check(!hover::game::tracks::is_valid(invalid),
          "speedway bank transition must fit inside half a turn");

    const hover::game::SampledTrack track = hover::game::tracks::make_sampled_speedway(
        hover::game::tracks::SpeedwayTrackBuild{definition, 512U});
    check(hover::game::is_valid(track) && track.frames.size() == 512U,
          "speedway produces a valid generic sampled track");
    check(nearly_equal(track.length_metres, hover::game::tracks::oval_track_length(oval)),
          "banking does not change the speedway centerline length");

    const hover::game::TrackFrame start = hover::game::sample_track(track, 0.0F);
    check(nearly_equal(start.normal, {0.0F, 1.0F, 0.0F}) &&
              nearly_equal(start.binormal, {1.0F, 0.0F, 0.0F}),
          "speedway seam and straight begin level");

    const float far_turn_midpoint =
        oval.straight_length_metres + oval.turn_radius_metres * std::numbers::pi_v<float> * 0.5F;
    const hover::game::TrackFrame banked = hover::game::sample_track(track, far_turn_midpoint);
    check(nearly_equal(banked.normal.y, std::cos(maximum_bank_radians), sampled_tolerance) &&
              nearly_equal(banked.binormal.y, std::sin(maximum_bank_radians), sampled_tolerance),
          "turn midpoint reaches the configured bank angle and raises track-right");

    const hover::math::Vec3 left_edge = hover::game::point_on_track_frame(
        banked,
        hover::game::TrackOffset{.lateral_metres = -oval.half_width_metres, .height_metres = 0.0F});
    const hover::math::Vec3 right_edge = hover::game::point_on_track_frame(
        banked,
        hover::game::TrackOffset{.lateral_metres = oval.half_width_metres, .height_metres = 0.0F});
    check(left_edge.y < banked.center.y && right_edge.y > banked.center.y,
          "banked left turn lowers its inside edge and raises its outside edge");

    const hover::render::MeshData mesh = hover::assets::generated::make_track_surface_mesh(track);
    bool has_banked_surface = false;
    for (const hover::render::Vertex& vertex : mesh.vertices) {
        has_banked_surface = has_banked_surface || vertex.normal.y < 0.95F;
    }
    check(hover::render::is_valid(mesh) && has_banked_surface,
          "generic track mesh preserves visibly angled speedway corners");
}

} // namespace

int main() {
    test_definition_and_length();
    test_track_rejects_frame_flips();
    test_sampled_track_and_landmarks();
    test_wrapping_and_closed_seam();
    test_frame_quality_and_offsets();
    test_world_point_projection();
    test_world_point_projection_across_seam();
    test_world_point_projection_on_banked_surface();
    test_generated_surface_mesh();
    test_banked_speedway();

    if (failure_count != 0) {
        std::cerr << failure_count << " track test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All track tests passed\n";
    return EXIT_SUCCESS;
}
