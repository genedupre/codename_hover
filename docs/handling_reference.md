# Grounded handling reference ledger

## Purpose and confidence policy

This ledger translates the matching F-Zero X decompilation into named behavioral
evidence for Codename Hover. It is not an instruction to copy source structure or
opaque constants. The production simulation remains original, SI-scaled, and
fixed at 120 Hz.

Confidence labels mean:

- **high**: the value's reads, writes, and mathematical role are directly clear;
- **medium**: the behavior is clear but its exact authored meaning or units are
  not fully named;
- **low**: a relationship is plausible but requires more call-site or empirical
  evidence before implementation.

Primary references:

- [`Racer` structure](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/include/unk_structs.h#L156-L340)
- [`Racer_UpdateFromControls`](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L3345-L4155)
- [wall and surface handlers](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/racer.c#L2325-L2624)
- [smooth follow camera](https://github.com/inspectredc/fzerox/blob/4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8/src/game/camera.c#L559-L676)

References are pinned to matching-decompilation commit
`4fd50c7ca6b44f996aa0fbb68ec86df75855d5b8` so the evidence does not move as the
project progresses.

## Coordinate and state mapping

| Reference field/concept | Inferred responsibility | Reference units/update | Codename Hover equivalent | Confidence |
| --- | --- | --- | --- | --- |
| `segmentPositionInfo.pos` | authoritative racer world position after movement/contact | game distance | `PhysicalVehicleState::position` | high |
| `velocity` / `acceleration` | authoritative world motion and accumulated forces | distance/tick and distance/tick² | world velocity; future accumulated acceleration | high |
| `segmentPositionInfo` | nearest course segment, parameter, displacement, and last grounded position | mixed derived state | `ProjectedCourseReference`; future contact state | high |
| `segmentBasis` | course-local axes near the racer | normalized basis | `TrackFrame` | high |
| `trueBasis` | physical machine orientation used by steering and forces | normalized basis | `PhysicalVehicleBasis` | high |
| `modelBasis` / `modelPos` | separately filtered rendered transform | presentation state | future visual basis plus existing `VehiclePose` | high |
| `unk_5C.x/y/z` | velocity resolved into local lateral, normal, and forward axes | distance/tick | `WorldTrackVehicleTelemetry::local_*_speed` | high |
| `speed` | magnitude of world velocity | distance/tick | telemetry world speed | high |
| `heightAboveGround` | signed separation from the supporting surface | game distance | `height_above_surface_metres` | high |
| `upFromGround` | local surface normal at the contact point | unit vector | course frame normal; future contact normal | high |
| `gravityUp` | current gravity reference, blended toward world-up while airborne | unit vector | future contact gravity direction | high |
| `tiltUp` | desired physical-up direction, influenced by ground and airborne pitch | unit vector | future desired physical-up | medium |
| `focusPos` / `focusVelocity` | filtered camera focus point and its motion | game distance/tick | future camera-facing physical sample | high |

The reference names its basis members differently from Codename Hover. Infer
roles from every dot product and cross product rather than assuming a field named
`x` is the world or local x-axis.

## Grounded update relationships

| Source behavior | Evidence and order | Project interpretation | Confidence |
| --- | --- | --- | --- |
| Steering rotates the physical basis | stick X changes `trueBasis`; velocity is not rotated with it | rotate physical forward around physical up; preserve momentum | high |
| Direction change is measured | the difference between old and reconstructed basis axes is stored as `directionChange` | expose steering direction change in radians per tick | high |
| Velocity is damped in local axes | velocity is projected onto physical axes, multiplied by axis coefficients, then recomposed | explicit time-corrected local forward/lateral/normal damping stage | high |
| Shoulder input changes steering response | Z/R selects values derived from `unk_1E0` and `unk_1E4` | ship-defined drift steering response | medium |
| Shoulder drift adds lateral force | one shoulder adds signed force projected into the gravity plane | directional left/right drift acceleration | high |
| Drift force fades with existing slide | force falls linearly to zero as same-direction `unk_5C.x` approaches a threshold | attenuate drift force from local lateral speed | high |
| Both shoulders are not ordinary drift | the normal drift switch handles neither/both as zero drift force; combined presses participate in attacks | keep both-held neutral until attacks are implemented | high |
| Grip removes bounded lateral speed | a signed local lateral component is capped before subtraction from velocity | move lateral speed toward zero by a maximum amount per second | high |
| Shoulder/air/damage selects another traction cap | Z/R or disturbed states choose `unk_1FC`; ordinary grounded logic may choose `unk_1F8` | explicit normal and drift traction limits | medium |
| Sustained side slip builds a timed state | lateral magnitude beyond 8 increments `unk_20C`, raising `unk_200`; it resets below the threshold | named slip duration/intensity with attack/release behavior | high |
| Sustained slip changes propulsion coefficients | active `unk_200` selects `unk_1B4` and a decaying multiplier | replace provisional direct slip drag with source-shaped propulsion response | medium |
| Direction change and drift reduce engine force | `directionChange * 0.5` and `driftAttackForce * 0.16` are subtracted from requested acceleration | positive propulsion penalty driven by measured direction change and drift strength | high |
| Acceleration response is smoothed | `accelerationForce` approaches a speed-dependent requested force with several setup-derived coefficients | named smoothed applied propulsion state | medium |
| Boost changes propulsion multiplier | boost timer raises a multiplier and consumes energy | retain timed boost now; later integrate energy and response curves | high |

## Contact, hover, and track form relationships

| Source behavior | Project interpretation | Confidence |
| --- | --- | --- |
| Gravity is applied every update | accumulate gravity in every supported/airborne mode | high |
| Hover correction uses height error and normal velocity | damped hover force toward a ship-defined target, not exact ride-height placement | high |
| Surface penetration is corrected separately | project out of the surface and remove only inward normal velocity | high |
| Separation changes airborne state | explicit supported/airborne transition with hysteresis | high |
| Wall response corrects penetration and checks outward velocity | separate wall contact from open-edge policy; add recoil, damage, and events | high |
| Borderless road permits falling | absence of support changes contact mode without replacing world position | high |
| Track form dispatch covers road, walls, pipe, cylinder, half-pipe, tunnel, air, and borderless road | surface queries and contact policies share one physical state | high |
| Pipe normal derives radially from centerline displacement | future pipe query returns radial surface normal and height | high |

## Camera relationships

| Source behavior | Project interpretation | Confidence |
| --- | --- | --- |
| Camera follows `focusPos`, not raw course position | expose a filtered physical focus point | high |
| Focus velocity contributes look-ahead | camera look-ahead follows motion, including slip | high |
| Camera up approaches racer gravity/up over time | smooth camera orientation; never copy track normal instantly | high |
| Camera basis is orthonormalized after smoothing | rebuild a stable view frame through banks, loops, and pipes | high |

## Unit conversion rules

For a multiplicative coefficient applied once per 60 Hz reference update:

```text
coefficient(dt) = coefficient60 ^ (60 × dt)
coefficient120  = sqrt(coefficient60)
```

Timers are first expressed in seconds. Additive velocity/acceleration values need
an explicit reference-distance scale before becoming SI values. Never infer that
an original value such as `8.0` means eight metres per second.

## Current project status

Already implemented provisionally:

- world position, velocity, and physical basis;
- bounded course projection after candidate movement;
- orientation steering with momentum lag;
- bounded normal/drift lateral grip;
- directional drift force with same-direction attenuation;
- steering/drift propulsion suppression;
- exponential local-axis damping converted to continuous per-second rates;
- a speed-shaped, speed-responsive propulsion envelope;
- named sustained-slip duration and intensity with a release envelope;
- exact supported ride height and safe edge constraint;
- per-tick local velocity, slip, grip, drift, propulsion, and edge telemetry;
- repeatable straight, steering, boost, drift, release, coast, and brake traces.

Still required for the grounded reconstruction:

- further source/feel comparison of the initial damping and propulsion tune;
- owner acceptance of straight acceleration, coasting, boost return, steering
  loss, and slip recovery on the handling lab and Speedway;
- telemetry additions for future surface height, contact mode, wall impacts,
  energy, and damage when those mechanics exist.

Do not advance low- or medium-confidence interpretations into authoritative
physics without a test that distinguishes them from the plausible alternatives.
