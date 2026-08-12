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

Prototype 01 has two nested low-poly plume meshes drawn at each rear engine
socket: an opaque light-blue core and a larger light-blue shell with 50% vertex
opacity. Positive propulsion input makes them visible. Releasing propulsion uses
a short presentation-only envelope that shrinks the plume to almost nothing over
about 0.2 seconds before hiding it; sustained idle and braking remain plume-free.
Normalized forward speed strongly controls both length and width, with a
deliberately pronounced change near top speed, and also controls pulse frequency.
Two elapsed-time frequencies add irregular life without feeding variable frame
time into deterministic simulation.

The plumes use a small unlit alpha-blended pipeline so their color stays vivid.
It depth-tests against the opaque scene but does not write depth; the translucent
outer shell is drawn before the core. Opaque track and ship drawing remains on its
original depth-writing pipeline. Engine socket positions live beside the
Prototype 01 draw until another ship or authored asset proves a reusable
attachment boundary is needed. Boost will later use a visibly different, more
extreme presentation rather than merely increasing this pulse.

The first boost presentation now combines faster plume rhythm, a substantially
wider and longer core/shell response, and a separate translucent cyan diamond
flare at each engine. The extra geometry is only drawn while fixed-step vehicle
state reports active boost, making boost visually distinct from ordinary
acceleration. The timed boost flare remains active after throttle release even
though vehicle acceleration stops and coasting begins; it communicates the
remaining burst state rather than claiming that throttle is still pressed.

The boost camera response is a render-time envelope driven by fixed-step boost
and speed state. It stays off below 65% of the ship's normal maximum speed. Once
that threshold is reached during a burst, it remains latched until the burst ends
even if speed briefly falls, then eases out rather than snapping back. At full
intensity the follow distance increases from 8.5 m to 9.1 m, look-ahead from 3.0 m
to 3.5 m, and vertical FOV from 60° to 68°. The smooth envelope is independent of
render frame rate and never changes vehicle simulation. All values remain
provisional until interactive handling tests confirm the effect supports speed
without weakening steering readability or causing discomfort.

At 97% of normal maximum speed, a smooth presentation envelope begins applying a
very small multi-frequency local-space offset to the complete ship transform. It
reaches at most 12 mm laterally, 8 mm vertically, and 3 mm longitudinally, then
saturates through boost speed. Because the vibration is composed after the
interpolated yaw and visual turn roll, the hull, canopy, driver, and engines move
together while steering lean remains intact; neither simulation nor camera moves.

Prototype 01's canopy glass is a separate 50%-transparent, lit mesh. The opaque
hull and low-poly driver silhouette draw first and write depth; the canopy then
draws through the alpha-blended, non-depth-writing 3D pipeline. Keeping glass out
of the opaque ship mesh avoids making the entire vehicle translucent and leaves a
clean asset boundary for a future authored ship.

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

Dynamic pilot portraits deliberately use low-resolution authored art even in the
crisp modern display mode. Keep portrait texel resolution separate from its
screen rectangle: rank and relevance may resize portraits, eliminated portraits
may shrink away, and the complete HUD must still scale across modern outputs.
The 32-by-32 prototype reference is not a fixed resolution: test larger asset
tiers at 1440p and 4K, and eventually expose HUD size independently from output
resolution. Use nearest-neighbor sampling for the portrait artwork unless an
explicit later art-direction test chooses otherwise. See `pilot_portraits.md` for
the portrait atlas, animation, and reflow design.

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
