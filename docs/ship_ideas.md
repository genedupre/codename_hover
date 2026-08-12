# Ship ideas

## Status of this file

This is a visual and gameplay idea notebook. It does not define implemented ship
statistics; accepted definitions and collider contracts belong in `ships.md` and
the source tree. Tentative ideas may be changed or removed freely.

## Regular Jaguar ship

No ship name or design is established yet. Its eventual design should support the
more grounded Regular Jaguar concept and provide a clear visual baseline against
which Cyber Jaguar's ship can look more extreme.

Open questions include silhouette, color palette, manufacturer, handling
identity, collider scale, durability, and whether this is related mechanically or
only visually to Cyber Jaguar's ship.

## Cyber Jaguar ship

No ship name or design is established yet. It should read as the cooler, more
technologically extravagant counterpart to Regular Jaguar's ship.

That does not automatically make it better in every statistic. A more extreme
silhouette, engine effect, cockpit treatment, sound, boost behavior, or demanding
handling tradeoff can communicate its identity without invalidating the other
ship.

## Template for future ship ideas

Record concepts here before implementation:

- working and display name;
- associated pilot, if any;
- one-sentence gameplay identity;
- silhouette, scale, palette, cockpit, engines, and exhaust language;
- likely strengths and tradeoffs, without inventing final numbers;
- special presentation or audio needs;
- generated prototype, sketch, or future Blender/GLB source;
- unresolved choices and explicit owner feedback.

When a concept is selected for implementation, give it a stable ID and explicit
`ShipDefinition`; do not infer handling, collision, or durability from its mesh.
