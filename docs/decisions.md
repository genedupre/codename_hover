# Decisions and open questions

This file records project-level choices that affect more than one subsystem. A
decision can be changed when evidence justifies it; update the affected topical
documents at the same time. Do not treat imported research claims as accepted
facts merely because they appear in the planning documents.

## Accepted baseline decisions

### D-001: Build a game-specific runtime

Status: accepted.

Codename Hover is the product. Do not create or name a separate general-purpose
engine. Shared code exists only to serve the game.

### D-002: Use C++23, CMake, and Ninja

Status: accepted for bootstrap.

This matches the desired native, compact, cross-platform runtime and conventional
tooling. Reconsider only with concrete toolchain or platform evidence.

### D-003: Use SDL3 and SDL_GPU

Status: accepted for bootstrap.

SDL3 owns platform/window/input/audio concerns and SDL_GPU owns modern graphics API
abstraction. Direct per-API renderers are out of scope unless SDL_GPU fails a
measured shipping requirement.

### D-004: Develop Linux-first and test on Steam Deck

Status: accepted.

The laptop is the development machine; the Deck remains clean target hardware.
Native Linux is a product requirement. Windows follows early enough to protect
portability.

### D-005: Separate fixed simulation from rendering

Status: accepted.

Gameplay must not depend on rendered frame rate. The prototype simulation runs at
90 Hz and interpolates rendering, including when rendering is slower or faster
than simulation. Treat 90 Hz as a provisional tuning choice, not the Steam Deck's
refresh rate and not a rendering limit.

Render correctly at common low and high rates. Presentation and limiter policy is
recorded separately in D-015.

### D-006: Use specialized track-relative vehicle physics

Status: accepted as the first prototype approach.

Do not add a general rigid-body engine initially. Vehicles may transition to a
genuinely airborne state where required.

### D-007: Use generated prototype geometry, then Blender

Status: accepted.

Generate the triangle, first ship, and first track in code. Introduce Blender and
GLB/glTF after the driving path works. Do not build a custom editor.

### D-008: Preserve an original identity

Status: accepted and non-negotiable.

The visual era and broad genre may inspire the project, but protected Nintendo
content must not be copied.

### D-009: mod support
eventually, we should be able to have mod support (eg people can make changes to the game, add vechiles, maps)
how this would like, we can decide later.

### D-010: Pin stable SDL as a Git submodule

Status: accepted for bootstrap on 2026-07-27.

Use SDL 3.4.10 at commit `8e37db5e797b6167f3a00d697d816a684bd259c7`
under `external/SDL` and include it with CMake `add_subdirectory`. Build the static
SDL target and disable SDL tests and install targets for the game build.

Git submodules make the exact source visible and avoid downloading dependencies
during CMake configure. They also give a clearer future replacement point for
platform-specific SDL forks than a hard-coded FetchContent declaration.

### D-011: Use SDL_shadercross only as a pinned host tool

Status: accepted for bootstrap on 2026-07-27.

SDL_shadercross has no official releases or tags. Pin source commit
`e55cf5e31ced6f3d1be5cc6d0c50e99384f9f4ba` and use the official Linux x64
artifact from Actions run `28236415347` for that commit. The verified artifact and
executable hashes are recorded in `development-hardware.txt`. That pinned source
declares SDL 3.1.3 as its minimum, which is satisfied by the selected SDL 3.4.10.

Do not link SDL_shadercross into the game or compile shaders at runtime. Keep the
tool and its bundled libraries in a repository-local ignored tools directory. An
acquisition script must verify hashes. If the GitHub artifact expires, build the
same pinned commit with its recorded submodules or publish a verified project-owned
mirror; do not silently move to current `main`.

### D-012: Compile HLSL into backend-specific shader assets offline

Status: accepted for bootstrap on 2026-07-27.

HLSL is the only authored shader language. SDL_shadercross produces SPIR-V for the
Linux/Vulkan build, DXIL for Windows/D3D12, and MSL for macOS/Metal. The game loads
only compiled assets matching the selected SDL_GPU backend. The first triangle
compiles SPIR-V only, while the build interface preserves the other outputs for
later platform jobs.

### D-013: Keep controller and keyboard/mouse input active together

Status: accepted.

Support the broad range of controllers exposed through SDL's gamepad APIs,
including hotplugging and rumble where hardware capabilities allow it. Rumble must
be optional and must never carry gameplay information that has no visual or audio
equivalent.

Keyboard and mouse input remain active at the same time as every connected
controller. Never force the player into an exclusive input mode or require a menu
toggle to change devices. UI glyphs may follow the last meaningful input, but that
presentation choice must not disable or discard events from other devices.

Test representative Xbox, PlayStation, Nintendo-style, Steam Deck, and common
generic controllers as hardware becomes available. Device-specific features are
progressive enhancements rather than requirements for basic play.

SDL3 is the baseline input provider. Platform code converts every device into
semantic analog/digital actions before simulation. Signed axes merge by greatest
absolute magnitude, unsigned axes by maximum, and buttons by logical OR; inputs
are never summed. Steam Input may later feed the same boundary but is neither a
runtime requirement nor permission to place Steam calls in gameplay code.

### D-014: Separate ship definitions from visual mesh sources

Status: accepted on 2026-08-11.

Each ship has an explicit gameplay definition containing its identity, visual-mesh
key, handling parameters, local collider, collision mass, energy, and damage
response. Gameplay values are not inferred from render geometry.

Generated C++ geometry and future Blender/GLB assets both produce the same generic
CPU `MeshData`, which is then uploaded as a renderer-owned `GpuMesh`. Adding,
removing, or replacing a visual source must not require a new renderer path or
silently change the associated gameplay definition. Do not add a generator class
hierarchy or proprietary asset database until a concrete source requires more than
this value boundary.

### D-015: Treat 240 FPS as a target, not an engine ceiling

Status: accepted on 2026-08-11.

Support 240 FPS as an important performance target and explicit limiter choice,
but do not impose it as the maximum render rate. The settings milestone should
offer refresh-matched/VSync behavior, common explicit limits such as 30, 60, 90,
120, 144, 165, 240, and 360 FPS, plus uncapped rendering. Refresh matching covers
displays with nonstandard or fractional rates without requiring every rate to be a
preset.

Expose only SDL_GPU presentation modes supported by the active window/device.
VSync is the portable fallback; mailbox and immediate are latency/tearing choices,
not universally available upgrades. Treat frames in flight, limiter pacing, frame-
time variance, power draw, and end-to-end input latency as measured concerns.

The bootstrap remains VSync-presented with no custom frame limiter. The 90 Hz
fixed simulation remains independent and provisional; compare higher simulation
rates later using game feel, latency, deterministic behavior, and representative
full-race CPU cost. Do not increase simulation frequency merely to match a monitor.

### D-016: Prefer native Wayland on Linux with X11 fallback

Status: accepted on 2026-08-11.

When the user has not explicitly selected an SDL video driver, Linux builds request
`wayland,x11` in that order before SDL initialization. This preserves an X11
fallback and user diagnostic overrides.

The decision follows an interactive Steam Controller diagnosis on GNOME Wayland.
Steam's desktop layout emitted a synthetic `W` through XTEST when the left stick
moved forward; SDL's X11/XWayland path reported that input through the same
aggregate keyboard as legitimate keys. Native Wayland retained real keyboard and
standard SDL gamepad input without receiving the synthetic key. Prefer the clean
backend boundary over a device-specific timing heuristic or disabling simultaneous
keyboard/controller support.

### D-017: Keep named development scenarios in the game executable

Status: accepted on 2026-08-11.

Use `--scenario NAME` for fast, focused interactive environments that still pass
through the real game executable and shared runtime. Use `--list-scenarios` and
`--help` for headless discovery. Parse scenario names into a closed, tested enum;
reject unknown and duplicate selections rather than silently choosing a default.

Retain the generated runway as the `runway` input, movement, camera, and hardware
diagnostic when the oval arrives. Add the oval as a separate named scenario. Do
not create one executable per experiment, a general scene framework, or divergent
copies of simulation/render/input logic. The default launch is `runway` during the
current prototype and may become the normal menu/game flow later.

### D-018: Use a closed distance-sampled track boundary

Status: accepted for the first oval on 2026-08-11.

Represent a runtime track as an ordered set of orthonormal frames in the half-open
range `[0, length)`. Each frame owns center, forward tangent, surface normal,
track-right binormal, and half-width. Define track-right as `normal × tangent` and
wrap both positive and negative query distances across the implicit last-to-first
seam.

Generated and future authored paths must converge on this boundary. The first flat
stadium oval analytically produces evenly spaced frames; sampling interpolates and
re-orthonormalizes them. This validates distance, seam, and lateral-offset behavior
without prematurely choosing the general spline or 3D frame-transport algorithm
needed for loops and corkscrews.

## Deferred decisions

- Final game title, fiction, and visual design language.
- Commercial content count and price.
- macOS as a committed release platform.
- Split-screen scope.
- HDR and retro presentation modes.
- Online or cross-platform multiplayer.
- a player-facing track editor.
- Xbox and PlayStation release commitments.
- Final Steamworks feature set.
- Compiled asset format.

Deferred means “do not implement now,” not “silently rejected.”

## Open technical questions

- Which spline type and coordinate-frame transport behave best through loops,
  corkscrews, and closed-track seams?
- Is 120 Hz the right simulation rate after latency, feel, and CPU tests?
- What is the minimum debug-text solution before a broader UI path is warranted?
- Which Linux runtime/container baseline will be supported for Steam distribution?
- Which laptop GPU/driver and Steam Deck performance baselines should be recorded?

Resolve these questions at the milestone that needs the answer. Add a numbered
decision entry with the evidence and consequences instead of letting the answer
live only in code or chat history.
