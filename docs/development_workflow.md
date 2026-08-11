# Development workflow

## Primary environment

The laptop is the development machine. The Steam Deck is deployment and test
hardware. The intended local toolchain is deliberately conventional:

- Linux;
- Neovim or another preferred editor with clangd;
- Clang;
- CMake and Ninja;
- gdb or lldb;
- Git;
- clang-format and selected clang-tidy checks;
- RenderDoc later where the active backend supports the needed capture workflow;
- Blender later for authored assets.

The repository should document exact supported versions once bootstrap work proves
them. Do not install a large dependency set before the corresponding milestone
needs it.

## Bootstrap commands

After installing the packages recorded in `development-hardware.txt`, initialize
the pinned source dependency and host shader tool:

```bash
git submodule update --init --recursive
tools/fetch-shadercross-linux-x64.sh
```

Configure and build with the checked-in presets:

```bash
cmake --preset development
cmake --build --preset development
```

Generated build files and local host tools stay under ignored `build/` and
`.tools/` directories. CMake must not download dependencies during configure.

## Development scenarios

Boot a small named test world through the same executable and production startup
path:

```bash
./build/development/codename_hover --scenario runway
```

The Linux build already prefers native Wayland with X11 fallback. Use an
environment override only for deliberate backend diagnostics, for example:

```bash
SDL_VIDEO_DRIVER=x11 ./build/development/codename_hover --scenario runway
```

Discover the available names without initializing SDL or opening a window:

```bash
./build/development/codename_hover --list-scenarios
./build/development/codename_hover --help
```

Call these scenarios rather than stages: a stage can be confused with a loading
phase or roadmap milestone. Keep one executable and shared runtime path. Each
scenario should only select its generated world, spawn/configuration, and relevant
diagnostics; it must not grow a parallel engine or duplicate game systems.

Current scenarios:

- `runway`: Prototype 01, the long presentation runway, free planar movement,
  follow camera, and full input/timing diagnostics.
- `oval`: Prototype 01 at the highlighted seam of a generated closed stadium
  track, with the same provisional free planar movement and diagnostics.

Running without `--scenario` currently defaults to `runway`. The eventual normal
game/menu boot may replace that default without removing explicit development
scenario selection.

## Inner development loop

1. Choose one observable behavior from `roadmap.md`.
2. Make a small change with a clear ownership boundary.
3. Build with warnings enabled.
4. Run focused automated tests for deterministic logic.
5. Run the game locally and inspect logs/validation output.
6. Deploy to Steam Deck when input, display, performance, or platform behavior is
   relevant.
7. Record game-feel observations in `../DEVLOG.md`.
8. Commit a coherent, reviewable result.

The desired eventual Deck loop is one documented command or small script that
builds, incrementally deploys with rsync, launches, and makes logs easy to retrieve.
The Deck is initially a playtest target, not a compilation host. A Valve Devkit
Client workflow can still be reconsidered if it offers a concrete advantage over
the planned SSH/rsync path.

## Git practice

Prefer small commits such as “open SDL window,” “create GPU device,” or “add
controller exit action.” Avoid enormous “initial game” commits. Do not mix broad
refactoring with gameplay tuning. Preserve a runnable checkpoint before risky
experiments.

AI-generated work is reviewed under `ai_collaboration.md` before being accepted.

## Build configurations

Plan for at least:

- a development configuration with symbols, assertions, GPU validation where
  available, and useful logs;
- a release configuration with representative optimization and no dependency on
  development-only assets or tools.

Sanitizer configurations and deterministic test executables should be added when
the first applicable code exists. Avoid configuration proliferation before then.

## Testing strategy

Automate code that has deterministic expected results:

- vector and coordinate-frame math;
- spline sampling and distance wrapping;
- track progress and checkpoint validation;
- fixed-step accumulation;
- input mapping transformations;
- vehicle state transitions;
- race ordering and lap timing.

Interactive tests remain essential for controller feel, camera comfort, display
modes, frame pacing, asset appearance, audio, suspend/resume, and game fun. Record
which devices and settings were actually tested.

## CI progression

Add CI incrementally:

1. Linux configure, compile, and tests.
2. Windows configure, compile, and tests.
3. Formatting/static checks that are stable and actionable.
4. macOS build if macOS remains a supported target.
5. Packaging smoke checks when release artifacts exist.

A graphical test does not have to run in CI to gain value from cross-platform
compilation and unit tests. Do not claim runtime support based only on a green
compile job.

## Asset workflow

Start with code-generated triangle, ship, and track geometry. This isolates runtime
learning from Blender learning. Once driving works, use Blender for low-poly
vehicle/environment assets and GLB/glTF as development interchange.

Blender Python scripts are welcome for repeatable procedural work, such as track
prototypes or export validation. They should be kept in the repository, reviewed,
and documented like other tools. Do not create a proprietary asset database or
runtime binary format until profiling demonstrates the need.

## Learning approach

Learn game math, graphics concepts, Blender, and C++ techniques when a milestone
needs them. Each explanation should connect the concept to visible behavior in the
current build. The owner should be able to answer why a file exists, what a system
owns, what enters and leaves it, when it runs, and how failure is reported, even
when AI wrote most of the implementation.
