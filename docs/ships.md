# Ship system

## Purpose

Ships vary in both appearance and gameplay. A wider visual hull may need a wider
collider; another ship may accelerate harder, steer faster, retain more grip while
drifting, survive more damage, or react differently in collisions. Keep those
choices explicit and reviewable rather than inferring gameplay from render
geometry.

## Current boundary

```text
generated C++ mesh ----\
                       -> MeshData -> GpuMesh -> SDL_GPU draw
future Blender GLB ----/

ShipDefinition -> identity, visual-mesh key, handling, presentation, collider
```

- `render::MeshData` is CPU-owned position, flat normal, color, opacity, and
  32-bit index data. Every supported visual source must produce this value.
- `render::GpuMesh` validates and uploads a `MeshData`. It owns SDL_GPU vertex and
  index buffers and does not know which ship or source produced them.
- `game::ShipDefinition` owns gameplay-facing data and references a visual mesh by
  stable key. Its small presentation profile holds ship-specific visual handling
  response such as turn roll. It never owns GPU resources.
- Generated mesh functions live under `src/assets/generated/`. A future GLB loader
  belongs under `src/assets/` and should produce the same `MeshData`.
- Per-racer simulation state will reference a ship definition. Racers using the
  same visual should share one uploaded `GpuMesh` rather than upload duplicate
  buffers.

This is deliberately a value boundary, not a generator inheritance hierarchy,
asset database, or general engine framework.

## Coordinates and units

Ship-local coordinates currently use:

- positive X: ship right;
- positive Y: up;
- positive Z: forward/nose;
- metres for visual geometry, collider dimensions, speeds, and accelerations;
- radians for steering rates;
- per-second rates for lateral grip;
- relative rather than physical kilograms for collision mass;
- game-defined energy units for durability.

The local box collider moves and rotates with the ship once simulation transforms
exist. Decorative geometry should normally remain inside it. Tests enforce that
contract for Prototype 01.

## Prototype 01

Prototype 01 is the first balanced baseline, not a final name or finished design.
Its generated dart/manta-style mesh contains a faceted central hull, swept wings,
50%-transparent blue canopy glass, twin rear engine pods, colored exhaust faces,
and a small three-dimensional driver silhouette beneath the glass. Flat shading
uses one duplicated normal per triangle. The opaque ship shell is 90 triangles,
the glass is 6 triangles, and the driver is 20 triangles; this is small enough
that duplication is intentional and harmless.

Two separate 12-triangle low-poly plumes are drawn at each engine socket only
during positive propulsion input. A light-blue opaque core sits inside a larger,
longer light-blue shell with 50% opacity. Normalized forward speed strongly grows
both radial and longitudinal scale and increases its two presentation-time pulse
frequencies. Releasing propulsion drives a frame-rate-independent 0.2-second
presentation envelope so the plume becomes very small before disappearing. This
visual sampling does not alter simulation. The sockets are Prototype 01-specific
constants for now; make them ship visual data when another ship or
Blender-authored mesh requires the same effect.

Its provisional definition currently specifies:

| Property | Value |
| --- | ---: |
| Visual mesh key | `generated/prototype_01` |
| Base maximum forward speed | 260 m/s (provisional) |
| Forward acceleration | 78 m/s² |
| Braking deceleration | 180 m/s² |
| Coasting deceleration | 90 m/s² |
| Steering rate | 1.90 rad/s |
| Maximum attached lateral speed | 42 m/s |
| Normal lateral grip | 7.0/s |
| Drift lateral grip | 2.4/s |
| World-space normal lateral grip | 300 m/s² |
| World-space drift lateral grip | 55 m/s² |
| Directional drift acceleration | 105 m/s² |
| Directional drift force fade speed | 32 m/s |
| Drift steering multiplier | 1.15× |
| Full-steer propulsion loss | 35% |
| Full-force drift propulsion loss | 85% |
| Drift forward deceleration | 60 m/s² |
| Lateral slip threshold | 8 m/s |
| Slip forward loss | 4 m/s² per excess lateral m/s |
| Track ride height | 0.62 m |
| Boost speed multiplier | 1.28× (332.8 m/s ceiling) |
| Boost acceleration | 145 m/s² |
| Excess boost-speed decay | 170 m/s² |
| Boost burst duration | 1.0 s |
| Boost throttle-release tail | 0.20 s |
| Maximum visual turn roll | 0.18 rad (~10.3°) |
| Visual turn-roll response | 8.0/s |
| Relative collision mass | 1.0 |
| Maximum energy | 100 |
| Collision damage multiplier | 1.0 |

The collider has local center `(0.0, 0.10, 0.21)` and half-extents
`(2.0, 0.58, 2.72)` metres. These values are direct future simulation inputs, not
menu rating bars. They remain provisional until driving and collision behavior can
be playtested.

Owner feedback on 2026-08-11 was that the result was recognizably a ship but needs
substantial visual improvement. Preserve it as a functional baseline; do not treat
its silhouette, palette, name, or proportions as accepted final art direction.

## Adding or replacing a ship

For the next generated ship:

1. Add a uniquely named mesh-producing function under `src/assets/generated/`.
2. Add a definition under `src/game/ships/` with a stable ID and visual-mesh key.
3. Validate positive gameplay parameters, valid indexed geometry, unit normals,
   and visual containment inside the declared collider.
4. Upload one GPU mesh per visual type and share it between racer instances.
5. Visually inspect silhouette, lighting, scale, and collider assumptions.

When Blender begins, keep the definition and mesh key stable where practical and
change the key's source from generated C++ to an exported GLB. Do not derive the
collider or handling automatically merely because the authored mesh changed.

## Current provisional simulation

Prototype 01 consumes semantic input at a fixed 120 Hz in three movement modes. The
`runway` keeps its planar free-driving simulation. `oval` and `speedway` share an
attached simulation that advances path distance, uses speed-scaled lateral
heading and grip for steering, keeps the provisional box footprint inside the
road width, and derives its full pose from the sampled surface frame. Attached
heading persists without throttle or steering, and horizontal track curvature
must be steered rather than being followed automatically. Both modes
share acceleration, braking, coasting, boost, and smoothed visual turn roll.
Rendering interpolates the previous/current full 3D pose; the camera follows its
surface-relative up direction without inheriting the additional visual turn roll.

`speedway_physics` is the isolated replacement experiment. It owns world position,
velocity, and physical orientation; steering rotates the ship without directly
rotating momentum. Normal grip removes up to 300 m/s² of local sideways velocity.
Because that is a fixed per-second limit, low-speed turning can remain planted
while base-maximum and boosted turning produce progressively more slip. This
value was reduced from 420 m/s² after boosted cornering proved too forgiving in
the first owner playtest and remains provisional.
LB/L1 or Q and RB/R1 or E apply opposite 105 m/s² lateral forces, use the lower
55 m/s² drift grip, and multiply steering by 1.15. The side force fades as the
ship approaches 32 m/s of lateral motion in the requested direction instead of
accelerating sideways forever. Steering and active drift suppress positive
propulsion, drift applies 60 m/s² forward loss, and sustained lateral slip above
8 m/s adds proportional forward loss. These values are playable starting points,
not accepted final balance.

Pressing boost currently starts a one-second burst that accelerates above
Prototype 01's provisional 260 m/s base ceiling to at most 332.8 m/s. Holding X
does not retrigger it; X must be released before another press. When the burst
ends, excess speed returns smoothly to the normal ceiling instead of snapping the
ship down. Braking cancels a burst. This initial mechanic has no energy cost or
cooldown and is intended for handling validation, not final balance. Its 145 m/s²
boost acceleration is the full-throttle value and scales with analog throttle.
Activation requires positive throttle and no brake. Releasing throttle during a
burst applies the existing 90 m/s² coasting deceleration from whatever boosted
speed the ship has reached and shortens the remaining burst to at most 0.20
seconds. The short presentation tail remains, but reapplying throttle does not
restore the discarded burst time or force the ship immediately back to its base
ceiling.

The prototype camera adds a speed-gated response shared by the current scenario
camera rather than storing camera numbers in `ShipDefinition`. It activates at
65% of this ship's normal maximum speed during a boost, latches for that burst,
and releases smoothly afterward. Revisit whether camera feedback belongs to a
global accessibility/presentation profile or has limited ship-specific tuning
only when multiple ship identities require different behavior.

This is the first intended track-relative architecture, but its handling constants,
hard ride-height attachment, width constraint, and camera feel are provisional.

## Known missing pieces

- `main.cpp` explicitly loads Prototype 01; there is no roster or asset registry.
- There is no mesh instancing yet.
- The current vertex format has position, normal, color, and opacity but no UV
  coordinates.
- The collider footprint limits attached lateral position, but vehicle/track and
  vehicle/vehicle collision response is not simulated or drawn.
- Damage, explosions, boost energy/cooldown, hover behavior, and ship selection do
  not exist.
- GLB loading and Blender export conventions remain future work.

Introduce each missing piece when a playable checkpoint needs it. The next physics
work is interactive attached-handling validation, followed by explicit edge,
airborne, or collision behavior chosen from those observations.
