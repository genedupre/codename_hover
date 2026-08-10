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
