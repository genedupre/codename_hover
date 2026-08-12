# Track system

## Purpose and boundary

The runtime track representation is a closed path sampled by distance. Generated
C++ prototypes and future Blender-authored paths should both converge on the same
`SampledTrack` boundary. Vehicle, camera, race-position, AI, and mesh-generation
code should consume this representation rather than special-case the oval's
formula.

The `oval` and `speedway` development scenarios render surfaces generated from
this generic representation, spawn the ship at distance zero, and currently use
the provisional scalar-distance traversal. The named `runway` scenario remains
unchanged.

## Track frame

Each `TrackFrame` contains:

- wrapped distance along the closed track in metres;
- center position;
- unit tangent in the direction of race travel;
- unit surface normal;
- unit binormal pointing toward track-right;
- half-width in metres.

The three axes are orthogonal. The project convention is:

```text
normal × tangent = binormal (track-right)
```

Therefore, on a flat frame facing positive Z, normal is positive Y and binormal is
positive X. Positive lateral offsets move right; positive height moves away from
the driving surface along its normal.

## Closed sampled track

`SampledTrack` stores a positive total length and strictly increasing frames in the
half-open distance range `[0, length)`. Frame zero exists at distance zero; the end
frame is not duplicated. Sampling:

1. wraps positive or negative input distance into `[0, length)`;
2. locates the surrounding stored frames;
3. interpolates center and width;
4. interpolates and re-orthonormalizes the local axes;
5. interpolates the final stored frame back to frame zero across the seam.

Keeping the seam implicit prevents two authoritative copies of the start frame.
Validation checks finite values, increasing distances, positive dimensions, unit
axes, orthogonality, the track-right convention, and compatible adjacent axes.
That final check includes the seam and rejects individually valid frames that flip
too far to interpolate safely.

## World-point projection

`project_point_onto_track` is the inverse query needed by world-space vehicle
physics. Given a world point, previous path-distance hint, and bounded along-path
search radius, it returns the nearest point on the piecewise-sampled centerline,
the interpolated frame, signed lateral/height offsets, and signed progress from
the hint. It searches through the closed seam and measures offsets in the local
banked frame rather than world axes.

The bounded hint is part of correctness, not only an optimization. A global
nearest-point query can choose the wrong layer of a loop, side of a crossing, or
future branch. Course-level code will select eligible paths and search windows;
the path primitive does not decide route changes. The current implementation
checks sampled chords and is intentionally simple enough to validate before any
spatial index is justified by profiling.

Projection will make path distance, lateral offset, and surface height derived
measurements after world velocity integration. The existing scalar-driven
traversal has not yet been switched over.

## First oval generator

The first generated source is a flat stadium oval: one straight in positive Z, a
semicircular far turn, the opposite straight in negative Z, and a semicircular
near turn. Its definition contains:

- full length of each straight;
- centerline turn radius;
- half-width;
- elevation.

Its exact centerline length is:

```text
2 × straight length + 2π × turn radius
```

The turn radius must exceed the half-width so the inner edge remains meaningful.
The generator analytically evaluates evenly spaced frames and returns a generic
`SampledTrack`; later users do not need to know how those frames were produced.

## First map prototype: Oval Speedway

`--scenario speedway` is the first named map rather than a geometry reference.
It uses the oval's exact centerline and applies a bank angle to the sampled frame
orientation, so the generic surface generator needs no speedway-specific logic.
Its current provisional dimensions are:

- 600 metres per straight;
- 180-metre centerline turn radius;
- 24-metre half-width;
- approximately 2,330.97 metres total centerline length;
- 28-degree maximum bank in both turns;
- 85-metre smooth bank transitions at each end of each turn.

Both stadium turns bend left in the race direction. Positive banking lowers the
inside edge and raises track-right, the outside edge. The normal and binormal are
rotated around the existing tangent using a smoothstep transition; straights and
the start seam remain level. The resulting map is still an ordinary
`SampledTrack`, preserving the path needed for authored tracks, loops, splits,
and other future segment types.

Keep the flat `oval` scenario as the simplest attachment regression. Use
`speedway` to validate that the same vehicle pose follows a banked surface without
map-specific vehicle behavior.

## Attached vehicle traversal

`TrackVehicleState` stores an opaque path ID, canonical distance, lateral offset
and velocity, persistent signed heading relative to the path tangent,
surface-normal offset and velocity, and the shared vehicle dynamics used for
propulsion and presentation. Course/scenario ownership resolves the ID to one
`ResolvedTrackPath` for simulation; the per-vehicle state never owns track
geometry.

Each fixed tick currently:

1. advances the same throttle, brake, coasting, and boost dynamics as `runway`;
2. rotates persistent surface-relative heading from semantic steering input;
3. resolves body speed into along-track and lateral components, then approaches
   the lateral target using normal or drift grip;
4. converts forward world distance into centerline distance using the local
   selected-lane length scale, then advances and wraps the resolved closed path;
5. subtracts tangent rotation around the surface normal from relative heading,
   requiring active steering through horizontal curvature without interfering
   with automatic surface pitch and roll;
6. advances lateral offset and provisionally clamps the ship's local box
   footprint inside the sampled road width;
7. holds the configured ride height along the local surface normal;
8. derives world position, forward, and up from the sampled frame and local
   heading.

The full orientation basis replaces a world-yaw-only pose, allowing the same
render and follow-camera path to bank now and later become vertical or inverted.
The width clamp is an early playable boundary, not a final declaration that every
track edge has an invisible wall. Barriers, open edges, falling, collision
response, and recovery need explicit surface/segment policy before replacing it.

## Verified properties

Focused tests cover:

- exact oval length and requested sample count;
- start, far-turn, and halfway landmarks;
- positive, exact-length, and negative distance wrapping;
- position and orientation continuity across the closed seam;
- validity of every stored frame and every midpoint interpolation;
- rejection of incompatible adjacent-frame orientation flips;
- right/height offset convention;
- rejection of an oval whose turn radius cannot contain its width;
- valid speedway frames without changing the underlying oval length;
- a level speedway seam, configured maximum turn bank, and correct inside/outside
  edge heights;
- preservation of the banked orientation by the generic surface mesh;
- spawn pose and ship-defined ride height from a generic path frame;
- forward seam wrapping, lateral steering, and collider-aware width limits;
- persistent heading without idle snap-back and required steering through
  horizontal corners;
- physically distinct centerline advancement for shorter inside and longer
  outside lanes;
- automatic sampled-surface pitch through a deterministic vertical-loop fixture
  without introducing horizontal steering;
- banked vehicle/model orientation without a world-up assumption;
- identical attached results under tested 24–360 Hz render schedules;
- world-point projection of distance and signed offsets on flat and banked track;
- bounded backward projection through the closed seam.

The generated prototype surface consumes only `SampledTrack`. It joins three
bands between each adjacent pair of frames and includes the implicit
last-to-first segment, so there is no special open end. The first segment is
copper-colored to make the start/seam visually inspectable. Variable frame widths
are respected independently at both ends of every segment.

## Next boundary

The generated surface and highlighted seam were visually validated in
`--scenario oval` on the laptop on 2026-08-11. The owner visually accepted the
standalone banked `--scenario speedway` surface on 2026-08-12. Both scenarios now
run the attached simulation described above; that implementation remains a
playable regression while world-space physics replaces its movement authority in
staged changes. Preserve `--scenario runway` as the free-driving regression
sandbox.

Next, initialize a physical world velocity/basis beside the current spawn state,
then use the projection primitive to derive progress without allowing both models
to move the same axis. Playtest the replacement on `speedway` before deleting the
scalar regression. A later course graph can transition `TrackPathId` at splits
and joins, but branch metadata and policy should arrive with the first observable
split-track experiment rather than being guessed now.

Loops, banks, corkscrews, and other continuously attached shapes are represented
by the sampled frame orientation. True airborne jumps retain authoritative world
motion, change contact/force policy, and may refresh their course reference from
an eligible landing path; do not force an airborne ship onto one path's normal.
