# Project brief

## One-sentence pitch

A very fast, original anti-gravity arcade racer with sparse low-poly visuals,
large fields of opponents, modern display behavior, and handling that rewards
precision without becoming a simulation game.

## Ownership and production model

The project is initially made by one person with AI assistance. The owner acts as
creative director, product owner, primary tester, and final reviewer. AI may help
with code, math, debugging, tooling, Blender scripts, documentation, and prototype
assets, but the owner decides whether driving is fun and whether the work belongs
in the game.

The owner's SRE background is a strength: favor reproducible builds, automation,
logs, profiling, small interfaces, CI, and incremental delivery. Do not assume
prior professional game-development or Blender experience.

## Experience goals

- Immediate sensation of speed.
- Responsive, learnable arcade handling with room for mastery.
- Tracks that can bank, loop, corkscrew, form tubes, and briefly launch vehicles.
- Dense races that can eventually contain about 30 vehicles.
- A clean presentation whose low complexity is an intentional art direction, not
  an excuse for careless output.
- Fast startup, predictable controller behavior, and native desktop integration.
- The same handling regardless of display refresh rate.

## Visual identity

Take inspiration from the technological and artistic constraints of late-1990s
low-poly racers: simple silhouettes, restrained textures, vertex color, fog, and
sparse environments. Render that language crisply on modern displays.

This is not a remake. All names, fiction, characters, vehicles, tracks, music,
logos, textures, artwork, and source code must be original.

## Initial audience and platforms

The first audience is desktop and handheld PC players who enjoy high-speed arcade
racing. Linux and Steam Deck are first-class targets, not compatibility accidents.
Windows is expected before a commercial PC release. macOS is desirable if its
maintenance and signing burden remains reasonable. Xbox and PlayStation are later
ports, not launch blockers.

## Product goals

- Ship a small, polished PC racing game before considering a broad engine or tool
  ecosystem.
- Support native Linux, Windows, common controllers, keyboard input, borderless
  fullscreen, selectable resolutions, ultrawide layouts, and high refresh rates.
- Make a strong Steam Deck experience with readable UI and no required launcher.
- Keep the runtime and dependency footprint small.
- Make future platform ports possible through abstraction at true platform
  boundaries.

## Explicit non-goals for the first release

- A reusable general-purpose engine.
- Photorealism or a PBR-first art pipeline.
- Online or cross-platform multiplayer.
- A player-facing track editor or modding ecosystem.
- A custom scene/content editor.
- Dynamic weather, ray tracing, or similarly expensive showcase features.
- Launching simultaneously on PC, Xbox, and PlayStation.

Split-screen, HDR, modding, a track editor, online play, and console releases may
be reconsidered only after the core driving game is good and the PC scope is under
control.

## First meaningful success

Before menus, story, content production, or store work, the project must produce
one ugly but compelling vehicle on one simple generated track. If the owner keeps
driving it longer than intended because the handling is enjoyable, the prototype
has achieved its main purpose.
