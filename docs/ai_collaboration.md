# AI collaboration

## Division of responsibility

The owner is the creative director, product owner, primary playtester, and final
technical reviewer. AI can act as a programmer, graphics assistant, math tutor,
build engineer, Blender/Python assistant, debugger, prototype designer, and
technical writer.

AI does not decide that driving feels good, choose the final art identity, expand
scope without approval, or substitute confident output for measured behavior.

## Preferred task shape

Ask for narrow, testable outcomes with existing context and explicit constraints.
Examples include implementing one spline-sampling operation with tests, adding one
controller action, explaining a coordinate basis used by current code, or
profiling a named scene.

Avoid requests to “make the whole racing game” or patches that introduce many
unproven systems at once. Large changes should be decomposed into runnable
checkpoints.

## Expected AI behavior

- Read `../AGENTS.md` and only the task-relevant routed documents.
- Inspect current code and conventions before proposing new structure.
- State assumptions that affect behavior or scope.
- Explain non-obvious C++, graphics, and math decisions in plain language.
- Keep interfaces small and make ownership, lifetime, inputs, outputs, and failure
  behavior clear.
- Add focused tests where results are deterministic.
- Distinguish measured facts from estimates and hypotheses.
- Preserve portability and avoid unnecessary dependencies.
- Update decisions and documentation when the actual contract changes.

Treat generated code as code from a very fast collaborator: useful, reviewable,
and fallible.

## Owner review checklist

The owner need not derive every equation from memory, but should be able to answer:

- Why does each changed file exist?
- Who owns every important object or resource?
- What inputs enter the system and what results leave it?
- When is it called and on which thread?
- How does it report or recover from failure?
- Which behavior is covered by tests?
- Which behavior was observed interactively, and on what hardware?
- Did the patch add an abstraction or dependency that the current milestone does
  not require?
- Can the change be safely reverted?

If those answers are unclear, pause and request an explanation or smaller patch.

## Game-feel loop

1. Play a short, controlled scenario.
2. Record concrete observations in `../DEVLOG.md`.
3. Change one related group of parameters or behavior.
4. Repeat the same scenario.
5. Keep or revert based on the observed difference.

Useful feedback is specific: “steering becomes too sensitive above 700 km/h” is
actionable; “physics bad” is not. Record build revision, input device, track, and
relevant settings when they could affect the result.

## Scope defense

Interesting ideas belong in `decisions.md` as deferred questions, not immediately
in code. Weather, multiplayer, editors, mod systems, a generalized ECS, or a new
renderer must not enter the active milestone merely because AI can generate a
plausible implementation.
