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
