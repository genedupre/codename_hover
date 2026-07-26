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

Candidate vehicle state includes:

- distance along the current track path;
- lateral offset and velocity;
- hover height and vertical velocity;
- forward speed and acceleration;
- orientation relative to the local track frame;
- energy, boost, and damage state;
- current lap, checkpoint, and recovery state.

The exact model is not yet specified. Add only the state required by observable
handling behavior, and record important tuning results in `../DEVLOG.md`.

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

## Fixed simulation

Game state advances at a fixed tick rate and is independent of rendered frame
rate. Rendering interpolates between simulation states. Input may be sampled per
rendered frame but must be consumed deterministically by simulation ticks.

A 90 Hz simulation is the initial aim, not a permanent promise, since we want wide capability (30, 60, 244hz). Benchmark
and feel-test it against other rates. High rendering rates must not give different
vehicle acceleration, grip, lap times, or AI behavior.

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
