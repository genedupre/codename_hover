# Development log

Use this file for dated build and playtest observations. Keep durable product and
architecture decisions in `docs/decisions.md` and the relevant topical document.

### 2026-08-12 — source-shaped grounded propulsion and sustained slip

Build/revision: pending changes after `73a2788`, building on the handling
measurement foundation below.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS; native Wayland,
Vulkan SDL_GPU backend, debug validation enabled. Compilation, automated
verification, and a technical interactive smoke test completed locally;
the owner subsequently playtested both handling scenarios.

Track/scenario and settings: `handling_lab` and `speedway_physics`; 120 Hz
production simulation. Scalar scenarios retain their previous longitudinal model.

Good:

- World velocity is decomposed and exponentially damped in physical forward,
  lateral, and normal axes using elapsed-time coefficients.
- Positive propulsion now follows a speed curve and stored response. Measured
  direction change, drift-force fraction, and persistent slip intensity reduce
  its target.
- Lateral slip above 8 m/s builds over 1.6667 seconds and releases over 0.75
  seconds after grip recovers. Full response halves available propulsion.
- The previous constant drift deceleration and proportional slip drag were
  removed; ordinary world coasting now comes from forward damping.
- Boost lifecycle/events and turn roll remain shared with scalar movement, while
  world braking and excess-speed return remain explicitly owned by world physics.
- Deterministic scripts pass, including equal-duration 60/120 Hz damping and
  propulsion comparison. All 11 test executables pass.
- The graphical handling lab opened with the wired Steam Controller, accepted
  throttle, steering, shoulder drift, brake, and repeated boost input, emitted
  live telemetry, and shut down cleanly. Sustained-slip intensity visibly built
  and recovered in the log.
- The owner described the result as looking good overall.

Needs work:

- It still does not feel sufficiently like the intended F-Zero X reference. The
  owner can boost repeatedly and take corners without fearing a consequential
  washout or drift-out. Boost-speed corner entry, loss of directional authority,
  and recovery consequences therefore remain the next handling problem; current
  values are not accepted final tuning.
- Exact ride height and the wide safety edge remain temporary; hover and real
  wall/open-edge behavior have not started.

Measurements:

- After startup/capture stalls, ordinary one-second debug-validation samples were
  generally about 124–135 FPS at the current 1280×720 window. This is a smoke
  observation, not a shipping performance claim.

Next experiment:

- Use the recorded traces to identify why boost-speed cornering regains grip too
  safely, then change one relationship at a time before another A/B playtest.

### 2026-08-12 — handling measurement foundation

Build/revision: pending changes after `73a2788`.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS. Compilation and all
automated tests completed locally; the new graphical scenario is not yet
interactively verified.

Track/scenario and settings: new `--scenario handling_lab`; generated flat oval
with 6 km straights, 1 km turn radius, 800 m half-width, and the authoritative
120 Hz world-space simulation.

Good:

- Added a source-to-project ledger with evidence, inferred responsibilities,
  confidence, unit caveats, and the intended owner for each grounded-handling
  concept.
- Split the existing fixed tick into named steering, drift, propulsion,
  grounded-force, and supported-contact stages without intentionally changing
  its equations.
- Each completed tick exposes observational telemetry for local velocity, slip,
  steering direction change, drift/traction, requested/applied propulsion,
  handling loss, and temporary edge correction.
- The handling lab adds selected values to the existing one-second diagnostics.
- Deterministic scripts cover straight acceleration, full-speed steering,
  boost-turn, drift entry/sustain/release, coasting while sliding, and braking
  during boosted drift. Repeated executions compare equal tick by tick.
- All 11 test executables pass.

Needs work:

- The lab surface and its live log still need an interactive laptop smoke test.
- Local-axis damping, speed-dependent propulsion, sustained-slip state, and a
  60/120 Hz reconstructed reference comparison remain the next physics work.
- The current proportional slip-loss heuristic, exact ride height, and wide edge
  clamp remain temporary and must not be mistaken for final behavior.

Next experiment:

- Run `./build/development/codename_hover --scenario handling_lab`, build speed,
  then compare full-speed steering, boost-turn, drift entry, and drift release
  while observing lateral speed and slip angle in the one-second log.

### 2026-08-12 — boosted cornering grip follow-up

Owner feedback: Prototype 01 could boost and corner without enough visible drift
or loss of control. The world-space model already separated heading from momentum
and removed only a bounded amount of lateral speed, but the 420 m/s² normal-grip
tune and temporary safe edge constraint masked that behavior.

Normal world-space grip is now 300 m/s². A deterministic test verifies that the
same steering input is planted at low speed, slips at normal maximum speed, and
slips further at boosted maximum speed. Qualitative handling remains pending
another owner playtest; the temporary edge clamp still prevents the final wall
impact or fall-off consequence.

Follow-up source inspection found that lowering grip alone omitted several
coupled behaviors from F-Zero X's racer update. Its shoulder side force fades as
same-direction slide builds, shoulder input selects lower traction, and both
steering direction-change and drift force reduce engine acceleration. The
Codename Hover translation now:

- fades directional force by 32 m/s same-direction lateral speed;
- suppresses positive propulsion by steering and current drift-force strength;
- applies 60 m/s² forward drift loss;
- converts sustained slip beyond 8 m/s into proportional forward loss.

Deterministic coverage verifies force fade, drift slowdown under full throttle,
and accumulated maximum-speed cornering loss. Interactive feel is pending owner
playtest.

### 2026-08-12 — first world-space Speedway physics checkpoint

Build/revision: pending changes after `15ad089`.

Hardware and OS: HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS; native Wayland,
Vulkan SDL_GPU backend, debug validation enabled.

Input device: wired Steam Controller plus virtual SDL gamepad tests and
deterministic semantic input.

Track/scenario and settings: new `--scenario speedway_physics`; existing banked
Speedway geometry, 120 Hz fixed simulation, temporary exact ride height and
collider-aware road-edge safety constraint.

Good:

- World position, velocity, and physical orientation now drive the comparison
  scenario; bounded course projection derives progress after candidate motion.
- Steering rotates physical orientation without directly rotating momentum.
- Normal grip removes lateral velocity by a bounded per-second amount.
- Q/LB/L1 and E/RB/R1 apply equal opposite directional drift forces; both held
  cancel drift and use normal grip.
- Seam projection, banking, boost/braking integration, temporary edge response,
  and tested 24–360 Hz render schedules have deterministic coverage.
- The scalar `speedway` remains available for direct comparison.
- The real scenario opened, accepted steering/throttle/brake/boost input over an
  extended run, and shut down cleanly without a reported validation error.

Needs work:

- Owner qualitative feedback is not yet recorded for steering inertia, drift
  strength, grip, speed loss, banking, camera behavior, seam crossing, or the
  temporary edge response. Current frame diagnostics also do not print the two
  drift actions, so their live use cannot be confirmed from captured logs alone.
- Gravity, hover spring/damping, wall/open-edge policy, airborne motion, and jumps
  are deliberately not part of this checkpoint.

Measurements:

- During the extended 1280×720 debug-validation run, most one-second presentation
  samples were approximately 135–143 FPS. This is a smoke observation, not a
  shipping performance result or a claim about game feel.

Next experiment:

- Run `./build/development/codename_hover --scenario speedway_physics` on the
  laptop and compare it directly with `--scenario speedway`.

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

### 2026-08-12 — track-relative vehicle state boundary

Build/revision: `985d6e4` plus the track-vehicle-state change.

Hardware and OS: Deterministic host tests on the HP ZBook Studio 16 G11, Ubuntu
24.04.4 LTS.

Input device: Not applicable; this checkpoint introduces state and validation but
does not change interactive movement.

Track/scenario and settings: No scenario behavior changed. `runway` and `oval`
still use the provisional free-planar vehicle simulation.

Good:

- `TrackVehicleState` separates path-local position and velocity from the existing
  runway's world-space pose.
- A nonzero opaque `TrackPathId` leaves future splits, shortcuts, and joins able
  to transition between paths without embedding geometry ownership in the ship.
- Lateral values are relative to track-right and normal values are relative to
  the sampled surface, avoiding a world-up assumption on banks, loops, and
  inverted segments.
- Validation rejects unassigned paths, negative canonical distance/speed/normal
  offset, and non-finite values while retaining signed lateral and normal motion.
- All seven test executables pass and focused clang-tidy is clean.

Needs work:

- The state is not yet advanced by simulation or converted to a rendered world
  pose.
- A path graph, branch selection, authored 3D frame transport, airborne state,
  and path reacquisition remain deliberately unimplemented.

Next experiment:

- Advance a `TrackVehicleState` along the oval centerline using throttle, brake,
  and seam wrapping, then derive the rendered pose from its sampled frame. Keep
  steering disabled for that first integration and preserve `runway` unchanged.

### 2026-08-12 — acceleration engine pulse

Build/revision: `985d6e4` plus the track-state and engine-pulse changes.

Hardware and OS: Automated tests and graphical launch on the HP ZBook Studio 16
G11, Ubuntu 24.04.4 LTS, Wayland and Vulkan.

Input device: Wired Steam Controller detected; no specific interactive pulse
feedback was recorded before accepting this checkpoint.

Track/scenario and settings: `--scenario runway`; Prototype 01's existing
free-planar simulation and follow camera.

Good:

- A separate 12-triangle plume is drawn at each rear engine during positive net
  propulsion input and hidden while idle, coasting, or net braking.
- Throttle strength controls the base scale while two presentation-time
  frequencies add a slightly irregular pulse.
- Variable presentation time never feeds back into fixed-step simulation.
- Boost is explicitly reserved for a different, more extreme presentation rather
  than only scaling this effect up.
- Eight test executables pass and focused clang-tidy is clean.

Needs work:

- Pulse silhouette, size, colors, and rhythm have not received explicit owner
  visual feedback and remain provisional.
- Engine sockets are Prototype 01-specific draw constants until a second ship or
  Blender-authored asset demonstrates the reusable data boundary.
- The current opaque pipeline cannot provide additive glow, transparency, or
  bloom; those are not required for this checkpoint.

Next experiment:

- Continue with oval centerline simulation. Revisit pulse art direction when
  visual feedback or boost behavior provides a concrete reason.

### 2026-08-12 — layered exhaust and visible driver

Build/revision: `2332651` plus the layered-transparency presentation changes.

Hardware and OS: Automated tests and graphical launch on the HP ZBook Studio 16
G11, Ubuntu 24.04.4 LTS, Wayland and Vulkan.

Input device: Wired Steam Controller. The owner drove the runway and described
the first light-blue layered exhaust revision as looking cool.

Track/scenario and settings: `--scenario runway`; decorated 1280x720 window,
Prototype 01's free-planar simulation and follow camera.

Good:

- The accepted exhaust direction uses an opaque light-blue core and a larger
  50%-transparent shell on a dedicated unlit blended pipeline.
- This revision makes both plume width and length respond much more strongly to
  normalized speed, especially near maximum speed.
- A frame-rate-independent release envelope shrinks the plume over about 0.2
  seconds instead of switching it off on the first frame without acceleration.
- Canopy glass is isolated in a lit 50%-transparent mesh and drawn after an opaque
  low-poly driver silhouette, hull, and world.
- All eight test executables pass and focused clang-tidy is clean.

Needs work:

- The stronger speed response, release timing, glass color, and driver silhouette
  still need explicit visual feedback.
- Transparent faces use a deliberately small fixed draw order suitable for this
  ship; a general transparent-object sorter is not justified yet.

Next experiment:

- Inspect the revised ship in `runway`, then return to advancing track-relative
  state along the oval unless the new presentation has a concrete visual defect.

### 2026-08-12 — faster handling and speed-scaled turn roll

Build/revision: `ec78180` plus the first runway handling-tune changes.

Hardware and OS: Automated tests and interactive graphical test on the HP ZBook
Studio 16 G11, Ubuntu 24.04.4 LTS, Wayland and Vulkan.

Input device: Wired Steam Controller with analog steering, A/South acceleration,
and the existing brake controls.

Track/scenario and settings: `--scenario runway`; fixed 90 Hz simulation and
interpolated rendering in the decorated 1280x720 development window.

Good:

- Coasting deceleration increased from 12 to 24 m/s² and the maximum steering
  rate increased from 1.65 to 1.90 rad/s.
- A ship-specific visual roll now follows steering direction, scales with speed
  up to 0.18 radians, and eases back to level without changing planar travel.
- The owner tested the complete tune from rest through maximum speed and reported
  that it feels good.
- All eight test executables pass and focused clang-tidy is clean.

Needs work:

- Owner feedback requests another pass with more aggressive braking and coasting.
- Full-speed steering should remain as accepted, while low-speed steering needs
  more authority than the current linear speed curve provides.
- The provisional driver silhouette does not look good and remains deferred in
  `TODO.md` for later authored-visual work.

Next experiment:

- Increase braking and coasting modestly, then reshape steering authority so its
  low-speed floor is higher and it converges to the accepted full-speed response.

### 2026-08-12 — aggressive deceleration and low-speed steering

Build/revision: `0aa03c6` plus the second and third handling-tune passes.

Hardware and OS: Automated tests and interactive graphical tests on the HP ZBook
Studio 16 G11, Ubuntu 24.04.4 LTS, Wayland and Vulkan.

Input device: Wired Steam Controller.

Track/scenario and settings: `--scenario runway`; fixed 90 Hz simulation and
interpolated rendering in the decorated 1280x720 development window.

Good:

- Braking is now 180 m/s² and passive coasting deceleration is 90 m/s².
- Steering authority starts at 60% near rest, follows a square-root speed curve,
  and retains the previously accepted 1.90 rad/s full-speed rate.
- The owner tested the stronger values and reported that they look and feel
  better.
- The owner confirmed that eventual coasting must become speed-sensitive and
  should help a non-accelerating ship take a sharper line.

Needs work:

- Constant passive deceleration remains a temporary runway approximation.
- There is no lateral grip or track-relative cornering model yet, so the requested
  lift-off turning benefit is documented rather than faked through planar yaw.

Next experiment:

- Validate the first button-activated boost, full-speed vibration, and distinct
  exhaust flare before returning to oval centerline movement.

### 2026-08-12 — provisional boost and full-speed vibration

Build/revision: `0aa03c6` plus uncommitted boost/presentation changes.

Hardware and OS: Automated tests and interactive graphical run on the HP ZBook
Studio 16 G11, Ubuntu 24.04.4 LTS, Wayland and Vulkan.

Input device: Virtual SDL gamepad tests and the wired Steam Controller confirm
X/West boost and B/East brake.

Track/scenario and settings: `runway` is the intended visual/handling scenario.

Good:

- A rising-edge X/West press starts a one-second fixed-step boost burst with
  ship-owned baseline tuning, a provisional 1.28× ceiling, added acceleration,
  and deterministic return to normal speed. Holding X cannot retrigger the burst.
- Prototype 01's 260 m/s value is now explicitly named and documented as a base
  ship value rather than a universal game maximum.
- Boost has separate flare geometry plus faster, larger core/shell behavior.
- Full-speed vibration is a bounded local-space presentation offset that composes
  with visual turn roll and does not move the camera or simulation.
- The physical run reached about 1198 km/h from the current 936 km/h base, returned
  cleanly to base speed after X release, retained steering during boost, cancelled
  boost while braking, and exited cleanly.
- The owner accepted the boost and thruster presentation as looking good for now,
  and clarified that boost is a button activation rather than a held action.
- All eight test executables pass and focused clang-tidy is clean.

Needs work:

- Burst duration and balance remain provisional even though the current visual
  direction is accepted.
- Boost has no energy/cooldown or camera response yet; both remain deliberate
  follow-up work.

Next experiment:

- Verify that one X/West press produces one complete burst, holding cannot
  retrigger it, and release-then-press starts another burst.

### 2026-08-12 — speed-gated boost camera feedback

Build/revision: uncommitted camera-presentation changes after `fb0223c`.

Hardware and OS: Compilation and automated tests on the HP ZBook Studio 16 G11,
Ubuntu 24.04.4 LTS. Interactive graphical feel is not yet verified.

Track/scenario and settings: `runway` is the intended first test scenario.

Implemented:

- Camera feedback stays inactive during boost below 65% of normal maximum speed.
- Reaching the threshold during an active burst latches the response for that
  burst, so it cannot flicker if speed fluctuates around the boundary.
- The presentation envelope reaches full intensity in about 0.1 seconds and
  releases over about one third of a second while excess speed decays.
- Full response widens vertical FOV from 60° to 68°, increases follow distance
  from 8.5 m to 9.1 m, and increases look-ahead from 3.0 m to 3.5 m.
- Camera state changes no simulation values and advances independently of render
  frame rate.
- All nine automated test executables pass, including focused speed-gate, latch,
  release, frame-rate independence, and invalid-input coverage.

Needs interactive verification:

- Confirm the 65% onset feels intentional rather than late or early.
- Confirm the FOV, pullback, and look-ahead preserve steering readability.
- Check that release carries the speed sensation without lingering too long or
  causing discomfort.

### 2026-08-12 — throttle-gated boost acceleration and activation rumble

Build/revision: uncommitted changes after `fa9c0ef`.

Hardware and OS: Compilation and automated tests on the HP ZBook Studio 16 G11,
Ubuntu 24.04.4 LTS. Interactive handling and physical rumble are not yet verified.

Track/scenario and settings: `runway` is the intended first test scenario; wired
Steam Controller is the intended first rumble device.

Implemented:

- Boost acceleration now scales with the existing analog throttle action. At zero
  throttle it contributes no acceleration.
- Releasing throttle during an active burst applies ordinary coasting from the
  current boosted speed while the timer, flare, and eligible camera response
  continue.
- A successful rising boost edge emits one semantic `boost_activated` event.
- The single-player runtime maps that event to a 160 ms rumble pulse with 35%
  low-frequency and 80% high-frequency strength on capable attached controllers.
- Holding boost cannot repeat the pulse, and simultaneous braking cancels boost
  without producing the event.
- All nine automated test executables pass. Simulation coverage includes boost at
  rest without throttle, coasting from boosted speed, analog half-throttle boost,
  one-shot activation events, and brake suppression.

Needs interactive verification:

- Confirm releasing A/South while the burst is active produces the intended
  boosted-speed coast-down.
- Confirm the Steam Controller pulse is noticeable but not harsh or too long.

### 2026-08-12 — eligible boost rumble and shorter throttle-release tail

Build/revision: uncommitted changes after `f7dbbff`.

Hardware and OS: Compilation and automated tests on the HP ZBook Studio 16 G11,
Ubuntu 24.04.4 LTS. Interactive handling and physical rumble are not yet verified.

Track/scenario and settings: `runway` is the intended first test scenario; wired
Steam Controller is the intended first rumble device.

Implemented:

- A boost rising edge is eligible only with positive throttle and no brake.
  Ineligible presses start no timer and emit no rumble event; holding the rejected
  press while adding throttle does not bypass the rising-edge rule.
- Releasing throttle during an active Prototype 01 burst irreversibly caps its
  remaining time to 0.20 seconds while applying ordinary coasting deceleration.
- Once the short tail ends, boosted excess speed uses the existing stronger
  170 m/s² return rate.
- The 0.20-second release tail is explicit ship handling data and validated as no
  longer than the full boost duration.
- All nine automated test executables pass, including activation eligibility,
  one-shot feedback, release-tail duration, coasting, and analog throttle tests.

Needs interactive verification:

- Confirm no rumble occurs when X is pressed without A/South held or while
  braking.
- Confirm the 0.20-second tail feels short enough and the transition into excess
  speed decay feels continuous.

### 2026-08-12 — first map prototype: Oval Speedway

Build/revision: uncommitted map/scenario changes after `c5c7be2`.

Hardware and OS: Compilation and automated tests on the HP ZBook Studio 16 G11,
Ubuntu 24.04.4 LTS. Interactive graphics and driving are not yet verified.

Track/scenario and settings: new `speedway` scenario; 600-metre straights,
180-metre turn radius, 24-metre half-width, 512 centerline samples, 28-degree
maximum banking, and 85-metre bank transitions.

Implemented:

- Registered `speedway` separately from the unchanged `runway` sandbox and flat
  `oval` geometry reference.
- Reused the generic oval centerline and surface generator. Only the sampled
  normal/binormal frames are banked, keeping vehicle and mesh consumers generic.
- Both left turns lower the inside edge and raise the outside edge; smoothstep
  transitions leave the straights and start seam level.
- Added definition, frame-orientation, edge-height, mesh, and launch-option tests.
  All nine automated test executables pass.

Known limitation:

- At this checkpoint the vehicle remained free and world-planar. The owner
  subsequently accepted the standalone track surface and requested the attached
  driving implementation recorded below.

### 2026-08-12 — first track-attached driving model

Build/revision: uncommitted attachment changes after `c5c7be2` and the uncommitted
Oval Speedway scenario.

Hardware and OS: Compilation, focused clang-tidy, and deterministic host tests on
the HP ZBook Studio 16 G11, Ubuntu 24.04.4 LTS. Interactive attached handling is
not yet verified.

Track/scenario and settings: `oval` is the flat attachment reference; `speedway`
is the banked playtest map. Fixed simulation remains 90 Hz. Automated render
schedules cover 24, 60, 90, 120, 240, 244, and 360 Hz.

Implemented:

- Shared propulsion, braking, boost, event, and visual-roll dynamics between the
  unchanged free-planar `runway` and both attached scenarios.
- Advanced canonical path distance and speed-scaled lateral velocity from the
  existing semantic input boundary. Normal/drift grip controls lateral response.
- Replaced yaw-only world poses with orthonormal forward/up poses. Ship rendering
  and the follow camera now inherit sampled banking without assuming world-up.
- Added explicit Prototype 01 attached values: 42 m/s maximum lateral speed and
  0.62 m ride height.
- Provisionally constrained the ship's local box footprint inside each sampled
  road width. This is not a final invisible-wall or falling policy.
- Kept path geometry under scenario/course ownership. Per-vehicle state retains
  only an opaque path ID, leaving explicit future transitions for splits and
  joins and a separate airborne mode for jumps or cross-path landings.
- All ten automated test executables pass. Coverage includes spawn pose, ride
  height, seam wrapping, lateral motion, road limits, banked model orientation,
  and render-schedule independence. Focused clang-tidy reports no project-code
  findings.

Needs interactive verification:

- Confirm the ship remains visually above the road through straights, both bank
  transitions, both fully banked turns, and repeated seam crossings.
- Evaluate the provisional lateral speed and grip: responsiveness, drift lag,
  edge arrival, and whether steering feels like driving rather than sliding.
- Evaluate full camera banking for comfort. Horizon damping is possible later,
  but should not be added before observing the current surface-relative camera.
- Confirm boost, braking, exhaust, rumble, and camera boost feedback still behave
  as accepted while attached.

### 2026-08-12 — persistent heading and player-steered corners

Build/revision: uncommitted revision of the first attached-driving checkpoint.

Hardware and OS: Compilation and deterministic host tests on the HP ZBook Studio
16 G11, Ubuntu 24.04.4 LTS. Revised interactive handling is not yet verified.

Owner feedback:

- Following a curved track perfectly without steering feels like an autopilot;
  the player should need to steer through a corner or reach the outside wall.
- Rotating without acceleration visibly snapped back to the path direction; the
  ship's heading should persist instead.
- Deferred hover, gravity, slope, collision, wall/fall, jump, zone/trap, and
  course-graph mechanics should be captured explicitly in `TODO.md`.

Implemented:

- Added persistent signed heading in the local surface plane. Steering rotates
  it at the ship's existing speed-dependent steering rate, including at rest;
  releasing steering does not recenter it.
- Decomposed body-forward speed by heading into along-track and lateral targets.
  Existing normal/drift grip controls lateral response and the ship-specific
  lateral-speed limit remains explicit.
- Measured the sampled path's local world-distance scale at the selected lateral
  offset. Equal body speed therefore advances farther in centerline distance on
  a shorter inside lane than on a longer outside lane.
- Removed sampled tangent yaw from the relative heading after travel. Horizontal
  curves now rotate beneath an unsteered ship, carrying it to the outside edge;
  steering into the curve is required to hold a line. Surface-normal changes
  still supply automatic pitch/roll for banking and future loops.
- Added deterministic coverage for idle heading persistence, unsteered outside
  edge contact, steered cornering, inside/outside lane distance, and automatic
  surface pitch through a vertical-loop fixture.
- Recorded the intentionally missing mechanics in `TODO.md` without implementing
  speculative physics or course systems.

Needs interactive verification:

- Find the steering input needed to hold the centerline through Speedway's 180 m
  turns at low, normal-maximum, and boosted speeds.
- Confirm rotation no longer snaps back when throttle and steering are released.
- Assess whether the provisional 1.90 rad/s steering rate, 42 m/s lateral cap,
  normal/drift grip, and full camera banking work together or need separate
  tuning.
- The current outside-edge result is still a hard width clamp with no wall mesh,
  impact response, damage, or fall behavior; those are explicit deferred tasks.
