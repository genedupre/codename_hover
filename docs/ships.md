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

ShipDefinition -> identity, visual-mesh key, handling, collider, mass, energy
```

- `render::MeshData` is CPU-owned position, flat normal, color, and 32-bit index
  data. Every supported visual source must produce this value.
- `render::GpuMesh` validates and uploads a `MeshData`. It owns SDL_GPU vertex and
  index buffers and does not know which ship or source produced them.
- `game::ShipDefinition` owns gameplay-facing data and references a visual mesh by
  stable key. It never owns GPU resources.
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
dark canopy, twin rear engine pods, and colored exhaust faces. Flat shading uses
one duplicated normal per triangle. The current mesh is 96 triangles; this is
small enough that duplication is intentional and harmless.

Its provisional definition currently specifies:

| Property | Value |
| --- | ---: |
| Visual mesh key | `generated/prototype_01` |
| Maximum forward speed | 260 m/s |
| Forward acceleration | 78 m/s² |
| Braking deceleration | 105 m/s² |
| Steering rate | 1.65 rad/s |
| Normal lateral grip | 7.0/s |
| Drift lateral grip | 2.4/s |
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

## Known missing pieces

- `main.cpp` explicitly loads Prototype 01; there is no roster or asset registry.
- There is no per-object model transform or mesh instancing yet.
- The current vertex format has position, normal, and color but no UV coordinates.
- Colliders and handling values are validated as data but not simulated or drawn.
- Damage, explosions, boost, hover behavior, and ship selection do not exist.
- GLB loading and Blender export conventions remain future work.

Introduce each missing piece when a playable checkpoint needs it. The next one is
a model transform driven by fixed-step keyboard movement.
