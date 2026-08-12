# Vehicle physics

## Purpose

Codename Hover uses a purpose-built arcade racer simulation. It is not a general
rigid-body engine, but the racer is still a real world-space moving object. The
track supplies nearby surface geometry, a reference frame, progress, and contact
policy; it must not directly advance the player's position from a scalar distance.

The desired foundation is inspired by observable behavior in F-Zero X while all
code, tuning, ship data, and game content remain original. The reference is useful
for system boundaries, not for copying per-frame constants or reverse-engineered
implementation details.

## Fit with the current stack

This model is fully compatible with C++23, the fixed-step runtime, SDL3, and
SDL_GPU. The simulation needs vectors, orthonormal bases, deterministic state, and
track queries; none require a general physics library or a different graphics
API. SDL3 already supplies semantic controller input, device hotplugging, and
rumble. SDL_GPU only renders the interpolated visual pose and does not constrain
how physics is calculated.

The simple graphics target leaves ample CPU budget for this specialized model and
roughly thirty racers, subject to later measurement on the laptop and Steam Deck.
Course projection is currently a clear linear search through sampled chords. Add
a spatial index only after representative tracks and racer counts demonstrate a
need; doing so will not change the public projection or vehicle-state boundary.

## Verified reference observations

The observations below were checked against the `inspectredc/fzerox` matching
decompilation at commit `4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8`.
Names in that project are reverse-engineered and therefore are not evidence of
Nintendo's original source names.

- The racer owns full 3D velocity and acceleration plus separate course,
  physical-machine, and rendered-model bases. See the
  [racer and course structures](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/include/unk_structs.h#L10).
- Controls rotate the machine's physical basis. Momentum is not automatically
  rotated onto that new forward direction. Grip subsequently removes lateral
  velocity, while shoulder-button drift adds a lateral force and changes other
  handling terms. See
  [`Racer_UpdateFromControls`](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L3345).
- The update integrates acceleration into velocity, predicts a world-space
  position, and resolves that point against nearby course segments. Lap distance
  and the segment frame are then derived from the result. See the
  [racer integration and course resolution](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L4155)
  and the
  [iterative course projection](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/course.c#L973).
- Hovering applies gravity and a height-error/vertical-speed correction instead
  of assigning an exact ride height. That permits takeoff, landing, and surface
  penetration correction within one world-space state.
- Road forms dispatch different contact responses. A wall pushes the racer back,
  removes or reduces velocity into the wall, and produces damage/recoil; an open
  edge can become airborne or falling instead of clamping. See the
  [track-shape dispatch](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L506),
  [wall response](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L2325),
  and [shape definitions](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/include/fzx_course.h#L310).
- Race cameras use the racer's physical basis and orthonormalize their own view
  frame rather than copying the course bank directly. See the
  [race-camera basis construction](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/camera.c#L2578).
- NTSC race timing is derived from one update counter as 16.666... milliseconds
  per update, while PAL uses 20 milliseconds. F-Zero X coefficients are therefore
  effectively 60 Hz or 50 Hz per-tick values, not values that can be pasted into
  Codename Hover's provisional 120 Hz simulation.

The reference course is essentially a cyclic next/previous sequence. Codename
Hover's stable path identity and future course graph are intentional extensions
for splits, shortcuts, jump landings, and joins.

## Codename Hover state model

The eventual racer state has three deliberately separate frames of reference:

```text
course frame       physical vehicle frame       visual model frame
tangent/normal     forward/up/right              smoothed lean/recoil
surface + edges    forces and collision          shake and animation
```

The authoritative physical state should own at least:

- world position, velocity, and accumulated acceleration;
- an orthonormal physical forward/up/right basis;
- boost and other existing deterministic gameplay state;
- a contact mode such as supported, airborne, or falling;
- the most recent course reference: path ID, projected distance, lateral offset,
  height, surface frame, and whether that reference remains eligible.

World position and velocity remain authoritative in every contact mode. Becoming
airborne changes which forces and collision responses apply; it does not move the
racer into an unrelated position representation. Keeping a nearby course
reference while airborne helps with landing tests, race progress, AI intent, and
recovery, but never means the ship is snapped to that course.

The visual pose is derived after physics. It may smoothly lag physical up/forward,
add the existing turn roll, vibration, boost response, recoil, or future attack
animation without feeding those effects back into velocity or collision.

## Fixed-tick order

The intended normal tick is:

1. Merge semantic player or AI input.
2. Rotate the physical vehicle basis from steering and drift controls.
3. Build forces: propulsion/braking, drag, gravity, hover, grip, drift, boost, and
   explicit surface/zone effects.
4. Integrate acceleration into world velocity and predict world position.
5. Project the candidate position onto eligible nearby course paths using the
   previous path/distance as a bounded hint.
6. Derive local forward, lateral, and normal measurements from that projection.
7. Resolve contact and edge policy: supported surface, wall, open edge, pipe,
   jump/air, trap, or another explicit shape.
8. Commit world state and derived course progress.
9. Update presentation and camera targets from the physical state.

The course query must be local and eligibility-aware. A globally nearest point can
select the wrong layer of a loop, the wrong side of a crossing, or an unrelated
branch. The current `project_point_onto_track` primitive therefore accepts a path
distance hint and bounded search radius. A future course graph will choose which
paths are eligible and compare their projection results during explicit split,
join, landing, or recovery windows.

## Force behavior

Steering rotates the physical basis around an appropriate local up direction.
Velocity retains inertia, so pointing into a corner and actually changing the
travel direction are different operations.

Grip works on local lateral velocity. It should remove at most a tunable amount of
sideways speed per second, rather than commanding a target lateral velocity. This
makes low grip, impacts, drift, and different ship handling emerge from the same
state.

Drift is an explicit lateral force combined with altered steering, grip, and
possibly forward acceleration. Left and right drift actions may later be separate
semantic controls even if an early keyboard mapping is simple. Drift strength can
fall as same-direction lateral velocity grows so it remains controllable.

The intended controller bindings are left shoulder/L1 for left drift and right
shoulder/R1 for right drift. Represent them as two semantic actions before adding
the force. Pressing both simultaneously needs an explicit policy and tests rather
than accidental cancellation; an initial neutral/cancel result is acceptable.

## Mass and weight

Prototype 01 already declares `relative_mass = 1.0`. Keep mass dimensionless
during early handling work: it gives collision impulses and ship-to-ship response
a baseline without pretending the generated ship has a researched kilogram mass.
The ship's configured propulsion and braking values are accelerations, so making
the ship heavier must not silently weaken them unless a later handling design
explicitly converts engine output into force.

Weight is not a separate ship statistic; it is the gravity force implied by mass
and gravity. Gravity and hover acceleration affect ordinary racers consistently,
while relative mass becomes meaningful for collisions, pushes, impacts, and
possibly ship-specific resistance to external forces. Revisit physical units when
the first two-ship collision experiment provides evidence.

The configured ride height is a hover target, not an assignment:

```text
height error = target ride height - measured surface height
normal speed = dot(world velocity, surface normal)
hover acceleration = spring strength * height error - damping * normal speed
```

Clamp or shape this response for stability and game feel. Apply gravity
independently. Loss of support, crests, jump ramps, landings, pipes, and inverted
surfaces then use the same position and velocity instead of bespoke teleports.

Per-second accelerations, rates, and time-based damping are preferred. When an
older per-tick behavior is used as a qualitative reference, convert it for the
chosen fixed tick and retune it; never copy a 60 Hz coefficient into 120 Hz code.

## Edge and surface policies

Track width is geometry, not a universal invisible clamp. Each edge or surface
section eventually declares behavior such as:

- solid wall or guard rail;
- open/fall edge;
- borderless surface;
- pipe, cylinder, half-pipe, or tunnel contact;
- jump/air section;
- boost, recharge, grip, damage, or trap zone.

A wall corrects penetration and responds only to velocity directed into it. It
may remove normal velocity, retain scrape velocity, add recoil, damage, sparks,
sound, rumble, and camera response. An open edge does not use the wall correction.

## Migration from the current prototype

The current `TrackVehicleState` was valuable scaffolding: it proved sampled
frames, seams, banking, persistent player heading, path identity, camera use, and
render-rate-independent stepping. Its scalar distance still directly advances
the ship and its grip approaches a requested lateral velocity, so it is not the
final physics foundation.

Migrate in testable slices:

1. Add hint-aware world-point projection without changing playable behavior.
   Implemented.
2. Add world velocity and a physical basis beside the existing state; initialize
   them from the current spawn pose. Implemented in `speedway_physics`.
3. Make world integration authoritative in a dedicated physics scenario, then
   derive path distance/lateral/height through projection. Implemented in
   `speedway_physics`.
4. Replace target lateral velocity with local-velocity grip and steering-driven
   physical orientation. Implemented provisionally and awaiting playtest tuning.
5. Replace exact ride-height assignment with gravity plus hover spring/damping.
6. Add explicit wall and open-edge policies before removing the safety clamp.
7. Add drift forces and tune their interaction with steering, traction, and
   acceleration.
8. Add jump takeoff, airborne control, landing eligibility, and route changes.
9. Derive a smoothed visual basis and make the follow camera consume the physical
   or intentionally filtered vehicle basis.

Do not attempt to tune final values while both the scalar-driven and world-driven
models contribute to the same axis. Each migration step needs deterministic tests
and an interactive comparison before deleting the superseded behavior.

## First playable world-space checkpoint

`--scenario speedway_physics` reuses the banked Speedway geometry and spawn but
selects `WorldTrackVehicleState`. The state owns world position, world velocity,
physical forward/up, the existing gameplay/presentation state, and a derived
course reference. Each 120 Hz tick rotates physical orientation, updates forward
propulsion, applies drift and velocity-based grip, integrates a candidate point,
projects it locally onto the current path, and only then enforces temporary
ride-height and edge safety constraints.

The current directional drift tune is:

- LB/L1 or Q: left force; RB/R1 or E: right force;
- both held: no drift force and normal grip;
- 105 m/s² lateral drift acceleration;
- lateral drift force fades to zero by 32 m/s of same-direction slide;
- 55 m/s² maximum lateral-speed removal while drifting;
- 300 m/s² maximum lateral-speed removal normally;
- 1.15× steering response;
- steering can remove up to 35% of positive propulsion and full-strength drift
  can remove another 85%; combined loss is capped at 100%;
- 60 m/s² forward drift deceleration;
- lateral slip above 8 m/s adds 4 m/s² of forward deceleration for every
  additional 1 m/s of sideways speed.

Normal grip is deliberately a fixed maximum amount of sideways velocity removed
per second, rather than a speed-proportional interpolation. A low-speed steering
change can therefore remain fully planted while the larger direction change
created by the same steering rate at base or boost maximum speed exceeds the
available grip and leaves progressively more lateral slip. The 300 m/s² value was
lowered from 420 m/s² after the first owner playtest found boosted cornering too
safe. This is an initial tune, not an accepted final value.

The second tune follows relationships visible in the matching F-Zero X
decompilation's [`Racer_UpdateFromControls`](https://github.com/inspectredc/fzerox/blob/main/src/game/racer.c#L3345):
the original rotates the machine basis without rotating momentum, damps velocity
in local axes, uses a much lower lateral-velocity removal cap while either
shoulder is held, fades shoulder side force as same-direction lateral speed
builds, and subtracts both steering direction-change and drift force from engine
acceleration. Codename Hover expresses these as units-per-second parameters at
120 Hz; it does not copy opaque N64 per-frame constants or claim exact emulation.

The slip threshold and proportional forward loss are our explicit, tunable
translation of the original's sustained-side-slip state into the current runtime.
They make ordinary maximum-speed steering and boost-speed steering lose energy
after momentum diverges far enough from heading. Shoulder drift additionally
suppresses propulsion and applies constant forward loss, so holding throttle no
longer cancels the slowdown every following tick. No random yaw is injected:
the instability comes from heading, momentum, lower grip, and fading side-force
authority disagreeing with each other.

The exact ride height and collider-aware edge remain intentional temporary
constraints. They remove velocity into the constrained direction rather than
allowing hidden penetration to accumulate. Gravity, spring/damping hover, contact
modes, wall/open-edge policies, jumps, and visual-basis smoothing remain later
steps. Keep the scalar `speedway` scenario until the owner accepts this model.
