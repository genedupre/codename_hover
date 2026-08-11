#pragma once

#include "game/track.hpp"
#include "render/mesh_data.hpp"

namespace hover::assets::generated {

// Builds a simple three-band driving surface from the generic sampled-track
// boundary. The closing segment is included, so the mesh has no open seam.
[[nodiscard]] render::MeshData make_track_surface_mesh(const game::SampledTrack& track);

} // namespace hover::assets::generated
