# AGENTS.md

## Project context

Codename Hover is a solo, is a futuristic anti-gravity arcade racer and is spritually inspired by f-zero x.
Its visual language should evoke that. (sparse low-poly console racing games of the late
1990s, while its world, vehicles, tracks, names, music, and code remain somewhat original.)

The project is a game, not an engine product. Build only the portable runtime
needed by this game. Linux is the primary development platform and Steam Deck is
the first real target device. Windows follows; macOS and consoles must remain
architecturally possible without blocking the first playable prototype.

The project is currently in pre-production. The laptop prototype now has a
generated ship moving on a long presentation runway, fixed-step simulation,
interpolated rendering, a follow camera, and simultaneous keyboard/mouse/SDL
gamepad input. The active checkpoint is validating input and presentation policy
before introducing track-relative movement; do not jump ahead to full race systems.

The cross-device bootstrap is not complete. A wired Steam Controller was detected
and used on the laptop on 2026-08-11. Steam Deck deployment remains deferred until
the Deck is available; retain its build, deployment, input, and exit criteria.

## Documentation

- [Agent instructions](AGENTS.md) — concise project context and task-based reading
  guide for coding agents.
- [Project brief](docs/project_brief.md) — vision, goals, boundaries, and success
  criteria.
- [Game design](docs/game_design.md) — handling, tracks, race systems, and AI.
- [Ship system](docs/ships.md) — ship definitions, visual sources, collider
  contracts, and the current Prototype 01 state.
- [Input](docs/input.md) — semantic actions, simultaneous-device merging,
  current bindings, hotplugging, and rumble policy.
- [Technical architecture](docs/architecture.md) — stack, system boundaries, and
  runtime design.
- [Rendering and display](docs/rendering.md) — visual direction, display behavior,
  and performance policy.
- [Platforms and release](docs/platforms_and_release.md) — desktop, Steam Deck,
  Steam, and future console plans.
- [Development workflow](docs/development_workflow.md) — tools, build/test loop,
  assets, CI, and deployment.
- [Environment audit](docs/environment_audit.md) — verified laptop hardware,
  graphics state, installed tools, and bootstrap prerequisites.
- [Development hardware](docs/development-hardware.txt) — laptop and, later, Steam Deck inventory.
- [Roadmap](docs/roadmap.md) — incremental milestones from first triangle to a
  vertical slice.
- [AI collaboration](docs/ai_collaboration.md) — responsibilities, patch sizing,
  and review practices.
- [Feasibility, cost, and risk](docs/feasibility_costs_and_risks.md) — planning
  estimates, costs, and major risks.
- [Decisions and open questions](docs/decisions.md) — accepted constraints,
  deferred features, and unresolved choices.
- [Development log](DEVLOG.md) — dated test observations and immediate follow-up
  work.

## Baseline constraints

- Use C++23, CMake, and Ninja unless a recorded decision changes the stack.
- Use SDL3 for platform facilities and SDL_GPU for graphics abstraction.
- Prefer HLSL source compiled offline through SDL_shadercross when shaders arrive.
- Keep rendering frame rate independent from a fixed-step simulation.
- Implement game-specific track-relative vehicle physics; do not introduce a
  general rigid-body physics engine without evidence that it is needed.
- Use simple data structures first. Do not add an ECS, scripting runtime, general
  scene editor, or general engine layer speculatively.
- Generate primitive prototype assets in code. Use Blender and glTF/GLB when
  authored assets become useful; do not build a custom editor.
- Keep vendor APIs behind narrow service interfaces and out of game logic.
- Preserve native Linux support. Proton compatibility is not a replacement for it.
- Never claim a performance target is achieved without measurements on named
  hardware and settings.

## Read only the context relevant to the task

Do not preload every project document. Start here, inspect the code being changed,
then read the smallest applicable set:

| Task | Read |
| --- | --- |
| Product direction, scope, or originality | `docs/project_brief.md` |
| Handling, track geometry, racing, AI, or camera feel | `docs/game_design.md` |
| Ship definitions, colliders, visual sources, or adding a ship | `docs/ships.md` |
| Keyboard, mouse, gamepad, Steam Input, bindings, or rumble | `docs/input.md` |
| C++ structure, dependencies, ownership, simulation, or services | `docs/architecture.md` |
| GPU code, shaders, visuals, resolutions, frame pacing, or UI rendering | `docs/rendering.md` |
| Linux, Steam Deck, Windows, macOS, Steam, Xbox, or PlayStation | `docs/platforms_and_release.md` |
| Local tools, builds, tests, CI, asset processing, or Deck deployment | `docs/development_workflow.md` |
| Current laptop capabilities, missing tools, GPU state, or audit caveats | `docs/environment_audit.md` |
| Choosing or completing the next milestone | `docs/roadmap.md` |
| AI-authored changes and human review boundaries | `docs/ai_collaboration.md` |
| Schedule, budget, feasibility, or project risk | `docs/feasibility_costs_and_risks.md` |
| Challenging a constraint or resolving an unknown | `docs/decisions.md` |
| Interpreting recent playtest feedback | `DEVLOG.md` and the relevant system doc |

For a cross-cutting change, read each directly affected document, not the entire
documentation set.

## Working rules

- Make the smallest playable, testable change that advances the active milestone.
- Inspect existing interfaces and ownership before adding a new abstraction.
- Keep patches reviewable by a technically experienced owner who is learning game
  development; explain non-obvious graphics, game-math, and C++ decisions plainly.
- Add focused tests for deterministic math and simulation logic. Verify rendering
  and game feel interactively on relevant hardware.
- Treat compiler warnings, validation errors, and shader compilation failures as
  defects; do not hide them to make a build green.
- Record measured game-feel observations in `DEVLOG.md`, not as unexplained magic
  constants in code.
- If a change alters an accepted constraint or milestone, update
  `docs/decisions.md` and the affected topical document in the same patch.
- Treat prices, store rules, SDK support, and console processes as time-sensitive.
  Reverify them using first-party sources at the point of use.

## Definition of done for a change

- The relevant target builds without introducing new warnings.
- Deterministic logic has proportionate automated coverage.
- Interactive behavior is tested where the environment allows it; otherwise state
  exactly what remains unverified.
- Linux and future portability are not knowingly regressed.
- Documentation changes only when behavior, decisions, or operating instructions
  actually changed.
