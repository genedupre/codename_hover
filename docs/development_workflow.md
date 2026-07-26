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
builds, incrementally deploys, launches, and makes logs easy to retrieve. Use the
official SteamOS Devkit Client workflow if it fits the development environment;
otherwise keep any SSH/rsync alternative minimal and explicit.

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
