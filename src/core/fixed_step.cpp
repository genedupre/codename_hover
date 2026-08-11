#include "core/fixed_step.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace hover::core {

FixedStepAccumulator::FixedStepAccumulator(FixedStepConfig config) : config_(config) {
    assert(config_.tick_seconds > 0.0);
    assert(config_.maximum_frame_seconds >= config_.tick_seconds);
    assert(config_.maximum_ticks_per_frame > 0U);
}

FixedStepPlan FixedStepAccumulator::advance(double elapsed_seconds) {
    constexpr double timing_epsilon = 1.0e-9;
    accumulator_seconds_ += std::clamp(elapsed_seconds, 0.0, config_.maximum_frame_seconds);

    std::uint32_t tick_count = 0;
    while (accumulator_seconds_ + timing_epsilon >= config_.tick_seconds &&
           tick_count < config_.maximum_ticks_per_frame) {
        accumulator_seconds_ -= config_.tick_seconds;
        accumulator_seconds_ = std::max(0.0, accumulator_seconds_);
        ++tick_count;
    }

    bool dropped_time = false;
    if (accumulator_seconds_ >= config_.tick_seconds) {
        accumulator_seconds_ = std::fmod(accumulator_seconds_, config_.tick_seconds);
        dropped_time = true;
    }

    return FixedStepPlan{
        tick_count,
        static_cast<float>(accumulator_seconds_ / config_.tick_seconds),
        dropped_time,
    };
}

void FixedStepAccumulator::reset() { accumulator_seconds_ = 0.0; }

} // namespace hover::core
