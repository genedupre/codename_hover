#pragma once

namespace hover::render {

struct EnginePulseSample {
    bool visible;
    float uniform_scale;
};

// Cosmetic animation only: elapsed time may be variable because this value never
// feeds simulation. Propulsion intensity is clamped to [0, 1].
[[nodiscard]] EnginePulseSample sample_engine_pulse(double elapsed_seconds,
                                                    float propulsion_intensity);

} // namespace hover::render
