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
   Implemented.
6. Add explicit wall and open-edge policies before removing the safety clamp.
   Implemented for road segments in `speedway_physics`.
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
propulsion, applies drift and velocity-based grip, applies gravity and hover,
integrates a candidate point, projects it locally onto the current path, and then
resolves support, penetration, walls, or open-edge falling.

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
- exponential local-axis damping at 0.240481/s forward and 0.180271/s lateral;
  the 3.71252/s normal rate is airborne-only because supported normal motion is
  owned by hover damping;
- propulsion remains full below 45% of base speed and smoothly reaches 0.81× at
  base maximum speed;
- rising propulsion responds from 12/s at rest to 60/s at base maximum, while
  reductions apply immediately;
- lateral slip above 8 m/s builds a normalized response over 1.6667 seconds,
  reaches a 0.50× propulsion multiplier, and releases over 0.75 seconds;
- ordinary world coasting uses forward damping; post-boost excess speed adds a
  90 m/s² return term.

Normal grip begins as a fixed maximum amount of sideways velocity removed per
second. Its available budget is full below 75% of base speed, falls smoothly to
45% at the boosted ceiling, and full sustained slip reduces the remainder to
55%. Lifting restores 1.5x and full braking 1.8x relative to that risky-speed
budget. Telemetry reports demand, availability, and saturation. The 300 m/s²
base remains an initial tune, not an accepted final value.

The second tune follows relationships visible in the matching F-Zero X
decompilation's [`Racer_UpdateFromControls`](https://github.com/inspectredc/fzerox/blob/main/src/game/racer.c#L3345):
the original rotates the machine basis without rotating momentum, damps velocity
in local axes, uses a much lower lateral-velocity removal cap while either
shoulder is held, fades shoulder side force as same-direction lateral speed
builds, and subtracts both steering direction-change and drift force from engine
acceleration. Codename Hover expresses these as units-per-second parameters at
120 Hz; it does not copy opaque N64 per-frame constants or claim exact emulation.

World velocity is decomposed into physical forward, lateral, and normal axes,
damped with `component × exp(-rate × dt)`, and recomposed before drift and bounded
grip. Measured physical direction change, rather than raw stick magnitude, drives
the steering propulsion penalty. A speed-shaped requested acceleration rises
through a stored response value; reduced targets and braking take effect
immediately.

The earlier proportional slip drag and constant drift deceleration are removed.
Remaining lateral speed now builds named sustained-slip duration and intensity.
That intensity persists after drift release, weakens requested propulsion, and
decays only after lateral velocity returns below the threshold. Damping turns the
lost propulsion into organic speed loss, so holding throttle cannot erase a
slide. No random yaw is injected: instability comes from heading, momentum,
lower grip, fading side-force authority, and delayed propulsion recovery
disagreeing with each other.

The exact ride-height assignment and universal edge clamp are removed from world
physics. Prototype 01 uses 30 m/s² gravity, a 180/s² hover spring, 27/s damping,
and a 120 m/s² lift cap around its 0.62 m target. Only real hull penetration is
corrected. Explicit supported, airborne, and falling modes preserve world motion;
physical up eases toward support and airborne gravity-up eases toward world up.

Every sampled chord has independent left/right solid-wall or open-edge policy.
Solid walls resolve the oriented local box, use 0.15 restitution, retain 85% of
tangential scrape velocity, and emit impact telemetry/events. Open edges do not
correct lateral motion. A fall returns to the last pose recorded at least one
metre inside the collider-safe road after 1.25 seconds or a 20 m drop, at 25% base
speed with transient boost/slip state reset. Damage, effects, jump ramps, and
route-aware airborne landings remain later steps. Keep scalar `speedway` until
the owner accepts this model.

The fixed tick is internally divided into boost lifecycle, steering, local
damping, drift/grip, sustained-slip, propulsion, gravity/hover, and contact stages.
Every completed world-space tick returns a read-only telemetry snapshot containing
world and local velocity, signed slip angle, measured steering direction change,
axis damping, drift direction/force, selected and available grip, grip demand and
saturation, propulsion response, sustained-slip state, post-boost return, surface
height/normal speed, contact mode, and wall impact speed.
Telemetry is observational and never feeds back into physics.

`--scenario handling_lab` runs this same authoritative simulation on a generated
6 km straight/1 km turn-radius/800 m half-width flat surface. It prints the most
useful handling values with the existing one-second frame log, avoiding bank,
tight curvature, and ordinary edge correction during controlled tests. Automated
script traces cover straight acceleration, full-speed steering, boost-turn,
drift entry/sustain/release, throttle release, and braking during boosted drift.
Each trace is replayed twice and compared tick by tick.

The accepted full source-to-project sequence, conversion rules, stage exit
criteria, and immediate grounded-dynamics checkpoint are maintained separately in
`handling_implementation_plan.md`. Keep this file focused on current physics
behavior and durable principles.
