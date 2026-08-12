# Persistence

## Goal

The game should use one explicit persistence boundary and one portable data
format rather than scattering `fstream`, home-directory paths, registry calls, or
platform SDK calls through gameplay.

```text
game/settings/race systems
          |
          v
    PersistenceService
          |
    serialize / validate
          |
          v
 SDL user storage on desktop
 platform storage adapter on consoles when required
          |
          v
 optional Steam Cloud policy on PC
```

SDL3's `SDL_OpenUserStorage` is the preferred desktop starting point. It provides
a user- and application-specific read/write container and is designed around the
restrictions of desktop and console storage. If a target needs its native user,
quota, save-data UI, or transaction flow, implement that behind the same narrow
boundary or through a custom `SDL_StorageInterface`. Exact Xbox and PlayStation
requirements must be verified in their partner SDK documentation when those ports
begin.

SDL supplies the portable storage operations and selects an appropriate location;
it does not invent the game's save format, schema, migration rules, or recovery
policy. Those remain Codename Hover responsibilities inside `PersistenceService`.

Do not hardcode `~/.config`, `AppData`, macOS Library paths, removable-media
paths, or console locations. SDL may use different real locations on each system;
game code should use relative paths inside the opened storage container.

## Separate files by portability

Keep machine-specific data separate from player data so cloud rules are safe:

| Data | Initial representation | Cloud policy |
| --- | --- | --- |
| Display and renderer settings | `settings.ini` | local only |
| Audio, accessibility, and general preferences | initially in `settings.ini` | decide per field |
| Input bindings | separate settings section or file | sync only when device-independent |
| Progress, unlocks, and career state | versioned `profile.json` initially | suitable for cloud |
| Records and statistics | versioned save data | suitable for cloud |
| Ghosts/replays | individual versioned files | optional; watch size |
| Shader caches and diagnostics | cache/log files | never cloud-save |

INI is appropriate for settings because users can inspect and repair it. JSON is
a practical early save format because it is inspectable while schemas are
changing. A final binary format is optional, not an automatic improvement. Every
save format needs an explicit schema version regardless of representation.

Avoid cloud-syncing resolution, selected monitor, graphics quality, or other
machine-specific settings. Steam Cloud can use Auto-Cloud rules or its API, but
it should consume files produced by `PersistenceService`; Steam calls must not
enter game logic.

## Reliability rules

- Validate schema version, field ranges, file sizes, and required identifiers on
  load; reject or migrate deliberately.
- Preserve unknown or newer data only when the selected format and migration path
  can do so safely.
- Write a temporary replacement, finish and flush/close the storage operation,
  then replace the live save using the strongest transaction supported by that
  backend. Keep a last-known-good backup where platform rules allow it.
- Treat checksums as corruption detection, not protection from deliberate
  editing.
- Save at explicit safe points and coalesce rapid settings changes; never write
  every rendered frame.
- Surface storage-full, permission, removed-user, and failed-write cases without
  destroying the previous valid save.
- Close storage when practical so remote or platform-backed implementations can
  flush batched operations.
- Test upgrade, missing-file, corrupt-file, interrupted-write, and unsupported
  future-version paths with deterministic unit tests.

## Platform behavior

- Windows, Linux, macOS, and Steam Deck: begin with `SDL_OpenUserStorage` using
  organization `Speeding Dog` and application `Codename Hover` until final public
  identifiers are chosen.
- Steam: add cloud synchronization later for portable player data, not for local
  display settings. Test saves moving between Windows and Linux.
- Xbox and PlayStation: select the active platform user first and obey native save
  containers, quotas, suspend/resume, and error/UI rules through a platform
  adapter. Public desktop assumptions are not sufficient for certification.

This architecture makes the serialization portable; it does not imply that a PC
save can be copied manually into a console save container or that console work is
automatic.

## References

- [SDL3 user storage](https://wiki.libsdl.org/SDL3/SDL_OpenUserStorage)
- [SDL3 storage overview](https://wiki.libsdl.org/SDL3/CategoryStorage)
- [SDL3 custom storage interface](https://wiki.libsdl.org/SDL3/SDL_StorageInterface)
- [SDL3 preference path fallback](https://wiki.libsdl.org/SDL3/SDL_GetPrefPath)
- [Steam Cloud documentation](https://partner.steamgames.com/doc/features/cloud?l=swedish&language=english)
