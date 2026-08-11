#pragma once

#include "render/mesh_data.hpp"

namespace hover::assets::generated {

struct Triangle {
    math::Vec3 first;
    math::Vec3 second;
    math::Vec3 third;
    math::Vec3 color;
};

struct Quad {
    math::Vec3 first;
    math::Vec3 second;
    math::Vec3 third;
    math::Vec3 fourth;
    math::Vec3 color;
};

class MeshBuilder final {
  public:
    void add_triangle(const Triangle& triangle);
    void add_quad(const Quad& quad);
    [[nodiscard]] render::MeshData build() &&;

  private:
    render::MeshData mesh_;
};

} // namespace hover::assets::generated
