#include "hover_math.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

constexpr float tolerance = 0.0001F;
int failure_count = 0;

bool nearly_equal(float left, float right) { return std::abs(left - right) <= tolerance; }

void check(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failure_count;
    }
}

void test_identity_transform() {
    const hover::math::Vec4 value{1.0F, -2.0F, 3.0F, 1.0F};
    const hover::math::Vec4 transformed = hover::math::transform(hover::math::identity(), value);

    check(nearly_equal(transformed.x, value.x), "identity preserves x");
    check(nearly_equal(transformed.y, value.y), "identity preserves y");
    check(nearly_equal(transformed.z, value.z), "identity preserves z");
    check(nearly_equal(transformed.w, value.w), "identity preserves w");
}

void test_left_handed_view() {
    const hover::math::Vec3 eye{0.0F, 2.0F, -5.0F};
    const hover::math::Vec3 target{0.0F, 2.0F, 0.0F};
    const hover::math::Mat4 view = hover::math::look_at_lh(
        hover::math::LookAt{eye, target, hover::math::Vec3{0.0F, 1.0F, 0.0F}});

    const hover::math::Vec4 eye_in_view =
        hover::math::transform(view, hover::math::Vec4{eye.x, eye.y, eye.z, 1.0F});
    const hover::math::Vec4 target_in_view =
        hover::math::transform(view, hover::math::Vec4{target.x, target.y, target.z, 1.0F});

    check(nearly_equal(eye_in_view.x, 0.0F) && nearly_equal(eye_in_view.y, 0.0F) &&
              nearly_equal(eye_in_view.z, 0.0F),
          "view maps the camera position to the origin");
    check(nearly_equal(target_in_view.z, 5.0F), "left-handed view looks toward positive z");
}

void test_zero_to_one_projection_depth() {
    constexpr float near_plane = 0.1F;
    constexpr float far_plane = 100.0F;
    const hover::math::Mat4 projection = hover::math::perspective_lh(
        hover::math::Perspective{1.0471975512F, 16.0F / 9.0F, near_plane, far_plane});

    const hover::math::Vec4 near_clip =
        hover::math::transform(projection, hover::math::Vec4{0.0F, 0.0F, near_plane, 1.0F});
    const hover::math::Vec4 far_clip =
        hover::math::transform(projection, hover::math::Vec4{0.0F, 0.0F, far_plane, 1.0F});

    check(nearly_equal(near_clip.z / near_clip.w, 0.0F), "near plane maps to depth zero");
    check(nearly_equal(far_clip.z / far_clip.w, 1.0F), "far plane maps to depth one");
}

void test_matrix_composition_order() {
    const hover::math::Mat4 view = hover::math::look_at_lh(hover::math::LookAt{
        hover::math::Vec3{0.0F, 0.0F, -5.0F},
        hover::math::Vec3{0.0F, 0.0F, 0.0F},
        hover::math::Vec3{0.0F, 1.0F, 0.0F},
    });
    const hover::math::Mat4 projection =
        hover::math::perspective_lh(hover::math::Perspective{1.0471975512F, 1.0F, 0.1F, 100.0F});

    const hover::math::Vec4 clip =
        hover::math::transform(projection * view, hover::math::Vec4{0.0F, 0.0F, 0.0F, 1.0F});

    check(nearly_equal(clip.x, 0.0F) && nearly_equal(clip.y, 0.0F),
          "view-projection keeps the target centered");
    check(clip.w > 0.0F, "view-projection places the target in front of the camera");
}

void test_uniform_scaling() {
    const hover::math::Vec4 transformed = hover::math::transform(
        hover::math::translation({5.0F, 7.0F, 11.0F}) * hover::math::scaling(2.0F),
        hover::math::Vec4{1.0F, 2.0F, 3.0F, 1.0F});

    check(nearly_equal(transformed.x, 7.0F) && nearly_equal(transformed.y, 11.0F) &&
              nearly_equal(transformed.z, 17.0F) && nearly_equal(transformed.w, 1.0F),
          "uniform scaling happens around local origin before translation");
}

} // namespace

int main() {
    test_identity_transform();
    test_left_handed_view();
    test_zero_to_one_projection_depth();
    test_matrix_composition_order();
    test_uniform_scaling();

    if (failure_count != 0) {
        std::cerr << failure_count << " math test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All hover math tests passed\n";
    return EXIT_SUCCESS;
}
