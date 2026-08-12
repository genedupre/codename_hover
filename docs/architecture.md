# Technical architecture

## Architectural intent

Build a portable runtime for one racing game. Every proposed subsystem should
answer a current game requirement. Generality is not a goal and extraction into a
separate engine repository is not planned.

## Baseline stack

| Concern | Choice |
| --- | --- |
| Language | C++23 |
| Build | CMake and Ninja |
| Platform/window/input/audio | SDL3 3.4.10 for bootstrap |
| Graphics abstraction | SDL_GPU |
| Shader source | HLSL |
| Shader translation | Pinned SDL_shadercross host CLI, offline during builds |
| Development assets | Programmatic primitives first; Blender and glTF/GLB later |
| Version control | Git |
| Primary toolchain | Clang and clangd on Linux |
| Formatting/static analysis | clang-format and focused clang-tidy checks |
| Debug UI | Minimal custom debug text; Dear ImGui only if debug tooling earns it |

SDL is a Git submodule at an exact release commit and is included with
`add_subdirectory`. Bootstrap builds SDL statically and disables SDL tests and
install targets. FetchContent is not the baseline because it adds configure-time
network access and is less convenient for later platform-specific SDL forks.

SDL_shadercross is a separate build-time tool, not a game library or runtime
dependency. Use the exact official build recorded in `development-hardware.txt`.
Third-party libraries should otherwise be vendored or pinned through a
reproducible mechanism and wrapped only when doing so isolates volatility or a
real platform boundary.

## Runtime boundaries

The expected high-level areas are:

```text
game
  race, vehicle, track, AI, camera, HUD
        |
simulation
  fixed-step state and deterministic game math
        |
render                 platform
  meshes, pipelines     SDL startup, window, input, audio, files
        |                    |
      SDL_GPU               SDL3
        |
  Vulkan / D3D12 / Metal / platform backend

services
  achievements, saves, cloud, leaderboards
  implemented by null/development, Steam, or future platform adapters
```

These are responsibility boundaries, not a demand for one class or directory per
label. Prefer plain values, direct ownership, and small functions over framework
machinery.

## Tentative source layout

Create directories only as code needs them:

```text
src/
  main.cpp
  core/
  platform/
  render/
  assets/
  game/
  services/
assets/
shaders/
tests/
tools/
```

There is no initial requirement for an ECS, service locator, dependency-injection
framework, reflection system, scripting language, or general scene graph. A fixed
container such as `std::array<Vehicle, max_vehicles>` is acceptable when it models
the problem clearly.

## Main loop policy

- Poll platform events and sample input.
- Accumulate elapsed real time with a bounded catch-up policy.
- Run zero or more fixed simulation ticks.
- Interpolate visual transforms between the previous and current simulation state.
- Render once and present.
- Record timing data needed to diagnose stalls and missed simulation deadlines.

Do not scatter variable frame delta through gameplay code. Define pause, focus
loss, long-stall, and simulation catch-up behavior explicitly when the loop is
implemented.

## Development scenario boundary

One executable may boot named, deterministic development scenarios through
`--scenario NAME`. A scenario selects the smallest world/configuration needed for
an interactive experiment while reusing the real SDL initialization, input,
simulation, renderer, and shutdown paths. It is not a separate executable, general
scene graph, scripting layer, or project milestone.

Scenario names are parsed into a closed enum so unknown names fail clearly. Keep
the registry small and retain a scenario when it remains a useful regression or
hardware diagnostic. The `runway` scenario is the current default and permanent
free-driving/input sandbox. `oval` is the flat attached-motion reference and
`speedway` is the first map prototype with banked turns. Both use the same
track-vehicle simulation; a scenario supplies geometry and spawn configuration,
not a parallel movement implementation.

## Input boundary

SDL keyboard, mouse, and gamepad state is translated by platform code into
semantic `PlayerInput` values before reaching simulation. All connected input
classes remain active together. Vehicle logic consumes normalized steering,
throttle, brake, drift, and boost values and does not know which device produced
them. See `input.md` for merge rules and current prototype bindings.

## Rendering boundary

Keep the public rendering vocabulary small: device/frame lifecycle, buffers,
meshes, textures, pipelines, cameras, instanced draws, and debug/UI draws. SDL_GPU
owns cross-API translation. Do not add direct Vulkan, D3D12, or Metal paths unless
a measured, shipping requirement cannot be met through SDL_GPU.

Author shaders in HLSL and compile them before runtime. Produce SPIR-V for Vulkan,
DXIL for D3D12, and MSL for Metal. The first Linux triangle needs only SPIR-V, but
the source and build interface must not assume SPIR-V is the only eventual output.

See `rendering.md` for presentation and performance requirements.

## Platform services

Game code calls narrow capabilities such as unlocking an achievement, submitting
a leaderboard score, or saving data. Steamworks, Xbox, and PlayStation APIs must
remain inside platform-specific implementations. A null or local development
implementation should allow the game to run without a store client.

Do not design every hypothetical platform method now. Introduce an interface when
the first concrete integration requires it, while keeping platform calls out of
race and vehicle logic.

## Assets

Prototype meshes may be generated in C++ to avoid learning several toolchains at
once. Later, Blender is the source editor and GLB/glTF is the interchange format.
Track-specific metadata may initially accompany a mesh as explicit data. Add a
compiled runtime format only after profiling shows a real loading or deployment
need.

All mesh sources converge on the same CPU-side `MeshData` value containing the
renderer's vertex and index data. `GpuMesh` validates and uploads that value, but
does not know whether it came from generated C++, a future GLB loader, or another
approved asset source. This is the interchange boundary inside the runtime; do not
make the renderer depend on ship-specific generators or Blender concepts.

Track sources similarly converge on `game::SampledTrack`: ordered distance frames
with center, tangent, normal, track-right binormal, and width. The initial analytic
oval is a source of those frames, not a special runtime track type that vehicle or
renderer code should depend upon. See `tracks.md`.

The current attached prototype uses `TrackVehicleState`, which stores an
opaque path ID, shared vehicle dynamics, and scalar values in that path's local
frame. It does not own or point at track geometry. Scenario/course ownership
resolves the ID to a `ResolvedTrackPath` for one tick, which permits a future
graph to switch paths at splits and joins. Banks, vertical sections, and loops
derive the rendered pose and camera up direction from the frame's tangent,
normal, and binormal rather than world-up.

That provisional step shares propulsion, braking, boost state, events, and visual
steering response with free-planar movement. It owns a persistent signed heading
within the local surface plane. Semantic steering rotates that heading; lateral
grip approaches its sideways velocity component. Path progress uses the forward
component and the physical length scale of the selected lateral lane. Change in
the path's tangent around its surface normal is subtracted from relative heading,
so horizontal curvature must be actively steered while surface pitch and roll
remain automatic. The step then applies a provisional collider-aware road-width
constraint and derives a full 3D pose.

The accepted replacement keeps one authoritative world position, velocity, and
physical basis in supported, airborne, and falling modes. It predicts world
movement first, then uses a bounded, hint-aware course projection to derive path
progress, lateral offset, height, and contact response. A separate visual basis
may lag or embellish the physical basis without affecting collision. The current
scalar traversal remains only until this replacement is proven in small slices.
See `physics.md`; graph path eligibility, surface zones, and contact transitions
are not implemented yet.

A ship's gameplay `ShipDefinition` is separate from its visual mesh. It contains a
stable identity and visual-mesh key plus handling, presentation, and collision
profiles. The small presentation profile currently owns speed-scaled visual turn
roll rather than placing that tuning in the renderer. The collision profile owns
local-space bounds, relative mass, energy, and collision-damage response.
Replacing a generated prototype mesh with a Blender-authored GLB must not silently
replace or derive these gameplay values.

`HandlingProfile::base_maximum_forward_speed_metres_per_second` is deliberately a
ship baseline, not a global cap. Vehicle/course simulation will own the eventual
effective-speed calculation when track zones, slope, jumps, and airborne motion
exist. Presentation consumes normalized values supplied from that state and must
not independently impose gameplay speed limits.

## Error handling and diagnostics

- Fail startup with a clear message when required GPU or asset initialization
  fails.
- Include relevant SDL error text and resource names in diagnostics.
- Keep debug validation enabled in development configurations where supported.
- Avoid exceptions crossing platform or C API boundaries unless the project later
  adopts a documented exception policy.
- Make ownership and cleanup visible; use RAII for acquired resources.

The precise error/result type is still open and should be chosen when enough code
exists to evaluate it.
