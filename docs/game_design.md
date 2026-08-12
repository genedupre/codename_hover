# Game design

## Design priority

Handling comes before content. A racing game with many tracks and mediocre driving
is not successful. Early builds may contain only a primitive vehicle, a plain
track, a speed readout, and reset controls while acceleration, steering, grip,
drift, boost, collision response, recovery, and camera behavior are tuned.

## Vehicle model

Use specialized arcade vehicle logic rather than asking a general rigid-body
engine to simulate the normal driving state. Most of the time a vehicle is related
to a known track surface. It may become genuinely airborne for jumps, crashes, or
loss of contact, then reacquire an appropriate track surface.
when there are jumps, there could be a possability to jump to another track (a short shortcut) which at a later point joins the main track again.

The first concrete track-bound vehicle state includes:

- an opaque current path identity and distance along it;
- lateral offset and lateral velocity in the path frame;
- normal offset and normal velocity relative to the local surface;
- forward speed.

Energy-backed boost limits, damage, lap, checkpoint, recovery, and airborne state
remain later additions. Local normal motion is intentionally not called
world-vertical motion:
the ship may be banked, vertical, or upside down on loops. A true jump that leaves
the track will later transition to world-space airborne state and may reacquire a
different eligible path, including a shortcut or branch.

Longer-term vehicle state also needs:

- orientation relative to the local track frame;
- energy, boost, and damage state;
- current lap, checkpoint, and recovery state.

The exact model is not yet specified. Add only the state required by observable
handling behavior, and record important tuning results in `../DEVLOG.md`.

Ship types use explicit definitions rather than deriving gameplay behavior from
their render meshes. The current definition boundary supports:

- a stable ship identity and visual-mesh reference;
- a local-space box collider sized for that ship;
- maximum forward speed, acceleration, braking, and steering rate;
- separate normal and drift lateral-grip rates;
- relative collision mass, maximum energy, and a collision-damage multiplier.

These are direct simulation parameters rather than menu rating bars. Prototype 01
establishes provisional balanced values so later ships can be meaningfully larger,
heavier, more agile, drift-oriented, fragile, or durable. Tune them through
controlled driving and collision tests once the corresponding behavior exists;
the initial constants are not claims about final balance.

The runway prototype also gives each ship a small presentation profile. Its first
parameters control maximum visual turn roll and response speed. The roll follows
steering direction, grows with normalized speed, and eases back to level. It does
not change planar travel or collision behavior; future track-relative orientation
will compose ship lean with the sampled surface frame rather than assuming
world-up.

Prototype 01's provisional planar steering authority uses `0.60 + 0.40 ×
sqrt(speed ratio)`. This preserves its configured full-speed steering rate while
providing substantially more yaw authority near rest and through middle speeds.
Treat this as playtest tuning, not the final track-relative steering model.

The first boost is deliberately a button-activated, one-second burst with no
energy, cooldown, or camera effect. A rising boost-action edge starts the timed
fixed-step state; holding the button cannot retrigger it, so the player must
release and press again. During the burst it adds acceleration up to a ship-defined
boosted speed ceiling. After the burst, speed above the normal ceiling decays at a
separate ship-defined rate and settles exactly at the normal ceiling before
ordinary throttle/coasting rules resume. Braking cancels an active burst. This
establishes deterministic mechanics and tuning boundaries without prematurely
designing the final energy system.

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

A 90 Hz simulation is the initial implementation, not a permanent promise and not
the render rate. Benchmark and feel-test it against 120 Hz or higher when input
latency and full race CPU costs can be measured. Rendering at 24, 30, 60, 90, 120,
144, 165, 240, 360, or other rates must not change vehicle acceleration, grip, lap
times, or AI behavior.

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
