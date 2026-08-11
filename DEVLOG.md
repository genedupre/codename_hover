# Development log

Use this file for dated build and playtest observations. Keep durable product and
architecture decisions in `docs/decisions.md` and the relevant topical document.

## Entry template

### YYYY-MM-DD — short description

Build/revision:

Hardware and OS:

Input device:

Track/scenario and settings:

Good:

- Observation.

Needs work:

- Specific observation, including when it occurs.

Measurements:

- Metric with units and test conditions.

Next experiment:

- One controlled change or question.

### 2026-08-10 — SDL window and event-loop smoke test

Build/revision: `8f3fb03` plus the pending window-loop change.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS.

Input device: Laptop keyboard and window controls; no game controller tested.

Track/scenario and settings: 1280x720 resizable high-density SDL window; automatic
video-driver selection followed by an explicit native Wayland launch.

Good:

- Automatic selection created the window through X11/XWayland and exited cleanly.
- `SDL_VIDEODRIVER=wayland` initialized the native Wayland window path and exited
  cleanly at the API level.
- Escape and the window close action both reach the same orderly shutdown path.
- An invalid video-driver selection fails immediately with a useful SDL error.
- SDL_GPU selected the Vulkan backend with development validation enabled and
  destroyed the device through the clean shutdown path without a reported
  validation error.
- An invalid GPU-driver selection fails immediately with a useful SDL error.

Needs work:

- Controller input is not implemented or tested yet.
- The GPU device is not claimed for the window yet, and no content is rendered or
  presented.
- The native Wayland test did not produce a visible window for the owner. The
  event-only checkpoint never attaches or presents a graphics buffer, so retest
  visibility after the first SDL_GPU clear/present rather than treating API-level
  window creation as visual verification.

Measurements:

- No frame-time or graphics-performance measurement applies to the event-only
  window loop.

Next experiment:

- Claim the SDL window for the GPU device, then clear and present its swapchain
  texture.

### 2026-08-10 — SDL_GPU clear and present smoke test

Build/revision: `8f3fb03` plus the pending window and clear/present changes.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, Intel Arc Vulkan
driver.

Input device: Laptop keyboard and window controls; no game controller tested.

Track/scenario and settings: 1280x720 resizable high-density window, Vulkan
SDL_GPU backend, debug validation enabled, default SDR/VSync swapchain. Tested
once with forced native Wayland and once with automatic X11/XWayland selection.

Good:

- Both window backends claimed an SDL_GPU swapchain, submitted repeated clear
  passes, presented, and exited cleanly.
- SDL reported no GPU validation error during either run.
- The render loop tolerates a temporarily unavailable swapchain texture without
  submitting invalid work or running an unrestricted busy loop.
- The owner confirmed the expected solid dark blue output was visible and the
  window resized normally during the interactive test.

Needs work:

- Controller input, frame timing, shader loading, and triangle drawing remain
  unimplemented.

Measurements:

- No performance claim is recorded; the default VSync swapchain and empty clear
  pass are only a presentation-path smoke test.

Next experiment:

- Load the compiled vertex and fragment shader assets, create the graphics
  pipeline for the active swapchain format, and draw three vertices.

### 2026-08-10 — first SDL_GPU triangle

Build/revision: `efe8ed9` plus the pending triangle change.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, Intel Arc Vulkan
driver.

Input device: Laptop keyboard and window controls; no game controller tested.

Track/scenario and settings: Build-generated SPIR-V vertex and fragment shaders,
three vertices generated from `SV_VertexID`, 1280x720 resizable high-density
window, Vulkan SDL_GPU backend, debug validation enabled. Tested through forced
native Wayland and automatic X11/XWayland.

Good:

- Both backends loaded the offline-compiled shader assets, created the graphics
  pipeline, rendered the colored triangle over the dark blue clear color, and
  exited cleanly.
- The owner confirmed the triangle looked correct during the interactive test.
- SDL reported no GPU validation error during either run.
- The runtime does not need a vertex buffer for this bootstrap triangle.
- A deliberately missing shader asset fails startup with the exact expected path
  and SDL filesystem error.

Needs work:

- Controller hotplugging and a controller exit action are not implemented.
- Frame-time and FPS diagnostics are not implemented.

Measurements:

- No performance claim is recorded; this scene only proves the shader and
  graphics-pipeline path.

Next experiment:

- Add controller discovery, hotplug handling, and a controller exit action while
  preserving simultaneous keyboard input.

### 2026-08-10 — presentation frame diagnostics

Build/revision: `efe8ed9` plus the pending triangle and timing changes.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, Intel Arc Vulkan
driver, internal 1920x1200 display at approximately 59.88 Hz.

Input device: Laptop keyboard and window controls; controller work explicitly
deferred until before Steam Deck testing.

Track/scenario and settings: Colored triangle, 1280x720 resizable high-density
window, X11/XWayland, Vulkan SDL_GPU backend, debug validation enabled, default
SDR/VSync swapchain.

Good:

- Once-per-second diagnostics report presented FPS plus average and worst frame
  interval in both the log and window title.
- Minimized or otherwise skipped swapchain frames reset the sample instead of
  manufacturing a large restore-time spike.
- The timing run exited cleanly without a reported GPU validation error.

Needs work:

- Controller input remains intentionally deferred.
- These CPU-side presentation-loop intervals do not separate CPU submission,
  GPU execution, and compositor/presentation latency.

Measurements:

- Stable samples were approximately 59.7-60.1 FPS with 16.64-16.75 ms average
  frame intervals under the default VSync swapchain.
- Worst intervals varied by sample and included approximately 17.7-26.8 ms in
  the otherwise steady portion of the run. Short interaction/resize periods
  produced different cadence and are not treated as performance results.
- This empty bootstrap scene is a diagnostics verification, not a game
  performance claim.

Next experiment:

- Commit the completed laptop triangle/timing checkpoint, then choose between
  controller work and the first Steam Deck deployment preparation. Controller
  support is required before calling the Deck bootstrap complete.

### 2026-08-10 — first stationary 3D scene

Build/revision: `26e4b44` plus the pending 3D scene change.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, Intel Arc Vulkan
driver.

Input device: Laptop keyboard and window controls; controller work remains
explicitly deferred until before Steam Deck testing.

Track/scenario and settings: Code-generated 11-vertex low-poly hovercraft above a
rectangular pad, fixed elevated perspective camera, resize-dependent projection,
D16 depth target, 1280x720 resizable high-density window, X11/XWayland, Vulkan
SDL_GPU backend, and debug validation enabled.

Good:

- The owner confirmed that the colorful hovercraft, dark pad, and elevated 3D
  perspective were all visible as intended.
- The owner confirmed the scene retained the expected appearance when the window
  was resized.
- Indexed vertex-buffer drawing, per-frame camera uniforms, and depth testing all
  worked together without a reported GPU validation error.
- Focused deterministic tests cover view placement, left-handed orientation,
  zero-to-one projection depth, identity transforms, and matrix composition.

Needs work:

- The vehicle and camera are stationary; this checkpoint intentionally contains
  no movement or simulation yet.
- The pad is only a depth and perspective reference, not the generated test
  track.
- Controller input and Steam Deck deployment remain deferred, not completed.

Measurements:

- No new performance claim is recorded. This run verifies the 3D rendering path,
  and the existing title diagnostics remained visible during the test.

Next experiment:

- Add a small keyboard input-state layer and move the vehicle freely in 3D before
  introducing track-relative constraints or fixed-step racing physics.

### 2026-08-11 — Prototype 01 ship and asset boundary

Build/revision: `4ccc91a` plus the pending Prototype 01 change.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, Intel Arc Vulkan
driver.

Input device: Laptop keyboard and window controls; the ship is stationary and no
driving input was tested.

Track/scenario and settings: Generated Prototype 01 above a separately uploaded
center-striped presentation pad, fixed elevated camera, flat directional lighting,
1280x720 resizable high-density X11/XWayland window, Vulkan SDL_GPU backend, and
debug validation enabled.

Good:

- The owner confirmed that the result was recognizably a ship.
- The generated ship, presentation pad, and generic 32-bit indexed `GpuMesh`
  uploads rendered together and exited cleanly.
- The 96-triangle ship has unit flat normals and fits completely inside its
  declared local collider according to deterministic tests.
- Gameplay definition data is now independent from mesh creation, allowing later
  generated or Blender/GLB visuals to use the same runtime upload path.

Needs work:

- The owner explicitly expects substantial visual improvement; the silhouette,
  colors, canopy, wings, engines, name, and proportions are not final art.
- Handling, drift, collider, mass, energy, and damage values exist as provisional
  data but have no simulation behavior yet.
- There is no per-object model transform, collider visualization, ship roster, or
  GLB loader.

Measurements:

- Prototype 01 uploads 288 face-duplicated vertices and 288 indices (96
  triangles). The presentation pad uploads 18 vertices and 18 indices.
- The long interactive development run varied broadly around 115-144 presented
  FPS with approximately 6.95-8.67 ms sampled averages and interaction-related
  worst-frame spikes. This stationary debug scene is not a representative
  performance benchmark or product claim.

Next experiment:

- Add fixed-step keyboard movement, an interpolated ship model transform, and a
  following camera while keeping the visual mesh and gameplay definition separate.

### 2026-08-11 — fixed-step movement and Steam Controller input

Build/revision: `41bf6ee` plus the A-button binding and documentation refinement.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, Intel Arc Vulkan
driver.

Input device: Laptop keyboard/mouse and wired Valve Steam Controller through
Steam's virtual gamepad path.

Track/scenario and settings: Prototype 01 on a generated 5.25 km presentation
runway, 90 Hz fixed planar simulation, interpolated model transform, following
camera, X11/XWayland window, Vulkan SDL_GPU backend, and debug validation enabled.

Good:

- SDL opened exactly one device named `Steam Controller`, classified it as a
  standard gamepad, reported rumble capability, and accepted the startup pulse.
- The controller and keyboard/mouse feed the same semantic action state and can be
  used simultaneously without summing their values.
- The owner described the moving result as already “pretty cool.”
- The owner confirmed the revised A/South-button throttle behavior. Runtime
  diagnostics showed full `1.0` throttle while the action remained analog-valued.
- Deterministic tests produce the same one-second speed and travel result across
  render schedules from 24 through 500 FPS, including 240 and 244 FPS.

Needs work:

- This is free planar runway movement, not track-relative hover handling.
- The follow camera, acceleration curve, steering response, and speed sensation
  are only plumbing defaults and need controlled playtesting.
- Rumble sensation, hot disconnect/reconnect, controller exit, and Steam Deck
  behavior have not been explicitly confirmed by the owner.

Measurements:

- Simulation tick: fixed 90 Hz (11.11 ms per tick).
- Prototype test schedules: 24, 30, 60, 90, 120, 144, 165, 240, 244, 360, and
  500 rendered frames per second; these are deterministic test inputs, not laptop
  performance measurements.

Next experiment:

- Interactively verify controller disconnect/reconnect and controller exit, then
  introduce a generated track frame without coupling physics to render FPS.

### 2026-08-11 — isolate Steam desktop-layout keyboard duplication

Build/revision: `75fdeff` plus the Linux video-backend preference.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, GNOME Wayland,
Intel Arc Vulkan driver.

Input device: Physical keyboard and wired Steam Controller with Steam running.

Track/scenario and settings: The same moving-ship runway build, compared through
SDL's X11/XWayland and native Wayland video backends.

Good:

- Event diagnostics proved that left-stick forward did not report A/South. At
  roughly half forward it caused a separate keyboard `W` down event, followed by
  `W` up as the stick returned.
- XWayland exposed the injected and real keys as one aggregate keyboard, confirming
  that a game-level device-name filter could not preserve genuine keyboard input.
- Under native Wayland, left-stick forward did not apply throttle; A/South still
  supplied `1.0` throttle, physical keyboard W still supplied `1.0` throttle, and
  the owner confirmed the behavior looked correct.
- A final launch without an environment override logged the `wayland` video driver,
  confirming that the code-selected `wayland,x11` preference takes effect. The
  normal build again kept stick-only steering at zero throttle and exited cleanly.

Needs work:

- X11 remains a required fallback. Steam Controller use through that fallback may
  need a per-game Steam layout without synthetic WASD bindings.
- The Steam Deck/Gamescope path still requires its own deployment test.

Measurements:

- No performance conclusion; this was an input-source isolation test.

Next experiment:

- Test controller hotplug and exit when convenient, then proceed toward the first
  generated track frame.
