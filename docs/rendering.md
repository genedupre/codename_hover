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
