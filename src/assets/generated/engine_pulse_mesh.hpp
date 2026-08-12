#pragma once

#include "render/mesh_data.hpp"

namespace hover::assets::generated {

// A single low-poly plume authored around a local engine socket at the origin.
// The renderer draws it once per engine using the ship's model transform.
[[nodiscard]] render::MeshData make_engine_pulse_mesh();

} // namespace hover::assets::generated
