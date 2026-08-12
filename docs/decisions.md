# Decisions and open questions

This file records project-level choices that affect more than one subsystem. A
decision can be changed when evidence justifies it; update the affected topical
documents at the same time. Do not treat imported research claims as accepted
facts merely because they appear in the planning documents.

## Accepted baseline decisions

### D-001: Build a game-specific runtime

Status: accepted.

Codename Hover is the product. Do not create or name a separate general-purpose
engine. Shared code exists only to serve the game.

### D-002: Use C++23, CMake, and Ninja

Status: accepted for bootstrap.

This matches the desired native, compact, cross-platform runtime and conventional
tooling. Reconsider only with concrete toolchain or platform evidence.

### D-003: Use SDL3 and SDL_GPU

Status: accepted for bootstrap.

SDL3 owns platform/window/input/audio concerns and SDL_GPU owns modern graphics API
abstraction. Direct per-API renderers are out of scope unless SDL_GPU fails a
measured shipping requirement.

### D-004: Develop Linux-first and test on Steam Deck

Status: accepted.

The laptop is the development machine; the Deck remains clean target hardware.
Native Linux is a product requirement. Windows follows early enough to protect
portability.

### D-005: Separate fixed simulation from rendering

Status: accepted.

Gameplay must not depend on rendered frame rate. The prototype simulation runs at
120 Hz and interpolates rendering, including when rendering is slower or faster
than simulation. Treat 120 Hz as a provisional tuning choice, not the Steam Deck's
refresh rate and not a rendering limit. Keep the rate in one shared code constant.

Render correctly at common low and high rates. Presentation and limiter policy is
recorded separately in D-015.

### D-006: Use specialized track-relative vehicle physics

Status: accepted as the first prototype approach.

Do not add a general rigid-body engine initially. Vehicles may transition to a
genuinely airborne state where required.

### D-007: Use generated prototype geometry, then Blender

Status: accepted.

Generate the triangle, first ship, and first track in code. Introduce Blender and
GLB/glTF after the driving path works. Do not build a custom editor.

### D-008: Preserve an original identity

Status: accepted and non-negotiable.

The visual era and broad genre may inspire the project, but protected Nintendo
content must not be copied.

### D-009: mod support
eventually, we should be able to have mod support (eg people can make changes to the game, add vechiles, maps)
how this would like, we can decide later.

### D-010: Pin stable SDL as a Git submodule

Status: accepted for bootstrap on 2026-07-27.

Use SDL 3.4.10 at commit `8e37db5e797b6167f3a00d697d816a684bd259c7`
under `external/SDL` and include it with CMake `add_subdirectory`. Build the static
SDL target and disable SDL tests and install targets for the game build.

Git submodules make the exact source visible and avoid downloading dependencies
during CMake configure. They also give a clearer future replacement point for
platform-specific SDL forks than a hard-coded FetchContent declaration.

### D-011: Use SDL_shadercross only as a pinned host tool

Status: accepted for bootstrap on 2026-07-27.

SDL_shadercross has no official releases or tags. Pin source commit
`e55cf5e31ced6f3d1be5cc6d0c50e99384f9f4ba` and use the official Linux x64
artifact from Actions run `28236415347` for that commit. The verified artifact and
executable hashes are recorded in `development-hardware.txt`. That pinned source
declares SDL 3.1.3 as its minimum, which is satisfied by the selected SDL 3.4.10.

Do not link SDL_shadercross into the game or compile shaders at runtime. Keep the
tool and its bundled libraries in a repository-local ignored tools directory. An
acquisition script must verify hashes. If the GitHub artifact expires, build the
same pinned commit with its recorded submodules or publish a verified project-owned
mirror; do not silently move to current `main`.

### D-012: Compile HLSL into backend-specific shader assets offline

Status: accepted for bootstrap on 2026-07-27.

HLSL is the only authored shader language. SDL_shadercross produces SPIR-V for the
Linux/Vulkan build, DXIL for Windows/D3D12, and MSL for macOS/Metal. The game loads
only compiled assets matching the selected SDL_GPU backend. The first triangle
compiles SPIR-V only, while the build interface preserves the other outputs for
later platform jobs.

### D-013: Keep controller and keyboard/mouse input active together

Status: accepted.

Support the broad range of controllers exposed through SDL's gamepad APIs,
including hotplugging and rumble where hardware capabilities allow it. Rumble must
be optional and must never carry gameplay information that has no visual or audio
equivalent.

Keyboard and mouse input remain active at the same time as every connected
controller. Never force the player into an exclusive input mode or require a menu
toggle to change devices. UI glyphs may follow the last meaningful input, but that
presentation choice must not disable or discard events from other devices.

Test representative Xbox, PlayStation, Nintendo-style, Steam Deck, and common
generic controllers as hardware becomes available. Device-specific features are
progressive enhancements rather than requirements for basic play.

SDL3 is the baseline input provider. Platform code converts every device into
semantic analog/digital actions before simulation. Signed axes merge by greatest
absolute magnitude, unsigned axes by maximum, and buttons by logical OR; inputs
are never summed. Steam Input may later feed the same boundary but is neither a
runtime requirement nor permission to place Steam calls in gameplay code.

### D-014: Separate ship definitions from visual mesh sources

Status: accepted on 2026-08-11.

Each ship has an explicit gameplay definition containing its identity, visual-mesh
key, handling parameters, local collider, collision mass, energy, and damage
response. Gameplay values are not inferred from render geometry.

Generated C++ geometry and future Blender/GLB assets both produce the same generic
CPU `MeshData`, which is then uploaded as a renderer-owned `GpuMesh`. Adding,
removing, or replacing a visual source must not require a new renderer path or
silently change the associated gameplay definition. Do not add a generator class
hierarchy or proprietary asset database until a concrete source requires more than
this value boundary.

### D-015: Treat 240 FPS as a target, not an engine ceiling

Status: accepted on 2026-08-11.

Support 240 FPS as an important performance target and explicit limiter choice,
but do not impose it as the maximum render rate. The settings milestone should
offer refresh-matched/VSync behavior, common explicit limits such as 30, 60, 90,
120, 144, 165, 240, and 360 FPS, plus uncapped rendering. Refresh matching covers
displays with nonstandard or fractional rates without requiring every rate to be a
preset.

Expose only SDL_GPU presentation modes supported by the active window/device.
VSync is the portable fallback; mailbox and immediate are latency/tearing choices,
not universally available upgrades. Treat frames in flight, limiter pacing, frame-
time variance, power draw, and end-to-end input latency as measured concerns.

The bootstrap remains VSync-presented with no custom frame limiter. The 120 Hz
fixed simulation remains independent and provisional; compare higher simulation
rates later using game feel, latency, deterministic behavior, and representative
full-race CPU cost. Do not increase simulation frequency merely to match a monitor.

### D-016: Prefer native Wayland on Linux with X11 fallback

Status: accepted on 2026-08-11.

When the user has not explicitly selected an SDL video driver, Linux builds request
`wayland,x11` in that order before SDL initialization. This preserves an X11
fallback and user diagnostic overrides.

The decision follows an interactive Steam Controller diagnosis on GNOME Wayland.
Steam's desktop layout emitted a synthetic `W` through XTEST when the left stick
moved forward; SDL's X11/XWayland path reported that input through the same
aggregate keyboard as legitimate keys. Native Wayland retained real keyboard and
standard SDL gamepad input without receiving the synthetic key. Prefer the clean
backend boundary over a device-specific timing heuristic or disabling simultaneous
keyboard/controller support.

### D-017: Keep named development scenarios in the game executable

Status: accepted on 2026-08-11.

Use `--scenario NAME` for fast, focused interactive environments that still pass
through the real game executable and shared runtime. Use `--list-scenarios` and
`--help` for headless discovery. Parse scenario names into a closed, tested enum;
reject unknown and duplicate selections rather than silently choosing a default.

Retain the generated runway as the `runway` input, movement, camera, and hardware
diagnostic when the oval arrives. Add the oval as a separate named scenario. Do
not create one executable per experiment, a general scene framework, or divergent
copies of simulation/render/input logic. The default launch is `runway` during the
current prototype and may become the normal menu/game flow later.

### D-018: Use a closed distance-sampled track boundary

Status: accepted for the first oval on 2026-08-11.

Represent a runtime track as an ordered set of orthonormal frames in the half-open
range `[0, length)`. Each frame owns center, forward tangent, surface normal,
track-right binormal, and half-width. Define track-right as `normal × tangent` and
wrap both positive and negative query distances across the implicit last-to-first
seam.

Generated and future authored paths must converge on this boundary. The first flat
stadium oval analytically produces evenly spaced frames; sampling interpolates and
re-orthonormalizes them. This validates distance, seam, and lateral-offset behavior
without prematurely choosing the general spline or 3D frame-transport algorithm
needed for loops and corkscrews.

### D-019: Keep track-bound state path-local and path-identifiable

Status: accepted on 2026-08-12.

Represent an attached ship with `TrackVehicleState`: an opaque nonzero path ID,
distance along that path, lateral offset and velocity, forward speed, and local
surface-normal offset and velocity. Scenario or course data owns geometry and
resolves the ID; vehicle state does not store a `SampledTrack` pointer or depend on
the oval generator.

This keeps continuous banks, vertical sections, loops, and corkscrews in the
sampled coordinate frame rather than assuming world Y is up. It also leaves a
future track graph free to transition path identity and distance at splits,
shortcuts, and joins. A genuine airborne jump will later use world-space motion
and reacquire an eligible path instead of remaining artificially constrained to
one path. Do not build the graph, branching policy, or airborne state until an
observable track checkpoint requires them.

### D-020: Treat ship maximum speed as a baseline, not a universal cap

Status: accepted on 2026-08-12.

Store a provisional base maximum forward speed in each ship's handling profile.
Do not define one immutable game-wide maximum. The eventual effective limit and
acceleration response may depend on ship selection, track surface or zone, local
slope, attached versus airborne/jump state, boost, and other explicit temporary
effects.

Add those factors only when their observable mechanics exist, with a documented
composition order and tests. Rendering, UI, input, and asset code must not enforce
their own gameplay caps. The current Prototype 01 value and boost multiplier are
tuning data and may change freely.

### D-021: Separate shared vehicle dynamics from movement mode

Status: accepted and implemented for first attached driving on 2026-08-12;
longitudinal-force ownership was refined by D-025.

Boost lifecycle/events and visual steering response are shared deterministic
vehicle dynamics. Free-planar `runway` movement and track-attached movement build
on those shared controls, then independently choose how forces, position, and
orientation advance. Do not fork boost or input mechanics per course or give AI
vehicles separate movement physics.

While attached, course ownership resolves the state's opaque path ID to sampled
geometry for the tick. The ship owns a persistent heading in that path's local
surface plane. Steering rotates the heading; body speed is decomposed into path
and lateral motion; grip controls lateral response; and selected-lane length
scales centerline progress. Tangent rotation around the surface normal is removed
from relative heading, so horizontal turns require steering rather than becoming
an autopilot rail. Surface pitch and roll remain automatic. A full forward/up
basis is derived from tangent, normal, and binormal, replacing world-yaw-only
orientation so banks, vertical sections, loops, and corkscrews do not require
another model.

Prototype 01 has explicit provisional attached lateral speed and ride-height
parameters. Its local box footprint is temporarily clamped to the sampled road
width. This is a bootstrap safety boundary, not a final edge policy: walls, open
edges, falling, traps, collision response, and recovery require explicit course
data and mechanics.

Splits and joins will transition path ID and canonical distance at the course
layer. A genuine jump will transition to a separate world-space airborne state
and may reacquire a different eligible path. Do not encode branch choice into
`SampledTrack`, force airborne ships onto a surface normal, or build the graph and
zone system before their first playable track experiment.

### D-022: Make world motion authoritative and course position derived

Status: accepted on 2026-08-12 after direct inspection of the F-Zero X matching
decompilation and comparison with the first attached playtest.

Retain the sampled track, stable path identity, semantic input, fixed-step loop,
and shared propulsion/boost boundary from D-018 through D-021. Supersede D-021's
scalar-driven attached movement as the final handling architecture: it remains
temporary scaffolding only.

The racer will own world position, world velocity, accumulated acceleration, and
an orthonormal physical basis in every contact mode. Each tick predicts world
motion, then projects the candidate point onto eligible nearby course paths to
derive distance, lateral offset, height, track basis, race progress, and contact
response. Projection must be bounded by a previous path/distance hint and future
route eligibility; globally snapping to the nearest geometry is invalid for
loops, crossings, stacked paths, splits, and jump landings.

Keep course basis, physical vehicle basis, and visual model basis distinct.
Steering rotates the physical vehicle orientation while velocity retains inertia.
Grip removes lateral velocity; drift applies an explicit lateral force and may
modify steering, traction, and forward acceleration. The ship's configured ride
height is a spring/damping target under gravity rather than an assigned offset.
Supported, airborne, and falling behavior change forces and contact policies but
do not switch to unrelated position representations. Walls and open edges require
different explicit responses.

F-Zero X is a qualitative behavioral reference only. Do not copy decompiled code,
Nintendo content, internal units, or 60/50 Hz per-tick constants. Codename Hover's
implementation, SI-scale tuning, 120 Hz fixed step, route graph, and content remain original. The
staged migration and verified reference links are recorded in `physics.md`.

### D-023: Validate world motion beside the scalar Speedway

Status: accepted and implemented provisionally on 2026-08-12.

Keep `runway`, `oval`, and `speedway` as existing regressions while
`speedway_physics` reuses the Speedway geometry and spawn with authoritative world
position, velocity, and physical basis. Track progress, lateral displacement, and
height are derived through the bounded projection query after candidate movement.
The temporary exact ride-height and collider-aware edge constraints were
subsequently replaced by D-026.

Replace the single drift action with independent left and right semantic actions.
LB/L1 and Q request left drift; RB/R1 and E request right drift. Each produces a
lateral force without steering input. Holding both cancels drift force and uses
normal grip. Keep the provisional force, grip, steering, and speed-loss values in
the ship handling definition so multiple ships can later differ without input or
renderer special cases.

Do not remove the scalar scenario until the world-space model passes deterministic
coverage and owner playtests. Do not tune hover, real edges, or jumps inside this
comparison checkpoint.

### D-024: Reconstruct handling relationships in measured stages

Status: accepted on 2026-08-12 after two world-space handling playtests and deeper
inspection of the matching F-Zero X racer and camera updates.

Use the matching decompilation as a behavioral reference for state separation,
fixed-update order, local velocity damping, bounded traction, drift-force
attenuation, propulsion response, hover/contact modes, collision consequences,
and camera dependencies. Do not copy anonymous per-frame constants or preserve
the original monolithic structure. Translate understood relationships into named
SI-scale parameters and explicit state at Codename Hover's 120 Hz fixed step.

Before gravity, walls, track forms, attacks, or AI, complete a measured grounded
handling checkpoint: maintain a source reference ledger, introduce a stable
telemetry vocabulary, split the world tick into named internal stages without a
generic physics engine, add a clean wall-free test surface and scripted input
traces, then replace provisional slip compensation only when the reconstructed
local-axis and propulsion model has comparative evidence.

Keep physical, course, visual, and camera frames separate. Preserve route graphs,
modern input, original content, and other project-specific extensions. Exact
emulation is not claimed without legal reference telemetry. The complete sequence
and exit gates live in `handling_implementation_plan.md`.

### D-025: Give world physics a source-shaped longitudinal model

Status: accepted and implemented provisionally on 2026-08-12.

Use a new straight-line baseline in `handling_lab` and `speedway_physics` rather
than compensating every new force to preserve the earlier scalar traces. Keep
boost activation/timers/events and visual roll shared, but let world physics own
local-axis damping, speed-shaped propulsion, propulsion response, braking,
coasting, and excess-boost return. Scalar scenarios retain their established
linear longitudinal model as regressions.

Convert reference-like axis retention into continuous exponential damping at the
120 Hz production step. Reduce positive propulsion using measured physical
direction change, current drift-force fraction, and a named sustained-slip
intensity. Slip intensity builds from remaining lateral velocity and releases
after grip recovers. Remove the provisional constant drift deceleration and
direct proportional slip drag rather than stacking both models.

Prototype 01's initial values are tuning data, not reference constants. Require
deterministic scripts, equal-duration 60/120 Hz comparisons, and owner playtests
before treating the tune as accepted feel.

### D-026: Make traction risk and road contact consequential

Status: accepted and implemented provisionally on 2026-08-12; owner playtest
pending.

Do not create boost instability with random yaw. Shape the bounded grip already
used by world physics: reduce available traction across the upper speed envelope
and under sustained slip, then let throttle lift or braking recover some control.
Expose demand, availability, and saturation through telemetry so tuning remains
measurable.

Replace exact ride-height assignment with gravity, damped hover force,
penetration-only correction, and explicit supported/airborne/falling state while
keeping world position and velocity authoritative. Track segments own discrete
left/right solid or open edge policy. Solid walls correct the oriented ship box,
recoil, remove scrape speed, and emit semantic impact feedback; open edges never
clamp and can enter falling/recovery.

Use `speedway_physics` as a deliberate A/B course: guarded straights and first
turn, open second turn. Automatic recovery to the last collider-safe pose is a
development policy, not the final damage/rescue design. Preserve scalar
`speedway` until deterministic coverage and repeated owner playtests accept the
world replacement.

## Deferred decisions

- Final game title, fiction, and visual design language.
- Commercial content count and price.
- macOS as a committed release platform.
- Split-screen scope.
- HDR and retro presentation modes.
- Online or cross-platform multiplayer.
- a player-facing track editor.
- Xbox and PlayStation release commitments.
- Final Steamworks feature set.
- Compiled asset format.

Deferred means “do not implement now,” not “silently rejected.”

## Open technical questions

- Which spline type and coordinate-frame transport behave best through loops,
  corkscrews, and closed-track seams?
- Is 120 Hz the right simulation rate after latency, feel, and CPU tests?
- What is the minimum debug-text solution before a broader UI path is warranted?
- Which Linux runtime/container baseline will be supported for Steam distribution?
- Which laptop GPU/driver and Steam Deck performance baselines should be recorded?

Resolve these questions at the milestone that needs the answer. Add a numbered
decision entry with the evidence and consequences instead of letting the answer
live only in code or chat history.
