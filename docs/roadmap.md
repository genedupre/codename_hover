# Roadmap

## Roadmap policy

Advance through playable evidence, not subsystem count. Do not begin content
production or platform services while fundamental handling and deployment remain
unproven. Time estimates are planning ranges, not deadlines.

## Bootstrap execution checklist

This checklist preserves the agreed implementation order across development
sessions. Complete one step before expanding the next:

- [x] Audit the Linux laptop: OS, CPU/GPU, display session, Vulkan, controller
  visibility, toolchain, storage, and repository state. See
  `environment_audit.md` for the 2026-07-27 result.
- [x] Verify current first-party SDL3, SDL_GPU, and SDL_shadercross guidance, then
  choose pinned compatible revisions and the minimum host dependencies.
- [x] Scaffold the minimal C++23, CMake, and Ninja project plus its offline shader
  build path.
- [x] Implement the SDL3/SDL_GPU bootstrap executable incrementally through a
  rendered triangle, keyboard exit input, timing, and clear errors.
- [x] Validate the laptop graphics bootstrap locally with compiler warnings and
  GPU validation enabled.
- [x] Complete controller discovery, hotplugging, and an exit action. Startup
  detection, semantic input, capability-aware rumble, and simultaneous keyboard
  input were implemented and laptop-playtested with a wired Steam Controller on
  2026-08-11. Rumble, disconnect/reconnect, resumed input, and the then-current
  B/East exit were verified interactively on the same hardware. The exit action is
  now bound to Select/Back/View; focused and interactive tests confirm that it
  exits while Start/Menu and B/East remain free.
- [x] Establish and verify incremental deployment to Steam Deck. The first rsync
  payload passed its headless check and the owner launched the graphical oval on
  2026-08-11; broader Deck input/display/lifecycle validation remains below.
- [ ] Add initial Linux CI, then protect Windows portability with an early compile
  job.

Sequencing note (updated 2026-08-11): the owner is currently working only on the
laptop. The laptop controller checkpoint is complete. The Deck hardware and
MicroSD mount were audited, and the first rsync deployment plus headless executable
check succeeded. Interactive Deck graphics, built-in input, and exit verification
remain before the deployment checklist item is complete.

## Internal bootstrap: first triangle

Scope:

- configure C++23, CMake, Ninja, and pinned SDL dependencies;
- open an SDL window;
- create an SDL_GPU device and compatible shader pipeline;
- clear the screen and draw one colored triangle;
- handle Escape and a controller exit action;
- report useful FPS/frame-time diagnostics;
- build and run on the Linux laptop;
- deploy and run on Steam Deck.

Exit criteria: both devices run the same source revision, input exits cleanly, the
triangle renders without known validation errors, and the build/deploy steps are
documented and reproducible.

## Version 0.0.1: drivable prototype

Progress (2026-08-11): Prototype 01 can accelerate, brake, coast, and steer on a
generated runway through a provisional planar fixed-step simulation. Rendering
interpolates its model transform and uses a following camera. Keyboard, mouse, and
SDL gamepad actions can remain active simultaneously; a wired Steam Controller was
detected and used on the laptop. Its hotplug, resumed input, rumble, and exit paths
are verified. Track-relative movement and Steam Deck verification remain required.
The runway is retained as the named `runway` development scenario. The closed
`SampledTrack` boundary, deterministic flat oval generator, generic surface mesh,
and separate `oval` scenario are implemented and tested. The surface and closed
seam were visually verified on the laptop on 2026-08-11; track-relative vehicle
state is next.

Scope:

- a simple 3D camera and perspective projection;
- a code-generated low-poly vehicle;
- keyboard and controller movement;
- a generated oval or loop track;
- basic track-relative acceleration and steering;
- fixed-step simulation with interpolated rendering;
- a speed and timing debug overlay.

Exit criteria: one vehicle can be driven around one simple track on laptop and
Deck, with behavior unchanged by render frame rate.

## Version 0.0.2: make driving fun

Tune and validate acceleration, steering, grip, drift, banking, hover response,
boost, collisions, falling, recovery, and camera behavior. Add track features
needed to test banked curves, a loop, pipe-like surfaces, a corkscrew, and a jump.

Exit criterion: repeated owner playtests describe the prototype as enjoyable and
the remaining problems are specific tuning issues rather than a failed vehicle
model.

## Version 0.0.3: complete a race

Add start flow, checkpoints, laps, timing, race position, finish/restart, recovery,
a time-trial ghost, and a minimal HUD.

Exit criterion: a player can start, complete, invalidate, restart, and review one
race without developer intervention.

## Version 0.0.4: first opponent

Add one AI vehicle using track-space goals and the supported vehicle-control path.
Validate racing line, speed choice, recovery, and basic passing behavior.

Exit criterion: the AI reliably completes the test race and creates a useful
driving interaction.

## Version 0.0.5: race field

Scale in measured steps from five to ten to roughly thirty racers. Profile CPU and
GPU cost, then improve traffic, overtaking, collision avoidance, race ordering,
and recovery.

Exit criterion: a representative full field completes races with acceptable frame
pacing on stated laptop and Steam Deck settings.

## First vertical slice

Planning scope, subject to revision after handling validation:

- three original vehicles;
- three tracks;
- roughly thirty racers;
- time trial and one small championship;
- difficulty-appropriate AI;
- representative sound and music;
- menus, settings, and readable final-direction HUD;
- representative visual effects and environment art;
- initial leaderboard integration only if it adds value to external testing.

Exit criterion: strangers can understand, play, and evaluate a short polished
sample without explanation, producing useful evidence about fun, readability,
performance, and commercial scope.

## After the vertical slice

Only then commit to the content count, commercial PC release scope, Steam demo and
store work, achievements/cloud/leaderboards, macOS support, or console ports.
Online multiplayer, HDR, modding, and a player track editor remain separate future
decisions.

## Imported solo effort ranges

These broad ranges assume an experienced programmer and may be optimistic for a
first game. Re-estimate from actual project velocity:

| Result | Imported rough effort |
| --- | --- |
| Basic SDL_GPU framework | 1–3 weeks |
| Track and driving prototype | 1–2 months |
| Credible vertical slice | 3–6 months |
| Small polished game | 9–18 months |
| Content-rich commercial release | 12–24+ months |

The owner is new to professional game development and Blender, so milestone
evidence is more reliable than this table.
