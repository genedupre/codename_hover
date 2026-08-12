#include "hover_math.hpp"

#include <cassert>
#include <cmath>

namespace hover::math {

float& Mat4::at(int row, int column) {
    assert(row >= 0 && row < 4);
    assert(column >= 0 && column < 4);
    return elements[static_cast<std::size_t>(column) * 4U + static_cast<std::size_t>(row)];
}

const float& Mat4::at(int row, int column) const {
    assert(row >= 0 && row < 4);
    assert(column >= 0 && column < 4);
    return elements[static_cast<std::size_t>(column) * 4U + static_cast<std::size_t>(row)];
}

Vec3 operator-(Vec3 left, Vec3 right) {
    return Vec3{left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3 operator+(Vec3 left, Vec3 right) {
    return Vec3{left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3 operator*(Vec3 vector, float scalar) {
    return Vec3{vector.x * scalar, vector.y * scalar, vector.z * scalar};
}

float dot(Vec3 left, Vec3 right) { return left.x * right.x + left.y * right.y + left.z * right.z; }

Vec3 cross(Vec3 left, Vec3 right) {
    return Vec3{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x,
    };
}

Vec3 normalized(Vec3 value) {
    const float length = std::sqrt(dot(value, value));
    assert(length > 0.0F);
    return Vec3{value.x / length, value.y / length, value.z / length};
}

Mat4 identity() {
    Mat4 result{};
    result.at(0, 0) = 1.0F;
    result.at(1, 1) = 1.0F;
    result.at(2, 2) = 1.0F;
    result.at(3, 3) = 1.0F;
    return result;
}

Mat4 translation(Vec3 offset) {
    Mat4 result = identity();
    result.at(0, 3) = offset.x;
    result.at(1, 3) = offset.y;
    result.at(2, 3) = offset.z;
    return result;
}

Mat4 scaling(float uniform_scale) {
    return scaling(Vec3{uniform_scale, uniform_scale, uniform_scale});
}

Mat4 scaling(Vec3 scale) {
    Mat4 result{};
    result.at(0, 0) = scale.x;
    result.at(1, 1) = scale.y;
    result.at(2, 2) = scale.z;
    result.at(3, 3) = 1.0F;
    return result;
}

Mat4 rotation_y(float radians) {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    Mat4 result = identity();
    result.at(0, 0) = cosine;
    result.at(0, 2) = sine;
    result.at(2, 0) = -sine;
    result.at(2, 2) = cosine;
    return result;
}

Mat4 operator*(const Mat4& left, const Mat4& right) {
    Mat4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            for (int index = 0; index < 4; ++index) {
                result.at(row, column) += left.at(row, index) * right.at(index, column);
            }
        }
    }
    return result;
}

Vec4 transform(const Mat4& matrix, Vec4 vector) {
    return Vec4{
        matrix.at(0, 0) * vector.x + matrix.at(0, 1) * vector.y + matrix.at(0, 2) * vector.z +
            matrix.at(0, 3) * vector.w,
        matrix.at(1, 0) * vector.x + matrix.at(1, 1) * vector.y + matrix.at(1, 2) * vector.z +
            matrix.at(1, 3) * vector.w,
        matrix.at(2, 0) * vector.x + matrix.at(2, 1) * vector.y + matrix.at(2, 2) * vector.z +
            matrix.at(2, 3) * vector.w,
        matrix.at(3, 0) * vector.x + matrix.at(3, 1) * vector.y + matrix.at(3, 2) * vector.z +
            matrix.at(3, 3) * vector.w,
    };
}

Mat4 look_at_lh(const LookAt& camera) {
    const Vec3 forward = normalized(camera.target - camera.eye);
    const Vec3 right = normalized(cross(camera.world_up, forward));
    const Vec3 up = cross(forward, right);

    Mat4 result = identity();
    result.at(0, 0) = right.x;
    result.at(0, 1) = right.y;
    result.at(0, 2) = right.z;
    result.at(0, 3) = -dot(right, camera.eye);

    result.at(1, 0) = up.x;
    result.at(1, 1) = up.y;
    result.at(1, 2) = up.z;
    result.at(1, 3) = -dot(up, camera.eye);

    result.at(2, 0) = forward.x;
    result.at(2, 1) = forward.y;
    result.at(2, 2) = forward.z;
    result.at(2, 3) = -dot(forward, camera.eye);
    return result;
}

Mat4 perspective_lh(const Perspective& projection) {
    assert(projection.vertical_field_of_view_radians > 0.0F);
    assert(projection.vertical_field_of_view_radians < 3.14159265358979323846F);
    assert(projection.aspect_ratio > 0.0F);
    assert(projection.near_plane > 0.0F);
    assert(projection.far_plane > projection.near_plane);

    const float vertical_scale = 1.0F / std::tan(projection.vertical_field_of_view_radians * 0.5F);
    const float depth_scale = projection.far_plane / (projection.far_plane - projection.near_plane);

    Mat4 result{};
    result.at(0, 0) = vertical_scale / projection.aspect_ratio;
    result.at(1, 1) = vertical_scale;
    result.at(2, 2) = depth_scale;
    result.at(2, 3) = -projection.near_plane * depth_scale;
    result.at(3, 2) = 1.0F;
    return result;
}

} // namespace hover::math
