# Handling implementation plan

## Status and purpose

Accepted on 2026-08-12. This document preserves the long-term implementation
sequence for Codename Hover's vehicle, surface-contact, and follow-camera model.
It complements `physics.md`, which describes the current physics boundary and
accepted principles. Read this longer plan when selecting or sequencing a
handling milestone, not for every small physics change.

The objective is to reproduce the important observable relationships behind
F-Zero X's handling in an original, modern 120 Hz simulation:

- steering rotates the ship without directly rotating momentum;
- velocity is decomposed and modified in the ship's local frame;
- grip removes only a bounded amount of lateral velocity;
- shoulder drift changes steering, traction, propulsion, and lateral force;
- drift force weakens as same-direction sliding builds;
- aggressive direction changes and sustained slip cost forward speed;
- gravity, hover, surface contact, and airborne behavior share one world-space
  representation;
- track, physical, visual, and camera orientations remain distinct;
- walls, open edges, pipes, jumps, damage, and other racers interact with that
  same simulation.

This is behavioral reconstruction, not a port or a disguised remake. Ships,
tracks, pilots, fiction, assets, code organization, SI-scale parameters, route
graphs, and modern extensions remain original.

## Reference boundary and caveats

The primary research sources are the matching decompilation's
[`Racer` state](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/include/unk_structs.h#L156-L340),
[`Racer_UpdateFromControls`](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L3345-L4155),
[wall and track-shape contact](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L2325-L2624),
and [smooth follow camera](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/camera.c#L559-L676).

Treat those sources as evidence, not as ready-to-paste production code:

- many reverse-engineered fields still have unknown names;
- some machine parameters are derived from ROM data;
- the original uses game-specific distance and per-frame velocity units;
- NTSC behavior is tuned around 60 updates per second;
- source order may encode dependencies that are not obvious from one formula;
- exact numeric comparison would require a legally owned compatible ROM or
  recorded reference telemetry;
- route splits and alternate paths are Codename Hover extensions rather than
  original F-Zero X course behavior.

Do not claim exact emulation from source inspection alone. The practical target
is matching response shapes and player-visible behavior while retaining the
project's 120 Hz fixed simulation and original content.

## Source-to-project state map

The reference racer contains considerably more mixed gameplay and presentation
state than Codename Hover needs. Preserve its useful separations without copying
the monolithic structure.

| Reference concept | Codename Hover responsibility |
| --- | --- |
| segment position and basis | derived `ProjectedCourseReference` and `TrackFrame` |
| world position, velocity, acceleration | authoritative physical vehicle state |
| `trueBasis` | physical vehicle basis |
| `modelBasis` | separately smoothed presentation basis |
| local velocity components | derived forward, normal, and lateral velocity |
| height above ground | surface-query result |
| `upFromGround` | current supporting surface normal |
| `gravityUp` and `tiltUp` | gravity reference and desired physical-up direction |
| hover target/correction | ship handling profile plus runtime contact state |
| drift force and shoulder timers | semantic drift input plus handling runtime state |
| sustained lateral-slip state | named slip duration/intensity state |
| boost, damage, spinout, and recoil | small explicit gameplay state groups |
| focus position and velocity | camera-facing filtered physical state |

The intended project state is:

```text
WorldTrackVehicleState
├── PhysicalVehicleState
│   ├── world position
│   ├── world velocity
│   ├── accumulated acceleration
│   ├── physical forward/up basis
│   ├── local velocity
│   └── current and previous speed
├── SurfaceContactState
│   ├── contact mode
│   ├── derived course reference
│   ├── surface frame and normal
│   ├── gravity direction
│   ├── height and normal velocity
│   └── last supported position
├── HandlingRuntimeState
│   ├── drift direction and force
│   ├── sustained slip duration/intensity
│   ├── smoothed propulsion response
│   ├── boost and energy
│   ├── damage and spinout
│   └── collision recoil
└── VehiclePresentationState
    ├── visual basis and position
    ├── turn/drift roll
    ├── vibration and recoil
    └── effect envelopes
```

Use an explicit contact mode rather than combinations of unrelated booleans:

```cpp
enum class VehicleContactMode {
    supported,
    airborne,
    falling,
    crashed,
};
```

## Target fixed-step order

The final order must be derived and verified against the source reference before
being treated as stable. The intended production shape is:

1. Consume one semantic input sample for the fixed tick.
2. Resolve current course/surface properties from the previous bounded hint.
3. Apply steering dead zone and rotate the physical basis.
4. Measure steering direction change independently from stick magnitude.
5. Decompose world velocity into local forward, normal, and lateral components.
6. Apply contact-mode and surface-specific local damping.
7. Resolve drift direction and attenuated side-force strength.
8. Select normal or drift traction and remove a bounded amount of lateral speed.
9. Resolve throttle/boost propulsion from speed and the ship's response curve.
10. Reduce positive propulsion using direction change and active drift force.
11. Accumulate gravity, hover, drift, jump, collision, and other forces.
12. Integrate acceleration into world velocity.
13. Integrate world velocity into a candidate world position.
14. Perform a bounded, route-aware course/surface projection.
15. Resolve support, penetration, wall, open-edge, landing, or falling policy.
16. Refresh derived local velocity and course measurements.
17. Smooth physical-up toward its supported or airborne target.
18. Emit semantic gameplay events.
19. Update presentation-only state from the final physical result.

The current comparison implementation does not yet follow all these stages. Do
not add more compensating constants to hide a missing stage; implement and test
the missing relationship.

## Converting the 60 Hz reference to 120 Hz

Never paste a per-frame coefficient into the production fixed step.

For a multiplicative 60 Hz coefficient `c60`, preserve equal elapsed-time decay:

```text
c(dt) = c60 ^ (60 × dt)
c120  = sqrt(c60)
```

Convert counters into seconds, then derive tick counts only at their consumption
boundary. Convert additive velocity and acceleration through an explicit
reference-unit scale. Keep source-derived reference values separate from the
final ship parameters so unit conversion cannot become an unexplained constant.

Create a test-only 60 Hz reference integrator for reconstructed equations. Given
the same timed input script, its 120 Hz production counterpart should converge on
similar positions, speeds, headings, and slip envelopes after equal elapsed time.
Rendering schedules must never alter either result.

## Stage 0: Reference ledger and telemetry vocabulary

The first ledger is maintained in `handling_reference.md`. Update its evidence,
confidence, and project mapping whenever later source inspection changes an
interpretation.

Create a focused reference ledger before the next large physics rewrite. For
each relevant source field or equation record:

- source location;
- inferred name and responsibility;
- reads and writes;
- likely units;
- per-frame versus continuous behavior;
- dependencies on update order;
- project equivalent;
- confidence and unresolved questions.

Define common telemetry names at the same time:

- world speed;
- local forward, lateral, and normal speed;
- slip angle;
- steering direction change;
- drift-force fraction;
- available traction;
- requested and applied propulsion;
- surface height and normal velocity;
- contact mode;
- wall impact speed;
- energy, damage, and boost state.

Exit criteria:

- the grounded racer update can be explained without relying on anonymous
  reference variable names;
- every planned production field has a stated owner;
- unknown behavior is marked as unknown rather than guessed silently.

## Stage 1: Source-shaped grounded dynamics

Refactor the growing world-space tick into named internal stages while keeping
one public game-specific simulation function. Do not create a generic physics
framework.

Implement:

- analog steering dead zone and response;
- physical-basis rotation without rotating momentum;
- measured direction change;
- local velocity decomposition/recomposition;
- time-corrected local forward/lateral/normal damping;
- bounded normal and shoulder-held traction;
- drift side-force attenuation from same-direction lateral speed;
- source-shaped acceleration suppression from direction change and drift force;
- named sustained-slip buildup/release state;
- speed-dependent propulsion response rather than independent compensating drag.

The current explicit slip-loss heuristic is temporary. Replace it when the
reference sustained-slip and acceleration behavior is sufficiently understood;
do not stack both models.

Test with a deliberately wide, flat, wall-free surface so edge correction cannot
hide handling errors.

Exit criteria:

- low-speed steering remains controllable;
- base-maximum steering develops measurable momentum lag;
- boost-speed steering develops more slip and speed loss;
- a shoulder drift changes steering, traction, side force, and propulsion;
- side force fades rather than accelerating laterally without limit;
- releasing input leaves recoverable momentum instead of snapping;
- 60 and 120 Hz reference trajectories agree within documented tolerances.

## Stage 2: Debug telemetry and reproducible input scripts

Add a development-only overlay and optional compact trace output. It should show
the telemetry vocabulary above without coupling UI to simulation.

Add deterministic input scripts for:

- straight acceleration;
- full-speed steering step;
- boost into a steering step;
- neutral-steer left and right drift;
- drift into and out of a turn;
- drift release while already sliding;
- throttle release during slip;
- braking during boost and drift.

Record response curves rather than judging every change from memory.

Exit criteria:

- a handling regression can be reproduced without live input;
- playtest feedback can name the measured state that felt wrong;
- trace output is stable enough to compare revisions.

## Stage 3: Gravity, hover, and supported contact

Replace exact ride-height assignment with forces and contact resolution:

1. Project against the eligible nearby surface.
2. Measure height and normal velocity.
3. Apply gravity along the current gravity direction.
4. Apply a damped hover correction toward the ship's ride-height target.
5. Correct actual penetration and remove only inward normal velocity.
6. Use hysteresis to transition between supported and airborne modes.
7. Smooth physical-up toward the supported surface normal.

Keep spring strength, damping, gravity, target height, airborne threshold, landing
threshold, and orientation response in the ship/contact profile.

Exit criteria:

- the ship hovers without visible numerical jitter;
- it rises naturally over a crest, leaves support, and falls;
- landing does not teleport or snap orientation;
- horizontal momentum survives takeoff and landing;
- banking still works with force-based height.

## Stage 4: Explicit walls and open edges

Remove the collider-aware invisible safety clamp once both policies exist.

Walled surfaces:

- correct penetration;
- respond only to velocity directed into the wall;
- preserve appropriate tangential scrape velocity;
- apply impact-dependent recoil, speed loss, and damage;
- emit collision, spark, sound, rumble, camera, and portrait events.

Open surfaces:

- do not push the ship back onto the road;
- lose supported contact when no eligible surface remains;
- enter airborne or falling mode according to position and velocity;
- retain world motion until recovery or destruction.

Exit criteria:

- boosted cornering errors cause visible, consequential wall impacts;
- crossing a borderless edge cannot silently clamp or snap back;
- collision response is deterministic and render-rate independent.

## Stage 5: Ship response curves and statistics

Replace the remaining provisional linear acceleration assumptions with explicit,
understandable response curves influenced by:

- current forward speed;
- acceleration versus maximum-speed tuning;
- relative mass;
- grip characteristics;
- boost characteristics;
- steering direction change;
- active drift force;
- sustained slip;
- surface properties.

Do not expose only copied letter grades. Store named ship parameters and derive
editor/HUD grades later if useful.

Exit criteria:

- Prototype 01 has one complete documented baseline profile;
- changing one ship parameter has a predictable measured effect;
- heavy, agile, stable, drift-oriented, and fragile future ships are possible
  without input, renderer, or track special cases.

## Stage 6: Surface effects and zones

Add track-authored properties consumed by the shared simulation:

```cpp
struct TrackSurfaceProperties {
    TrackSurfaceKind kind;
    float grip_multiplier;
    float propulsion_multiplier;
    float damping_multiplier;
    bool supports_hover;
    bool has_walls;
};
```

Introduce normal, dirt, ice, dash, recharge, jump, and damage/trap behavior one at
a time. Physics reads surface data; track objects do not mutate the player through
unrelated shortcuts.

Exit criteria:

- surface changes compose with ship handling and boost;
- player and AI vehicles receive identical mechanics;
- transitions are reproducible at segment boundaries.

## Stage 7: Loops and alternative track forms

Generalize the surface query and contact policy incrementally:

1. banked road;
2. vertical loop;
3. half-pipe;
4. full pipe interior;
5. cylinder exterior;
6. tunnel;
7. joins between different forms.

The surface query returns position, tangent, normal, lateral axis, height, bounds,
surface kind, and path identity. A loop must not need a special steering model;
its changing frame and gravity relationship should be sufficient.

Exit criteria:

- one ship can transition through road, loop, and pipe without switching position
  representations;
- orientation remains orthonormal through seams and joins;
- projection cannot jump to crossing or stacked geometry.

## Stage 8: Jumps and airborne control

Add:

- track jump impulses and natural crest takeoff;
- limited airborne pitch, yaw, and roll authority;
- gradual gravity/up-direction behavior;
- route-aware eligible landing surfaces;
- landing impact, bad-landing speed loss, and recovery thresholds.

Left-stick vertical input may become airborne pitch, but it must never implicitly
become throttle. Player and AI use the same semantic airborne actions.

Exit criteria:

- jumps are physical motion, not scripted trajectories;
- a jump may land on another eligible path;
- missed landings enter falling behavior cleanly.

## Stage 9: Damage, energy, spinout, and vehicle collisions

Implement in small order:

1. energy and maximum energy;
2. wall-impact damage;
3. recharge zones;
4. destruction and falling retirement;
5. spinout and recovery;
6. pairwise vehicle contact using relative velocity and mass;
7. side attack;
8. spin attack;
9. attack recoil and temporary control effects.

Simulation emits semantic events. Audio, effects, rumble, camera, HUD, and pilot
portraits subscribe outside authoritative physics.

Exit criteria:

- the same deterministic collision mechanics work for player and CPU racers;
- thirty racers remain architecturally possible;
- damage and destruction do not leak platform or presentation dependencies into
  physics.

## Stage 10: Physical, visual, and camera frames

Maintain four intentional frames:

- course frame: nearby surface orientation;
- physical frame: ship orientation used by forces and collisions;
- visual frame: smoothed model orientation plus feedback;
- camera frame: filtered view orientation and focus behavior.

The visual frame may add turn roll, drift lean, hover vibration, recoil, spinout,
boost response, and landing motion without altering physics.

The camera should consume filtered physical position, focus velocity, gravity/up,
and slip state. Add smooth anchor motion, velocity look-ahead, gradual up
alignment, and bounded collision/boost feedback. Do not copy the course frame
directly and do not let camera effects feed back into simulation.

Exit criteria:

- drift, landing, collision, and boost are legible from motion;
- loops and pipes do not cause abrupt camera flips;
- camera settings can change comfort without changing handling.

## Stage 11: Course graph, splits, and alternate routes

Extend beyond the single cyclic reference course:

```text
CourseGraph
├── path A
│   └── split junction
├── path B
├── path C
└── rejoin junction
```

Projection considers previous path/distance, bounded movement, explicit junction
candidates, travel direction, and airborne landing eligibility. Race progress
uses canonical route/checkpoint data rather than trusting raw local distance.

Exit criteria:

- ordinary and airborne splits can select different routes;
- alternate routes can rejoin without corrupting lap progress;
- shortcuts, crossings, and stacked paths do not cause global nearest-surface
  snapping.

## Stage 12: AI through ordinary controls

AI never assigns vehicle position, velocity, path distance, or lateral offset. It
produces the same semantic actions as the player:

- steering;
- throttle;
- brake;
- left/right drift;
- boost;
- later attack and airborne pitch actions.

Separate route planning, racing-line targets, speed planning, local steering,
traffic avoidance, overtaking, jump commitment, recovery, and aggression. Course
space informs decisions while world physics executes them.

Exit criteria:

- one AI completes road, loop, pipe, jump, and split test sections;
- it recovers from moderate disturbances rather than following a perfect rail;
- scaling to several racers is measured before targeting thirty.

## Stage 13: Reference comparison and acceptance

For every material physics revision, run scripted scenarios and record:

- speed over time;
- local velocity components and slip angle;
- heading and yaw rate;
- requested and applied acceleration;
- drift-force and traction limits;
- height, normal velocity, and contact mode;
- impact speed, damage, and recovery time;
- derived course progress.

If a legally owned compatible ROM becomes available, build or capture a reference
setup and compare normalized response curves. Do not compare or import Nintendo
content into the game.

The handling model is ready to replace the scalar regression when:

- boost-speed cornering requires anticipation;
- heading and travel direction visibly diverge;
- drift creates opportunity and danger rather than a free tighter turn;
- throttle cannot cancel slide losses;
- drift release leaves persistent but recoverable momentum;
- walls and open edges have distinct consequences;
- different ship profiles feel mechanically distinct;
- 24–360 Hz tested render schedules do not change outcomes;
- laptop and Steam Deck produce matching deterministic results;
- repeated owner playtests call the model enjoyable and identify only bounded
  tuning issues.

## Implementation sequencing rules

- Keep `speedway_physics` as the active experiment and `speedway` as the scalar
  regression until the replacement satisfies its exit criteria.
- Make one physical relationship authoritative per axis; remove temporary
  compensation when its source-shaped replacement lands.
- Keep each stage playable, tested, and independently reviewable.
- Add parameters to ship definitions only when simulation consumes them.
- Add track data only with the first scenario that exercises it.
- Preserve 120 Hz fixed simulation and interpolated rendering unless measurements
  justify a recorded decision change.
- Do not begin race systems, content production, or generic engine work to avoid
  completing handling.

## Immediate checkpoint

The next implementation checkpoint is Stage 0 plus the smallest part of Stage 1:

1. [x] Write the grounded-update reference ledger with meaningful inferred names,
   equations, source locations, units, and confidence.
2. [x] Add development telemetry values to the world-space state/result without
   rendering the full overlay yet.
3. [x] Refactor `simulate_world_track_vehicle` into named internal stages while
   preserving current behavior.
4. [x] Add a wide, flat, effectively wall-free handling scenario or test surface so edge clamps
   and oval curvature cannot contaminate measurements.
5. [x] Add scripted straight, steering-step, boost-turn, drift-entry, and drift-release
   traces.
6. [x] Reconstruct local-axis damping, propulsion response, and sustained-slip state
   from the reference update.
7. [x] Replace the current provisional slip-loss heuristic only after comparison
   tests demonstrate the reconstructed model.

The implementation uses continuous exponential damping, measured direction
change, a speed-shaped/smoothed positive-propulsion response, and persistent
sustained-slip buildup/release. The old constant drift deceleration and direct
proportional slip drag have been removed. Automated comparison covers 60 and
120 Hz equal-duration response plus deterministic scripted traces. Interactive
owner acceptance remains the gate before Stage 3 hover/contact work.

Do not start hover, walls, loops, or AI during this checkpoint. Its output should
be an understandable and measurable grounded handling core ready for owner A/B
playtesting.
