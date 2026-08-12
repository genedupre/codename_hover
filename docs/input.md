# Input

## Boundary

SDL3 is the primary free, cross-platform input provider. Platform code translates
SDL keyboard, mouse, and gamepad state into one game-facing `PlayerInput` value;
vehicle simulation never reads SDL devices or button names directly.

`PlayerInput` currently contains:

- signed analog steering in `[-1.0, 1.0]`;
- analog throttle and brake in `[0.0, 1.0]`;
- digital drift and boost actions.

A digital button therefore contributes `0.0` or `1.0` to an analog action. Using
the controller's A/South button for full throttle does not turn throttle into a
boolean and does not prevent a trigger, pedal, or other analog source from being
mapped later.

## Simultaneous devices

Keyboard, mouse, and every open gamepad remain live at the same time. Contributions
are merged without adding them together:

- steering uses the contribution with the greatest absolute magnitude;
- throttle and brake use the greatest contribution;
- digital actions use logical OR.

This permits combinations such as keyboard throttle plus controller steering and
prevents two active devices from producing values outside the action range.
Per-player controller assignment and duplicate-device suppression become necessary
before local multiplayer, but are not part of this one-player prototype.

## Current prototype bindings

| Action | Keyboard/mouse | SDL gamepad |
| --- | --- | --- |
| Steer | A/D or Left/Right | Left stick X or D-pad |
| Throttle | W/Up or left mouse | A/South button, 100% |
| Brake | S/Down or right mouse | Left trigger analog or B/East button |
| Drift | Space or mouse X1 | Left shoulder |
| Boost | X, Shift, or mouse X2 | X/West button |
| Rumble test | R | Y/North button |
| Escape | Escape/window close | Select/Back/View button |

The Select/Back/View button currently exits because the prototype has no menu flow.
Start/Menu remains available for a future pause/menu action. B/East contributes
100% to the analog brake action and the left trigger preserves proportional
braking. X/West moved from duplicate braking to boost; the right shoulder is
currently free.

The right trigger is deliberately unbound for throttle in this checkpoint. The
logical position names South/East/West/North are authoritative; printed letters
vary on Nintendo-style and other layouts.

Drift remains represented but does not yet alter the provisional runway
simulation. Boost uses the rising edge of its merged digital action to start a
timed fixed-step burst when throttle is positive and brake is inactive. Holding a
source does not continuously boost or retrigger the action; after an ineligible
press, the player must release and press again. A successful activation produces
one semantic event and submits a short rumble pulse to every rumble-capable
controller currently assigned to the prototype player. This also happens when
boost is activated by keyboard or mouse. No-throttle presses and simultaneous
braking suppress both boost and the pulse. See `game_design.md` and
`rendering.md`.

## Device lifecycle and capabilities

- Enumerate and open existing SDL gamepads at startup.
- Open and close gamepads on SDL hotplug events.
- Apply dead zones before merging analog axes.
- Log the SDL name, standardized type, Steam handle, and rumble capability.
- Submit rumble only when SDL reports support; failed rumble is a warning, never a
  startup or gameplay failure.
- Keep gameplay rumble driven by semantic events rather than physical SDL button
  events so remapping and simultaneous input retain identical feedback.
- Never encode essential information only as vibration.

The wired Steam Controller lifecycle was interactively verified on the Linux
laptop on 2026-08-11: startup discovery, Y/North rumble, removal, reconnection
under a new SDL instance ID, and resumed steering/throttle all worked as intended.
The exit binding was subsequently moved from B/East to Select/Back/View. Focused
tests and a Steam Controller playtest confirmed that Select exits while Start and B
leave the prototype running. B/East was subsequently assigned to braking; a
virtual SDL gamepad test protects both its full-brake value and non-exit behavior.

SDL currently opens the wired Steam Controller as a standard gamepad while Steam
is running. Steam Input may later become an optional platform provider for richer
remapping and glyph behavior, but it must feed the same semantic actions and remain
outside vehicle logic. Do not add direct XInput, DirectInput, HID, or vendor calls
while SDL supplies the required capability.

### Linux Steam desktop-layout duplication

On the audited GNOME Wayland laptop, Steam's desktop controller layout injects a
synthetic keyboard `W` through XTEST when the Steam Controller's left stick moves
forward. When the game used SDL's X11 backend through XWayland, SDL received the
synthetic event and the real keyboard through one aggregate keyboard. Game code
could not filter only the controller-generated `W` without also risking legitimate
keyboard input.

The verified fix is to prefer SDL's native Wayland backend on Linux and retain X11
as fallback. The native client still receives the real keyboard and SDL gamepad,
but does not receive the XWayland-only XTEST injection. Do not add a timing-based
`W`/stick suppression heuristic. If X11 fallback is required with this controller,
use a Steam per-game gamepad layout that does not emit keyboard movement.

## Simulation timing

Platform state is sampled once per rendered frame and the resulting semantic input
is consumed by the next fixed simulation tick. Multiple ticks used to recover from
a slower render frame deliberately receive the same sample. Changing render rate
must not change acceleration, travel distance, or other deterministic behavior.

Latency-sensitive tuning may justify a higher fixed tick rate or later input
sampling closer to a tick, but those changes require measurements and game-feel
testing rather than coupling simulation to render FPS.
