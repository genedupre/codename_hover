# Dynamic pilot portraits

## Intent

The race HUD should make pilots feel present inside their ships. Portraits react
to driving and race events rather than remaining static headshots. While active,
the player is always represented, along with the bots currently occupying race
positions one through five. If the player is already in that group, highlight
that ranked portrait instead of drawing a duplicate player portrait. If the
player is outside the top five, the layout may therefore contain the player plus
five leaders. An eliminated player's portrait may complete the same shrink-away
transition as any other eliminated pilot.

That is the current interpretation of the idea, not a final HUD layout. Split
screen, ultrawide, small displays, and a field of roughly thirty racers may
require different sizes or fewer simultaneously visible portraits.

## Low-resolution visual direction

Portraits are not intended to look like high-resolution modern illustrations by
default. The working direction is a deliberately small, compressed, N64-era
image: visible pixel structure, restricted color precision, strong silhouettes,
and enough facial contrast that an expression reads at racing speed. This is an
art target and remains subject to playtesting, not a requirement to reproduce the
Nintendo 64 hardware internally.

The Nintendo 64 supported 16-bit RGBA textures with 5 bits each of red, green,
and blue plus 1 bit of alpha. A 32 by 32-pixel texture in that format is exactly
2,048 bytes (`32 × 32 × 2`). That arithmetic and format are documented in
Nintendo's programming manual. The owner's current F-Zero X reference is that the
first three displayed portraits were approximately 32 by 32 pixels while later
entries were approximately 24 by 24. These dimensions are references for finding
the desired N64 feeling, not fixed asset specifications. Treat the particular
F-Zero X sizes as observations to validate against captures or reverse-engineered
data, not as verified source facts or mandatory Codename Hover sizes.

Codename Hover should experiment with portrait resolution. A 32 by 32 asset may
look right in one layout but unnecessarily coarse on a large 4K display. The game
may use larger source or generated portrait tiers at higher output resolutions,
provided they preserve the intended restricted detail, color treatment, and
pixelated/compressed character rather than silently becoming polished modern
illustrations.

Recommended prototype rules:

- begin by testing 32 by 32 for prominent slots and 24 by 24 for secondary slots,
  then compare larger portrait tiers on 1080p, 1440p, 4K, and Steam Deck;
- preserve an unscaled lossless source or layered working file so the direction
  can change without regenerating degraded artwork;
- quantize exported color deliberately and test ordered dithering where useful;
- sample portrait art with nearest-neighbor filtering and avoid smoothing the
  intended pixel structure;
- keep source texel resolution, on-screen size, and rank-based layout size as
  separate values;
- prefer integer on-screen scaling where the selected asset tier and HUD layout
  permit it, while still supporting arbitrary display resolutions;
- add a future HUD-size setting, including an accessibility-friendly large
  option, that changes layout rectangles independently from the portrait art's
  source resolution.

We do not need to store the runtime atlas as literal raw N64 RGBA5551. SDL_GPU
universally supports RGBA8 sampled textures, so the build can reproduce a limited
palette in RGBA8 while retaining useful multi-level transparency for composited
layers, fire, and sparks. An exact RGBA5551 export can be evaluated later if its
visual result is materially better. PNG is only a source/shipping container; once
decoded and uploaded, animation selects resident atlas regions rather than
loading new PNG files.

## Layered artwork

Aligned transparent PNG layers are the recommended prototype format. They are
easy to create, revise, and combine while preserving the late-1990s illustrated
portrait character. A portrait can initially be composed as:

```text
cockpit/background
pilot base or body
expression/face variant
foreground cockpit or helmet edge
damage state overlay
transient sparks, flame, flash, and warning light
HUD frame, race position, and status
```

Every layer for a pilot should share one canvas size, pivot, crop, and color-space
convention so expressions cannot jump when swapped. A complete face variant is
preferable to separately combining arbitrary eyes and mouths until the artwork
proves that finer facial rigging is useful.

Separate PNGs are suitable during prototyping. The release asset build may place
them in texture atlases and package archives without changing the portrait data
model. Animated sparks and flames can use small sprite sheets or procedural
shader motion; they do not require a video per pilot.

All portrait regions needed for the current race should be decoded, uploaded, and
cached before racing begins. Per-frame animation changes UV regions, tint,
opacity, and small quad transforms only. It performs no PNG decoding, filesystem
access, or texture creation during ordinary portrait animation.

## Portrait definition and runtime state

Each pilot should eventually reference a data definition rather than hardcoded UI
paths:

```text
PilotPortraitDefinition
  base layers
  expression variants
  damage overlays
  effect sprites and attachment regions
  per-expression timing and transition values

PortraitContext
  racer identity and race position
  portrait visibility and layout tier
  speed ratio and propulsion
  boost state
  recent impact event
  damage/energy ratio
  finished, won, or eliminated state
```

Race and vehicle systems publish semantic state or events. A presentation-only
portrait controller chooses images, animation, and transition timing. Portrait
code must not inspect controller buttons, mutate simulation, or change behavior
with render frame rate.

## Initial expression vocabulary

- neutral/focused;
- accelerating;
- boost activation;
- sustained extreme speed;
- steering or high lateral stress, if it reads well;
- recent impact or attack;
- critical ship damage;
- finish/win;
- finish/loss;
- elimination or ship destruction.

Whether ship destruction means the pilot literally dies is a later fiction and
tone decision. Presentation code should call the state `eliminated` or
`ship_destroyed` rather than deciding that story question.

Use a small priority system so simultaneous conditions do not flicker between
faces. A terminal result normally outranks critical damage, which outranks a
recent impact, boost, high speed, and neutral expression. Short events need a
minimum display time and smooth return; sustained states should use hysteresis so
hovering near a speed or damage threshold does not rapidly toggle artwork.

## Rank sizing, elimination, and reflow

The portrait layout is dynamic. Rank and player relevance select a layout tier;
they do not select a different source image or require reloading an asset. A
starting experiment is:

- larger tier: the player and/or race positions one through three;
- smaller tier: the remaining visible leaders;
- absent: racers outside the selected set after any transition completes.

The exact treatment of the player when outside the leaders remains a HUD design
choice, but the player must stay identifiable whenever their portrait is shown.
Interpolation operates in screen/layout space, independent of simulation tick and
render resolution.

When a racer is eliminated, hold their elimination expression briefly, then
shrink the complete portrait stack toward its center until it reaches zero and is
removed. Other portraits receive new target positions and sizes and ease into the
vacated space. The next eligible leader may enter from zero scale or from the
list's edge. Do not instantly rebuild the list: retain stable racer identities so
each visual item can animate from its previous rectangle to its new rectangle.

An implementation can model each visible item with:

```text
racer ID
current rectangle -> target rectangle
current scale -> target scale
current opacity -> target opacity
transition time and state
```

Sorting and elimination originate in authoritative race state, while shrinking,
repositioning, and entrance timing are presentation-only. The elimination
transition must still complete predictably if several racers are destroyed close
together. Reduced-motion mode may replace shrink/reflow motion with a short fade
and immediate stable layout.

## Critical damage presentation

When a ship is near destruction, its portrait background can become an active
cockpit emergency scene:

- intermittent sparks behind or in front of the pilot;
- fire or smoke in the cockpit background;
- warning-light color pulses;
- subtle portrait shake on impacts;
- a pilot-specific critical expression.

Intensity should follow normalized damage with authored limits. Avoid constant
full-screen flashing, preserve face and position readability, and provide reduced
motion/flashing options. Critical state, damage, and imminent elimination must
also be communicated outside this portrait through readable HUD/audio cues.

## Implementation order

1. Draw one static player portrait in a placeholder HUD frame.
2. Select the player plus leaders from authoritative race-position data.
3. Add neutral, boost, and critical-damage variants for one pilot.
4. Add timed transitions and one sprite-sheet spark/fire effect.
5. Define per-pilot data and add a second pilot without portrait-specific code.
6. Validate layout and cost with the maximum visible portrait count.

Do not implement this before authoritative race position and the basic HUD exist;
for now it is a preserved presentation design.

## Research notes

- Nintendo's N64 programming manual documents RGBA16 as 5/5/5/1 and shows 32 by
  32 RGBA16 texture examples:
  https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro14/14-03.html
- SDL_GPU recommends creating and caching resources rather than repeatedly
  creating, releasing, or uploading them:
  https://wiki.libsdl.org/SDL3/CategoryGPU
- Texture atlases reduce texture binding and store named smaller regions in one
  or more page images:
  https://eu.esotericsoftware.com/spine-texture-packer
- Doom's status face is a useful precedent for selecting preloaded expression
  states through priority and timing rather than streaming continuous frames:
  https://github.com/id-Software/DOOM/blob/master/linuxdoom-1.10/st_stuff.c
