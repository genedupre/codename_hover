#pragma once

#include <array>

namespace hover::math {

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vec4 {
    float x;
    float y;
    float z;
    float w;
};

struct Mat4 {
    std::array<float, 16> elements{};

    [[nodiscard]] float& at(int row, int column);
    [[nodiscard]] const float& at(int row, int column) const;
};

struct LookAt {
    Vec3 eye;
    Vec3 target;
    Vec3 world_up;
};

struct Perspective {
    float vertical_field_of_view_radians;
    float aspect_ratio;
    float near_plane;
    float far_plane;
};

[[nodiscard]] Vec3 operator-(Vec3 left, Vec3 right);
[[nodiscard]] Vec3 operator+(Vec3 left, Vec3 right);
[[nodiscard]] Vec3 operator*(Vec3 vector, float scalar);
[[nodiscard]] float dot(Vec3 left, Vec3 right);
[[nodiscard]] Vec3 cross(Vec3 left, Vec3 right);
[[nodiscard]] Vec3 normalized(Vec3 value);

[[nodiscard]] Mat4 identity();
[[nodiscard]] Mat4 translation(Vec3 offset);
[[nodiscard]] Mat4 scaling(float uniform_scale);
[[nodiscard]] Mat4 scaling(Vec3 scale);
[[nodiscard]] Mat4 rotation_y(float radians);
[[nodiscard]] Mat4 rotation_z(float radians);
[[nodiscard]] Mat4 operator*(const Mat4& left, const Mat4& right);
[[nodiscard]] Vec4 transform(const Mat4& matrix, Vec4 vector);

[[nodiscard]] Mat4 look_at_lh(const LookAt& camera);
[[nodiscard]] Mat4 perspective_lh(const Perspective& projection);

} // namespace hover::math
