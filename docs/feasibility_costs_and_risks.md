# Feasibility, costs, and risks

## Feasibility summary

The proposed game is technically plausible for a solo developer with AI help.
Simple low-poly rendering, a specialized track-relative simulation, and
track-space AI keep the runtime problem modest compared with a general 3D game.
The difficult work is expected to be handling, track design, camera tuning, AI
behavior, UI, audio, content, balance, testing, and polish—not triangle throughput.

Support for modern resolutions, high refresh rates, native Linux, common
controllers, Steam Deck, and a field of roughly 30 racers is reasonable to pursue.
Exact performance and platform support remain claims to prove on named hardware.

## Cost model

The core toolchain can be free/open source. Imported planning figures below are
not vendor quotes and must be rechecked at purchase or onboarding time.

| Item | Planning assumption |
| --- | --- |
| C++ compiler, CMake, Ninja, Git | No license cost |
| SDL3 and SDL_shadercross | No license cost under their applicable licenses |
| Blender | No license cost |
| Steam Direct | USD 100 per product; imported as recoupable after a revenue threshold |
| Apple Developer Program | USD 99 per year if a macOS release needs it |
| Xbox program | No public application/certification/publishing fee was assumed; other partner costs may exist |
| PlayStation program | Unknown until current partner terms are available |

A self-produced PC prototype may have almost no external cash cost beyond existing
hardware. A polished commercial release might spend roughly EUR 1,000–10,000 on
optional music, audio, art, trailer/store assets, localization, testing, software,
or services. That is a planning envelope, not a target or market quote.

Console releases can add hardware, porting, platform integration, ratings,
specialist QA, certification iteration, and business overhead even when public
platform fees are low.

## Primary risks

### Game feel

The largest product risk is that the vehicle is technically correct but not fun.
Mitigation: reach the one-vehicle/one-track prototype early and spend substantial
time on controlled playtest iterations before producing content.

### Scope expansion

A custom runtime can quietly become a general engine, editor, network platform, or
mod toolkit. Mitigation: every system must serve the active milestone; record new
ideas as deferred decisions.

### First-game learning load

C++, rendering, game math, Blender, game design, and distribution are each
substantial topics. Mitigation: learn just in time, generate early assets in code,
and keep changes small enough for owner review.

### AI-generated complexity or defects

AI can create plausible but unnecessary abstractions, unsafe lifetimes, subtly
incorrect math, and portability failures. Mitigation: narrow tasks, tests, warnings,
validation, profiling, small commits, and the review contract in
`ai_collaboration.md`.

### Cross-platform drift

Linux-first code can accidentally accumulate POSIX assumptions, while a Proton
build can mask native Linux problems. Mitigation: native Linux as a requirement,
early Windows compilation, runtime tests on each claimed platform, and narrow
platform boundaries.

### Performance assumptions

Simple art does not automatically guarantee smooth 4K/240 output; frame pacing,
CPU submission, synchronization, drivers, and pathological content still matter.
Mitigation: representative benchmarks, explicit hardware/settings, and profiling
before optimization.

### Content and polish volume

Tracks, vehicles, sound, music, UI, tuning, accessibility, compatibility, and QA
can dominate schedule after the renderer works. Mitigation: use a vertical slice
to set final content scope and budget, and outsource selectively if justified.

### Console certification

Console difficulty lies mainly in platform access, behavior, certification, and
QA rather than graphics. Mitigation: isolate real platform services, stabilize the
PC game first, and obtain current partner requirements before promising releases.

### Intellectual-property confusion

A faithful era/style inspiration can drift into copying recognizable protected
content. Mitigation: establish original terminology, silhouettes, world building,
track layouts, audio, branding, and source assets from the beginning.

## Not recommended

- Building directly on Vulkan, D3D12, and Metal for this visual target.
- Using OpenGL as the primary long-term cross-platform renderer.
- Adding a general physics engine before custom track-relative physics is tested.
- Building a custom editor instead of using Blender.
- Treating an ECS, scripting, HDR, modding, or online multiplayer as foundational.
- Promising console support before partner access and certification planning.
- Producing large amounts of content before proving game feel.
