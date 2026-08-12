#pragma once

namespace hover::render {

struct EnginePulseSample {
    bool visible;
    float radial_scale;
    float length_scale;
};

// Presentation-only release envelope. Ignition is immediate; reducing propulsion
// shrinks the pulse briefly before it disappears.
[[nodiscard]] float advance_engine_pulse_intensity(float current_intensity,
                                                   float requested_intensity,
                                                   double elapsed_seconds);

// Cosmetic animation only: elapsed time may be variable because this value never
// feeds simulation. Propulsion intensity and speed ratio are clamped to [0, 1].
[[nodiscard]] EnginePulseSample sample_engine_pulse(double elapsed_seconds,
                                                    float propulsion_intensity, float speed_ratio);

} // namespace hover::render
