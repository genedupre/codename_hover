# Platforms and release

## Important freshness note

Commercial fees, store timing, SDK capabilities, operating-system requirements,
and console processes in this file were imported as planning assumptions on
2026-07-26 and were not independently revalidated in this repository. Verify them
against current first-party documentation before relying on them.

## Target order

1. Linux development laptop.
2. Steam Deck running SteamOS.
3. Windows x86-64.
4. macOS if support and signing effort remain justified.
5. Xbox after the PC game is stable and platform access is approved.
6. PlayStation after the PC game is stable and platform access is approved.

The codebase is shared, but each platform gets its own binary, packaging, and
release validation. “All PC platforms” does not mean every Linux distribution or
every historical machine.

## Native Linux

Native Linux is a product requirement. Keep system dependencies minimal and target
a supportable runtime environment suitable for Steam. CI should compile Linux from
the beginning. Before a release, test at least SteamOS/Steam Deck plus documented
desktop Linux configurations rather than claiming universal distribution support.

## Steam Deck

Treat the Deck as target hardware, not the primary development workstation. Avoid
turning its immutable SteamOS installation into a conventional Arch development
machine. Deploy development builds from the laptop over the local network using
Valve-supported tooling or a small, documented SSH/rsync workflow.

Deck validation should cover:

- complete controller operation and correct glyphs;
- controller connect/disconnect behavior;
- readable UI at the native display size;
- startup without a mandatory launcher;
- suspend/resume and focus changes;
- stable frame pacing and sensible power use;
- logs that can be retrieved after a failed run.

## Windows

Introduce a Windows CI build early enough to catch accidental POSIX assumptions.
Test actual execution before the first public demo. SDL_GPU is expected to use an
appropriate D3D12 or Vulkan path, but backend availability and behavior must be
confirmed on supported hardware.

## macOS

SDL_GPU's Metal path makes macOS technically plausible. Shipping still requires
Apple-compatible packaging, signing, notarization, supported architectures, and
ongoing test hardware. The imported planning estimate for Apple Developer Program
membership is USD 99 per year; verify the price and requirements before enrolling.

## Steam

Steamworks calls should be isolated behind game-owned platform-service boundaries.
Achievements, cloud saves, leaderboards, and Steam Input are useful but are not
needed for the first gameplay prototype.

Imported planning assumptions to recheck before onboarding:

- Steam Direct charges USD 100 per product and recoups it after USD 1,000 in
  adjusted gross revenue.
- Onboarding requires legal identity, tax, bank, store-page, pricing, build, and
  review information.
- A release has a minimum waiting period after paying the fee and a minimum public
  Coming Soon period.
- Initial review commonly takes multiple days.
- Steamworks runtime integration is not required merely to sell a game.

Do not schedule a release using those values until current Steamworks documentation
has been checked.

## Xbox and PlayStation

Console ports are plausible because SDL and SDL_GPU have relevant platform paths,
but access, exact capabilities, and integration details are controlled by partner
programs and may be under NDA. Public desktop APIs are not evidence that a console
build is ready.

The major console workload is expected to be platform behavior and certification,
including:

- user/profile selection;
- save data and storage failures;
- suspend, resume, and activity transitions;
- controller removal and reassignment;
- achievements or trophies;
- system UI and platform error flows;
- privacy and network policies;
- packaging, certification, QA, and updates.

Apply to platform programs and obtain current SDK requirements before committing to
a date or cost. Do not let speculative console abstractions delay a strong PC
vertical slice.
