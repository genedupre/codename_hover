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

Gameplay must not depend on rendered frame rate. A 90 Hz is a first desired state (steamdeck) but we should be able to get 244hz as well, but should also work well on 60hz or lower, since we want broad compatability.

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

- Which SDL3, SDL_shadercross, and supporting dependency revisions should bootstrap
  pin?
- Should dependencies be Git submodules, CMake FetchContent with locked revisions,
  or another reproducible local mechanism?
- What shader build toolchain gives the cleanest Linux, Windows, and Deck workflow?
- Which spline type and coordinate-frame transport behave best through loops,
  corkscrews, and closed-track seams?
- Is 120 Hz the right simulation rate after latency, feel, and CPU tests?
- What is the minimum debug-text solution before a broader UI path is warranted?
- Which Linux runtime/container baseline will be supported for Steam distribution?
- Which laptop GPU/driver and Steam Deck performance baselines should be recorded?

Resolve these questions at the milestone that needs the answer. Add a numbered
decision entry with the evidence and consequences instead of letting the answer
live only in code or chat history.
