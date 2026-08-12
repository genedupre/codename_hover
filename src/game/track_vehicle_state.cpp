#include "game/track_vehicle_state.hpp"

#include <cmath>

namespace hover::game {

bool is_valid(TrackPathId path) { return path.value != 0U; }

bool is_valid(const TrackVehicleState& state) {
    return is_valid(state.location.path) &&
           std::isfinite(state.location.distance_along_path_metres) &&
           state.location.distance_along_path_metres >= 0.0F &&
           std::isfinite(state.location.lateral_offset_metres) && is_valid(state.vehicle) &&
           std::isfinite(state.heading_offset_radians) &&
           std::isfinite(state.lateral_velocity_metres_per_second) &&
           std::isfinite(state.normal_offset_metres) && state.normal_offset_metres >= 0.0F &&
           std::isfinite(state.normal_velocity_metres_per_second);
}

} // namespace hover::game
