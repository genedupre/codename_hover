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

## CPU opponents and complex tracks

- [ ] Make CPU racers produce the same semantic vehicle actions and use the same
  track-relative/airborne simulation rules as the player. Avoid a separate
  simplified movement implementation that cannot survive later track features.
- [ ] Give AI path planning access to the future track graph so it can select
  branches, shortcuts, and rejoining paths rather than assuming one closed line.
- [ ] Represent jump intent, takeoff requirements, airborne control, landing
  targets, missed landings, and path reacquisition in AI planning.
- [ ] Separate lower-frequency route/behavior decisions from per-tick vehicle
  control so roughly thirty opponents remain measurable and manageable.
- [ ] Add deterministic scenario tests for splits, joins, jumps, recovery, and
  route choice before scaling the field from one to five, ten, and roughly thirty
  opponents.

## Engine presentation

- [ ] Give boost a deliberately different and more extreme engine presentation
  than normal acceleration, potentially changing plume geometry, color, rhythm,
  and supporting effects. Do not implement boost as only a larger normal pulse.
- [ ] Move engine socket placement into ship visual data when a second ship or a
  Blender-authored mesh needs the effect; do not build a general attachment system
  before then.
