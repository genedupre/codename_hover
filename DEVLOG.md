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
