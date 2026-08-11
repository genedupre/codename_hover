#pragma once

#include "game/ship_definition.hpp"

namespace hover::game::ships {

inline constexpr std::string_view prototype_01_mesh_id = "generated/prototype_01";

[[nodiscard]] const ShipDefinition& prototype_01_definition();

} // namespace hover::game::ships
