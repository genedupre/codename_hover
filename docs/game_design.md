# Game design

## Design priority

Handling comes before content. A racing game with many tracks and mediocre driving
is not successful. Early builds may contain only a primitive vehicle, a plain
track, a speed readout, and reset controls while acceleration, steering, grip,
drift, boost, collision response, recovery, and camera behavior are tuned.

## Vehicle model

Use specialized arcade vehicle logic rather than a general rigid-body engine.
The racer nevertheless owns authoritative world position, velocity, and physical
orientation. A nearby track provides surface/contact information and derived race
progress; scalar track distance must not directly move the final vehicle model.
The full simulation contract and reference evidence live in `physics.md`.

The current track-bound prototype state includes:

- an opaque current path identity and distance along it;
- lateral offset and lateral velocity in the path frame;
- normal offset and normal velocity relative to the local surface;
- forward speed.

Energy-backed boost limits, damage, lap, checkpoint, recovery, and airborne state
remain later additions. Local normal motion is intentionally not called
world-vertical motion: the ship may be banked, vertical, or upside down on loops.
A true jump changes contact and force policy while retaining the same world-space
state. It may later land on a different eligible path, including a shortcut or
branch.

Longer-term vehicle state still needs:

- energy, boost, and damage state;
- current lap, checkpoint, and recovery state.

The exact model is not yet specified. Add only the state required by observable
handling behavior, and record important tuning results in `../DEVLOG.md`.

Ship types use explicit definitions rather than deriving gameplay behavior from
their render meshes. The current definition boundary supports:

- a stable ship identity and visual-mesh reference;
- a local-space box collider sized for that ship;
- maximum forward speed, acceleration, braking, planar steering rate, attached
  lateral speed, and track ride height;
- separate normal and drift lateral-grip rates;
- relative collision mass, maximum energy, and a collision-damage multiplier.

These are direct simulation parameters rather than menu rating bars. Prototype 01
establishes provisional balanced values so later ships can be meaningfully larger,
heavier, more agile, drift-oriented, fragile, or durable. Tune them through
controlled driving and collision tests once the corresponding behavior exists;
the initial constants are not claims about final balance.

The prototype also gives each ship a small presentation profile. Its first
parameters control maximum visual turn roll and response speed. The roll follows
steering direction, grows with normalized speed, and eases back to level. It does
not change travel or collision behavior. Attached orientation composes this lean
with the sampled surface frame rather than assuming world-up.

Prototype 01's provisional planar steering authority uses `0.60 + 0.40 ×
sqrt(speed ratio)`. This preserves its configured full-speed steering rate while
providing substantially more yaw authority near rest and through middle speeds.
Treat this as playtest tuning, not the final track-relative steering model.

The first attached steering model deliberately remains small and is migration
scaffolding. The ship owns a
persistent signed heading relative to the path tangent. Steering rotates that
heading even without throttle; releasing steering does not snap it back to the
tangent. Forward speed is resolved into along-track and sideways components.
Normal or drift grip controls how quickly actual lateral velocity approaches the
sideways component, capped by the ship's maximum lateral speed.

The separate `speedway_physics` comparison now implements the intended
world-authoritative boundary. Steering rotates the physical ship while momentum
lags; grip removes local sideways velocity by a bounded per-second amount. LB/L1
and RB/R1 apply directional lateral forces even with neutral steering. The
existing paragraph above describes only the retained scalar regression.

Normal grip is a fixed lateral-speed removal budget, so it does not scale up to
cancel the larger velocity-direction error produced by steering at higher speed.
Prototype 01 is currently tuned to hold low-speed steering more readily, begin
slipping near its normal maximum, and lose still more directional authority in
the boost-speed envelope. Proper wall impacts and open-edge consequences are
still required before that loss of control has its final gameplay cost.

Directional drift must not be a constant sideways thruster. Its force fades as
same-direction lateral speed builds, while its reduced grip lets momentum keep
lagging behind the ship. Steering direction-change and drift force also suppress
positive propulsion. Sustained lateral slip builds a persistent response that
further weakens propulsion until grip recovers; local-axis damping then turns the
missing propulsion into speed loss without an unrelated direct-slip drag. This
combination is intended to create controllable instability and slowdown without
adding random steering noise.

As the path advances, its change in horizontal direction is removed from the
ship's relative heading. Consequently, entering a horizontal corner without
steering causes the road to turn underneath the ship and carries it toward the
outside edge. The player or AI must steer into the turn to hold a line. Changes
in the sampled surface normal still provide automatic pitch and roll, so banking
and a future vertical loop do not require an unrelated horizontal steering hack.
Along-track advancement also accounts for the shorter inside and longer outside
lane geometry.

Position and full 3D orientation are then derived from the sampled path frame.
Prototype 01's local box footprint is provisionally constrained inside the
sampled road width, preventing it from crossing through the surface edge before
wall impacts, open edges, falling, and recovery have explicit mechanics.

This does not make the course spline an AI-only rail. Human and AI controllers
both supply the same semantic throttle, brake, steering, directional-drift, and
boost values.
Course-level logic will later resolve a path ID and perform explicit transitions
at splits or joins. Jump takeoff retains world motion while support is lost;
landing selects an eligible path and refreshes the derived course reference.
Track zones and traps remain course/surface data consumed by the shared
simulation, not special cases embedded in an AI controller.

The first boost is deliberately a button-activated, one-second burst with no
energy or cooldown. A rising boost-action edge starts the timed fixed-step state
only while throttle is positive and brake is inactive. Holding the button cannot
retrigger it, so an ineligible press must be released and pressed again after the
conditions become valid. During the burst it adds acceleration up to a
ship-defined boosted speed ceiling, with both normal and boost acceleration scaled
by the analog throttle action.

Releasing throttle stops acceleration immediately and irreversibly caps that
burst to its ship-defined release tail. Scalar movement applies its configured
coasting term; world physics coasts through local forward damping. Prototype 01's
tail is 0.20 seconds, after which speed above the normal ceiling uses the movement
model's boost-excess return. Reapplying throttle during the tail does not restore
discarded burst time. The flare and any latched camera feedback remain active for
the short tail and then use their normal release envelopes. Braking cancels an
active burst immediately. This establishes deterministic mechanics and tuning
boundaries without prematurely designing the final energy system.

A successful boost activation emits one semantic simulation event. The current
single-player runtime maps that event to a 160 ms controller-rumble pulse. A press
without throttle, a press while braking, and a held boost action emit no event.
Keyboard or mouse activation can rumble an attached controller because feedback
follows the gameplay event rather than a particular physical button.

Boost camera feedback is presentation-only and speed-gated. When an active burst
reaches 65% of the ship's normal maximum speed, the response latches for the
remainder of that burst. It widens vertical FOV by at most 8°, pulls the follow
camera back by at most 0.6 m, and adds at most 0.5 m of look-ahead. It attacks over
roughly 0.1 seconds and releases over roughly one third of a second, allowing the
sensation to continue while excess boost speed returns toward normal. Boosting at
low speed still changes simulation but does not move the camera until the speed
threshold is crossed.

## Effective speed limits

A ship definition's base maximum forward speed is one input, not a universal or
final game cap. The effective limit and acceleration response will eventually be
resolved at runtime from relevant state, including:

- the selected ship's base handling;
- track surface or zone properties;
- track slope and the local direction of travel;
- attached, jumping, or airborne state;
- boost and other temporary effects.

Do not bake a single global maximum into rendering, input, track code, or UI. Do
not multiply an arbitrary pile of modifiers without specifying their order and
clamps either: introduce each factor alongside the observable track or airborne
behavior that needs it, then test the resulting effective-speed calculation. The
current 260 m/s base and 1.28× boost multiplier are provisional runway values that
can change without redefining the architecture.

## Track representation

The working model is a sampled path with a stable local coordinate frame:

- distance along track;
- center position;
- tangent, normal, and binormal;
- left and right width;
- banking and optional orientation controls;
- surface and zone properties.

Possible zone properties include boost, recharge, grip changes, jumps, damage,
checkpoints, and recovery hints. The representation should support straights,
banked curves, loops, corkscrews, half-pipes, full pipes, and tunnels without
special-casing race-position logic for each shape.

Prevent frame flips and discontinuities when constructing the sampled coordinate
basis. The exact spline and frame-transport algorithm remains an engineering
decision to validate with a generated test track.

The first concrete `SampledTrack` uses frames in `[0, length)`, wraps distance at
the closed seam, and treats binormal as track-right (`normal × tangent`). Its flat
stadium-oval generator provides exact straight/semicircle frames that are evenly
sampled and interpolated with re-orthonormalization. See `tracks.md` for the data
contract and current verification; general 3D frame transport remains unresolved.

## Fixed simulation

Game state advances at a fixed tick rate and is independent of rendered frame
rate. Rendering interpolates between simulation states. Input may be sampled per
rendered frame but must be consumed deterministically by simulation ticks.

A 120 Hz simulation is the initial implementation, not a permanent promise and not
the render rate. Benchmark and feel-test it against 120 Hz or higher when input
latency and full race CPU costs can be measured. Rendering at 24, 30, 60, 90, 120,
144, 165, 240, 360, or other rates must not change vehicle acceleration, grip, lap
times, or AI behavior.

F-Zero X's NTSC handling was tuned around a 60 Hz update (50 Hz for PAL). Any
qualitatively useful per-frame behavior from that reference must be converted to
per-second or time-correct coefficients and independently tuned; numeric constants
must not be copied directly into the 120 Hz prototype.

## Race systems

After driving feels good, add the smallest complete race loop:

- starting grid and countdown;
- checkpoints and lap validation;
- timing and finish order;
- race position;
- restart and recovery;
- a time-trial ghost;
- a minimal HUD.

Race position should use progress along the track plus validated lap/checkpoint
state, rather than world-space proximity to the finish line.

## Opponent AI

AI operates primarily in track coordinates. A controller can choose target speed,
target lateral position, preferred gaps, overtaking side, and aggression, then use
the same vehicle-control interface as the player where practical.

CPU racers must understand the same future path graph and attached/airborne state
transitions as the player. Their planning needs to choose branches and shortcuts,
prepare for takeoff, target a valid landing path, recover from missed jumps, and
continue after paths rejoin. Keep route/behavior decisions above the shared
per-tick vehicle controls rather than inventing AI-only movement physics.

Build AI incrementally: one opponent, then five, ten, and roughly thirty. Add
behavior in layers: follow a racing line, react to track features, identify slower
traffic, choose an overtake, avoid collisions, and only later add attacks or
characterful mistakes.

Thirty racers are a product goal, not an excuse to optimize unmeasured code.
Profile representative races before introducing specialized concurrency or data
architectures.

## Camera and sensation of speed

The camera is part of handling. Tune follow distance, field of view, banking,
look-ahead, lag, shake, and collision response alongside the vehicle rather than
treating them as late polish. Avoid effects that obscure steering feedback.

## Debug presentation

Early builds need readable diagnostics, not finished UI. Useful values include:

- rendered frames per second and frame time;
- simulation tick rate and dropped/catch-up ticks;
- speed and acceleration;
- track distance and lateral offset;
- lap, checkpoint, and recovery state;
- current input values.

Menus, final HUD art, championship structure, vehicle roster, and progression are
deliberately unspecified until the driving prototype is validated.
