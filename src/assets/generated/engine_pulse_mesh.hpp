#pragma once

#include "render/mesh_data.hpp"

namespace hover::assets::generated {

// Two nested low-poly plumes authored around a local engine socket at the
// origin. Draw the transparent outer shell before the opaque core.
[[nodiscard]] render::MeshData make_engine_pulse_outer_mesh();
[[nodiscard]] render::MeshData make_engine_pulse_core_mesh();

} // namespace hover::assets::generated
