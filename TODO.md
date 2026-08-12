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

- [ ] Move engine socket placement into ship visual data when a second ship or a
  Blender-authored mesh needs the effect; do not build a general attachment system
  before then.

## Ship visual polish

- [ ] Replace or substantially improve Prototype 01's provisional low-poly driver
  silhouette. Owner feedback on 2026-08-12 was that the layered exhaust and canopy
  looked much better, but the driver itself did not look good. Revisit it alongside
  authored ship work rather than delaying handling experiments now.

## Dynamic race portraits

- [ ] Prototype the player portrait plus the current first-through-fifth-place
  portraits, without duplicating the player when the player occupies one of those
  positions.
- [ ] Drive expressions from explicit race and vehicle presentation events such
  as boost, sustained high speed, impact, critical damage, elimination, and
  finishing result.
- [ ] Prototype aligned transparent PNG layers for pilot, expression, cockpit
  background, and transient fire/spark effects. Test 32-by-32 prominent portraits
  and 24-by-24 secondary portraits as a deliberately pixelated starting point,
  not a fixed requirement. Retain lossless working sources, compare larger tiers
  at 1080p, 1440p, 4K, and Steam Deck, and keep the dimensions changeable.
- [ ] Add a HUD-size setting when the settings UI exists. Keep HUD layout scale,
  display resolution, and portrait asset resolution independent so a large HUD
  does not merely expose an unnecessarily blurry 32-by-32 source.
- [ ] Pack portrait layers into resident atlases before a race. Runtime animation
  must select UV regions and transforms rather than decode or load PNG files per
  frame.
- [ ] Animate eliminated portraits shrinking to zero, smoothly reflow surviving
  entries into their new rank-sized rectangles, and admit the next eligible
  leader without losing stable racer identity during the transition.
- [ ] Keep critical damage and race position readable elsewhere in the HUD so the
  portraits are expressive feedback rather than the sole accessibility channel.

## Handling follow-up

- [ ] Replace hard ride-height attachment with an explicit hover/suspension
  response when vertical feel becomes the active experiment. Define spring,
  damping, surface attraction, loss-of-contact criteria, and how these behave on
  banks, vertical sections, and inverted loops without assuming world gravity is
  the surface normal.
- [ ] Add airborne/contact modes to the authoritative world-space racer state,
  with gravity, takeoff momentum, limited airborne control, landing eligibility,
  impact response, and path-reference refresh. A jump must be able to land on a
  different shortcut or branch without changing position representation.
- [ ] Define per-edge behavior instead of the provisional width clamp: solid
  wall, guard rail, open/fall edge, soft boundary, or another explicit course
  property. Add wall collision normals, bounce/scrape response, speed loss,
  damage, and controller/camera feedback when that experiment begins.
- [ ] Model vehicle/vehicle collision, relative mass, impact damage, explosions,
  and elimination using ship definitions rather than render-mesh dimensions.
- [ ] Decide how track curvature, banking, slope, grip, and lift-off throttle
  affect cornering authority and effective speed. Do not add fake centrifugal or
  slope modifiers without a controllable playtest and deterministic tests.
- [ ] Add surface and zone data for boost/recharge strips, grip changes, damage,
  traps, checkpoints, jump launch regions, and recovery hints. Keep zones in
  course data and consume them through shared player/AI simulation.
- [ ] Build explicit course-path transitions and topology for lane splits,
  shortcuts, jump-only branches, joins, and route-relative race progress when the
  first split-track scenario becomes active.
- [ ] Replace Prototype 01's constant passive coasting deceleration with a tuned
  speed-sensitive curve. Owner intent: releasing acceleration should change speed
  differently across the range and should help the ship take a sharper line than
  holding acceleration. Define the relationship with track-relative grip/turning
  rather than treating it as only a larger drag constant.
- [ ] Design the final boost resource rules, duration/cooldown or energy cost, and
  balancing after the button-activated burst prototype has established useful
  duration, acceleration, and excess-speed-return values.
