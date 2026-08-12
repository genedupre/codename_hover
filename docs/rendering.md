# Rendering and display

## Visual direction

The game should look intentionally simple at modern output resolutions:

- low-polygon vehicles with strong silhouettes;
- simple track meshes and restrained low-resolution textures;
- vertex color and simple lighting where effective;
- sparse backgrounds and distant geometry;
- deliberate fog as composition and depth cue;
- very few material families and shaders.

Do not default to PBR, ray tracing, temporal upscaling, motion blur, or a large
post-processing stack. Add an effect only when it improves the game's identity or
driving readability enough to justify its complexity.

## Renderer scope

SDL_GPU is the graphics abstraction. HLSL is the preferred shader source language,
translated offline through SDL_shadercross into formats required by the active
backend. Runtime shader compilation should not be required for a normal build.

The early renderer needs only:

- device and swapchain setup;
- clear and present;
- vertex and index buffers;
- transforms and a perspective camera;
- one simple world pipeline;
- a minimal debug text or UI path;
- useful validation and failure messages.

Instancing, texture management, additional materials, and effects arrive when a
visible milestone needs them.

## Engine pulse presentation

Prototype 01 has a separate tiny low-poly plume mesh drawn at each rear engine
socket. Positive propulsion input makes it visible; idle, coasting, and net
braking hide it. Two elapsed-time frequencies modulate uniform scale so the pulse
has irregular life without feeding variable frame time into deterministic
simulation. Propulsion strength changes the base scale.

The plume currently reuses the simple opaque vertex-color pipeline. Engine socket
positions live beside the Prototype 01 draw until another ship or authored asset
proves a reusable attachment boundary is needed. Boost will later use a visibly
different, more extreme presentation rather than merely increasing this pulse.

## Display behavior

The long-term PC settings should support:

- current/native and explicitly selectable resolutions;
- windowed and borderless-fullscreen modes;
- exclusive fullscreen only if testing shows a useful platform benefit;
- VSync on/off and sensible frame-limit choices;
- high-density displays;
- automatic aspect handling, including common ultrawide ratios;
- configurable MSAA if it materially improves the simple geometry.

UI and camera layouts must use actual drawable pixel size and safe layout rules,
not assume 1920x1080 or 16:9.

## Resolution and frame-rate policy

Supporting 4K and 240 Hz means the game can select those display modes and behave
correctly. It does not mean guaranteeing 4K at 240 frames per second on every GPU.

Treat 240 FPS as an important target and future limiter choice, not a hard engine
maximum. Displays beyond 240 Hz already exist, so long-term settings should include
display-refresh/VSync behavior, common explicit limits (including 240), and an
uncapped option. The bootstrap remains on SDL_GPU's always-supported VSync present
mode until settings work can expose supported modes deliberately.

SDL_GPU distinguishes VSync, immediate, and mailbox presentation. VSync avoids
tearing but may add queue latency; immediate prioritizes latency but may tear;
mailbox can reduce queued visual latency without tearing where supported. Query
capabilities rather than assuming immediate or mailbox exists. Frames in flight
also trade throughput for latency and must be measured before changing the SDL
default.

The target art style makes high frame rates plausible on modern hardware, but all
performance statements must name:

- hardware and driver;
- operating system;
- resolution and display mode;
- graphics settings;
- scene/racer count;
- representative CPU and GPU frame-time measurements.

Frame pacing matters as much as an average FPS number. Keep simulation behavior
independent of refresh rate as described in `architecture.md`.

## Potential presentation modes

A crisp modern mode can render low-poly assets directly at the display resolution.
A later retro presentation mode could render at a lower internal resolution and
scale deliberately. These modes should share assets and simulation. Retro mode is
deferred until the base renderer and art direction are stable.

## Performance approach

- Establish budgets with representative content rather than speculative systems.
- Profile before optimizing.
- Track CPU simulation, render submission, GPU time, and presentation separately.
- Test both high-end/high-refresh desktop behavior and Steam Deck power limits.
- Avoid hiding frame-time spikes behind averages.

The internal triangle milestone proves the API and deployment path, not final
rendering performance. The first meaningful graphics benchmark is a generated
track with a moving vehicle and representative camera.

## Frame-rate evidence checked on 2026-08-11

- SDL present-mode behavior and capability requirements:
  https://wiki.libsdl.org/SDL3/SDL_GPUPresentMode
- SDL's explicit throughput/latency tradeoff for frames in flight:
  https://wiki.libsdl.org/SDL3/SDL_SetGPUAllowedFramesInFlight
- 2019 controlled 60/120/240 Hz targeting study reporting monotonic performance
  improvement with refresh rate:
  https://research.nvidia.com/publication/2019-09_esports-arms-race-latency-and-refresh-rate-competitive-gaming-tasks
- 2024 study finding visibly worse smoothness from sufficiently variable frame
  times across tested rates up to 240 Hz:
  https://research.nvidia.com/publication/2024-08_variable-frame-timing-affects-perception-smoothness-first-person-gaming
- Current evidence that 240 Hz is not a hardware ceiling (500 Hz display):
  https://news.samsung.com/us/samsung-launches-worlds-first-500hz-oled-gaming-monitor-and-new-odyssey-g7-lineup/
