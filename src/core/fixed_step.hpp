#pragma once

#include <cstdint>

namespace hover::core {

struct FixedStepConfig {
    double tick_seconds;
    double maximum_frame_seconds;
    std::uint32_t maximum_ticks_per_frame;
};

struct FixedStepPlan {
    std::uint32_t tick_count;
    float interpolation_alpha;
    bool dropped_time;
};

class FixedStepAccumulator final {
  public:
    explicit FixedStepAccumulator(FixedStepConfig config);

    [[nodiscard]] FixedStepPlan advance(double elapsed_seconds);
    void reset();

    [[nodiscard]] double tick_seconds() const { return config_.tick_seconds; }

  private:
    FixedStepConfig config_;
    double accumulator_seconds_ = 0.0;
};

} // namespace hover::core
