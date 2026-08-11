# Project TODO

This is the short deferred-work backlog. `docs/roadmap.md` remains authoritative
for implementation order; an item here should not interrupt the active playable
checkpoint unless new evidence makes it urgent.

## Display settings and persistence

- [ ] Add a small, human-readable INI settings file when display settings become
  an active milestone. Keep parsing and validation game-specific rather than
  introducing a general configuration framework.
- [ ] Persist display selection, current/native or explicit resolution, windowed
  or borderless-fullscreen mode, VSync/present mode, frame limit, and MSAA.
- [ ] Log logical window size, pixel size, swapchain size, selected display, and
  refresh rate so display problems can be diagnosed from captured output.
- [ ] Reject or safely fall back from unavailable displays, modes, resolutions,
  refresh rates, and malformed INI values.
- [ ] Validate settings on the laptop, the Deck's internal display, and an
  external TV/monitor before treating them as complete.

Context: the first Deck graphical test used the prototype's hardcoded 1280x720
window on a 2560x1440 TV, so the low-resolution appearance was expected. There is
no intentional low-resolution internal renderer yet.

## Input remapping

- [ ] Add configurable keyboard and mouse bindings when the settings/menu work
  reaches an active milestone.
- [ ] Add controller bindings as a separate configuration from keyboard and
  mouse. Do not force both device classes to share one binding table.
- [ ] Preserve simultaneous keyboard/mouse and controller input after remapping;
  rebinding one device class must not disable or rewrite the other.
- [ ] Support analog actions such as steering, throttle, and brake without
  reducing them to boolean bindings merely because a keyboard key or face button
  contributes 100%.
- [ ] Define conflict handling, reset-to-default behavior, controller-position
  labels/glyphs, hotplug behavior, and safe fallback for malformed settings.
