# Track system

## Purpose and boundary

The runtime track representation is a closed path sampled by distance. Generated
C++ prototypes and future Blender-authored paths should both converge on the same
`SampledTrack` boundary. Vehicle, camera, race-position, AI, and mesh-generation
code should consume this representation rather than special-case the oval's
formula.

The `oval` development scenario renders a surface generated from this generic
representation and spawns the ship at distance zero. It does not yet constrain
the ship; the named `runway` scenario remains unchanged.

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

## Verified properties

Focused tests cover:

- exact oval length and requested sample count;
- start, far-turn, and halfway landmarks;
- positive, exact-length, and negative distance wrapping;
- position and orientation continuity across the closed seam;
- validity of every stored frame and every midpoint interpolation;
- rejection of incompatible adjacent-frame orientation flips;
- right/height offset convention;
- rejection of an oval whose turn radius cannot contain its width.

The generated prototype surface consumes only `SampledTrack`. It joins three
bands between each adjacent pair of frames and includes the implicit
last-to-first segment, so there is no special open end. The first segment is
copper-colored to make the start/seam visually inspectable. Variable frame widths
are respected independently at both ends of every segment.

## Next boundary

The generated surface and highlighted seam were visually validated in
`--scenario oval` on the laptop on 2026-08-11. `TrackVehicleState` now provides a
validated path-local state boundary, but no simulation uses it yet. Its opaque
`TrackPathId` identifies the currently followed path without storing an owning
pointer or assuming that a future course has only one route. A later track graph
can transition the ID and distance at splits and joins.

Next, advance this state along the oval centerline, wrap its distance at the seam,
and derive a world pose from the sampled frame. Preserve `--scenario runway` as
the free-driving regression sandbox. Steering and branch choice follow only after
centerline movement is verified.

Loops, banks, corkscrews, and other continuously attached shapes are represented
by the sampled frame orientation. True airborne jumps should use separate
world-space motion until the ship reacquires an eligible path; do not force an
airborne ship to remain attached to one path's normal.
