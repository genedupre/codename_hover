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

### 2026-08-11 — complete laptop controller lifecycle

Build/revision: `bca563d`.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, native SDL Wayland,
Intel Arc Vulkan driver.

Input device: Wired Steam Controller with Steam running, plus physical keyboard.

Track/scenario and settings: Moving-ship runway build with live input values in the
window title and SDL device lifecycle logging.

Good:

- Y/North produced the expected rumble.
- SDL logged removal of gamepad instance 5 after unplugging.
- SDL opened the reconnected controller as instance 7, reported rumble capability,
  and accepted its connection pulse.
- Steering and A/South throttle resumed after reconnection.
- B/East followed the controller exit path and the game shut down cleanly.
- The owner confirmed every requested lifecycle check worked as expected.

Needs work:

- Repeat controller and lifecycle validation on Steam Deck when deployment begins.
- Test other controller families as representative hardware becomes available.

Measurements:

- No performance conclusion; this was a behavior and lifecycle verification.

Next experiment:

- Define a minimal sampled track frame and generate the first closed oval while
  preserving the fixed-step/input boundaries.

### 2026-08-11 — named development scenario entry point

Build/revision: `72fe297` plus the scenario-launch change.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS.

Input device: Not applicable to parser tests; the runway retains all previously
verified keyboard/mouse/controller behavior.

Track/scenario and settings: Command-line selection for small development worlds
through the main `codename_hover` executable.

Good:

- `--scenario runway` and `--scenario=runway` select a typed scenario while the
  no-argument launch continues to default to the runway.
- `--help` and `--list-scenarios` return before SDL initialization, making scenario
  discovery fast and usable in headless shells.
- Missing, unknown, duplicate, and unrelated arguments fail with clear messages.
- Focused automated tests cover default, named, informational, and invalid parsing.
- The runway remains available as a stable input/movement/camera sandbox instead
  of being replaced by the upcoming oval.
- The real `--scenario runway` launch logged the selected scenario, opened the
  native Wayland/Vulkan path with the Steam Controller, accepted steering and
  throttle, and shut down cleanly.
- The final controller Escape binding uses Select/Back/View. The owner confirmed
  that Start/Menu and B/East leave the runway running, while Select/Back/View exits
  cleanly; focused tests enforce all three cases.

Needs work:

- Only `runway` exists, so scenario-dependent world construction begins when the
  oval is implemented.
- The eventual normal game/menu launch needs a distinct default, but not during
  this prototype milestone.

Measurements:

- Help and scenario listing do not initialize SDL or open a window.

Next experiment:

- Add `oval` as the second scenario using a minimal sampled track frame and
  generated mesh while leaving `runway` unchanged.

### 2026-08-11 — sampled track and oval math

Build/revision: `44f03a7` plus the track-math change.

Hardware and OS: Deterministic host tests on the HP ZBook Studio 16 G11, Ubuntu
24.04.4 LTS.

Input device: Not applicable; this checkpoint has no interactive behavior.

Track/scenario and settings: A flat stadium oval generated as 512 evenly spaced
frames for focused tests. Test definition uses 100 m straights, 40 m centerline
turn radius, 12 m half-width, and 3 m elevation.

Good:

- The runtime boundary is a generic closed `SampledTrack`, not oval-specific
  vehicle or rendering code.
- Frames carry center, tangent, surface normal, track-right binormal, and width;
  validation enforces finite orthonormal frames and ordered distances.
- Sampling wraps positive and negative distances, interpolates the implicit seam,
  and re-orthonormalizes orientation.
- Six test executables pass. Track tests cover exact length, landmarks, frame
  quality, midpoint interpolation, seam continuity, wrapping, and local offsets.

Needs work:

- The oval is not registered as a scenario or rendered yet.
- The track is flat; banking, elevation changes, loops, corkscrews, and general
  frame transport remain intentionally unresolved.
- Vehicle simulation still uses free planar position and yaw.

Measurements:

- Test oval length: approximately 451.327 m from `200 + 80π`.
- Stored test frames: 512, with no duplicate end frame.

Next experiment:

- Register `oval`, generate a visible mesh from the sampled frames, and inspect the
  closed seam while preserving the runway scenario.

### 2026-08-11 — visible closed oval scenario

Build/revision: `44f03a7` plus the sampled-track and visible-oval changes.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS, Wayland, Vulkan with
development GPU validation enabled.

Input device: Wired Steam Controller detected by the shared SDL input path;
keyboard/mouse remained available.

Track/scenario and settings: `--scenario oval`; 600 m straights, 180 m centerline
turn radius, 24 m half-width, 512 samples, and a copper-colored first segment at
the implicit closed seam. The scenario uses the existing 90 Hz free-planar
simulation and follow camera.

Good:

- The owner confirmed that the generated oval looked good in the interactive
  laptop test.
- The world mesh is generated from generic `SampledTrack` frames, including the
  last-to-first closing segment; the renderer does not know the oval formula.
- `runway` remains the default separate scenario and continues to use the same
  runtime, renderer, simulation, and input paths.
- All six test executables pass and focused clang-tidy checks are clean.

Needs work:

- Vehicle movement in `oval` remains deliberately planar and can leave the track.
- The flat surface does not yet test banking, elevation, loops, or general 3D
  frame transport.
- Steam Deck deployment and runtime verification remain outstanding.

Measurements:

- Runtime oval length: approximately 2330.973 m (`1200 + 360π`).
- Surface mesh: 9,216 vertices and 9,216 indices across three bands and 512 closed
  segments.

Next experiment:

- Establish the first incremental Steam Deck deployment path, then introduce the
  smallest track-relative vehicle state while keeping `runway` unchanged.

### 2026-08-11 — first Steam Deck rsync deployment

Build/revision: `9edd632` plus the Deck deployment script and documentation.

Hardware and OS: Valve hardware platform `Galileo`, SteamOS 3.8.16 stable,
MicroSD card `SR01T` mounted at `/run/media/deck/SR01T`.

Input device: Not yet tested on the Deck.

Track/scenario and settings: Laptop development build deployed to
`/run/media/deck/SR01T/development/codename_hover`; graphical scenario not yet
started.

Good:

- The Deck was reachable through the `steamdeck` SSH host alias.
- The deployment script created the isolated MicroSD directory and incrementally
  copied the executable plus four compiled SPIR-V shaders without copying source
  files or touching `steamapps`.
- The deployed executable ran `--list-scenarios` directly on SteamOS and reported
  both `runway` and `oval`, proving basic executable and C++ runtime compatibility.
- The MicroSD mount is writable and executable and had approximately 210 GiB free
  during the audit.

Needs work:

- Graphical SDL/Wayland/Vulkan startup has not yet been tested on the Deck.
- Built-in controls, frame timing, clean exit, suspend/resume, and log retrieval
  remain unverified.

Measurements:

- Deployed executable size: approximately 12 MiB, unstripped development build.
- Deployed shaders: four SPIR-V files totaling approximately 20 KiB locally.

Next experiment:

- Launch `--scenario oval` from the Deck graphical desktop and record rendering,
  input, exit behavior, and any SDL/GPU diagnostics.

### 2026-08-11 — first graphical Steam Deck launch

Build/revision: `9edd632` plus the uncommitted Deck deployment tooling and
documentation.

Hardware and OS: Valve hardware platform `Galileo`, SteamOS 3.8.16 stable,
connected to a 2560x1440 TV.

Input device: Not specifically reported during this check.

Track/scenario and settings: Deployed `--scenario oval` launched manually from
the graphical desktop. The executable requests a resizable, high-density 1280x720
window. It renders directly to the acquired SDL_GPU swapchain and uses one sample
per pixel; borderless fullscreen, explicit native resolution, and MSAA are not yet
implemented.

Good:

- The owner confirmed that the graphical build ran and the oval looked good on
  the Deck.
- The laptop-build-to-MicroSD deployment path is now proven through actual
  graphical execution, not only a headless parser check.

Needs work:

- The owner perceived the displayed resolution as low.
- The executable does not log logical window size, pixel size, or acquired
  swapchain size, so the actual render dimensions from this run are unknown.
- The hardcoded 1280x720 window was displayed on a 2560x1440 TV, so it had one
  quarter of the TV's native pixel count before any compositor behavior;
  single-sample rasterization can make polygon edges look coarser still.
- Built-in input, clean exit, refresh rate, frame pacing, and suspend/resume were
  not specifically reported.

Deferred follow-up:

- Add a validated INI-backed display settings path, dimension logging, native
  borderless fullscreen, and optional MSAA at the display-settings milestone.
  The detailed checklist is in `TODO.md`; this does not replace the next
  track-relative gameplay checkpoint.

### 2026-08-11 — B/East full-brake binding

Build/revision: `9edd632` plus the pending Deck workflow and input-binding changes.

Hardware and OS: Automated host test on the HP ZBook Studio 16 G11, Ubuntu
24.04.4 LTS.

Input device: SDL virtual standard gamepad for the automated mapping test;
interactive Steam Controller and Deck verification pending.

Track/scenario and settings: Shared semantic input path used by both `runway` and
`oval`; no scenario-specific mapping.

Good:

- B/East now contributes 1.0 to the analog brake action and does not request exit.
- Left-trigger proportional braking and X/West full braking remain available.
- Keyboard/mouse and every controller still merge through the existing
  simultaneous-device policy.
- A virtual SDL gamepad test exercises the actual platform mapping and all six
  test executables pass; focused clang-tidy is clean.

Needs work:

- The new B/East mapping needs an interactive Deck or Steam Controller check.
- User-configurable mappings remain deferred. `TODO.md` records independent
  keyboard/mouse and controller configurations without changing the semantic
  action boundary.

Next experiment:

- Deploy the updated executable to the Deck and confirm B/East braking during the
  existing free-planar scenario.
