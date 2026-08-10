# Development environment audit

Last audited: 2026-07-27

This document records observed development-laptop capabilities and tool gaps. It
is operational state, not a portable project requirement. Re-run the relevant
checks after major operating-system, driver, or hardware changes.

## Verdict

The laptop is suitable for the bootstrap milestone. Its Intel GPU exposes a
working Vulkan implementation in the real desktop session, and the CPU, memory,
and storage are ample. The intended Clang/Ninja toolchain and shader tools were
installed and verified after the initial audit. The discrete NVIDIA GPU is not
currently available as a Vulkan device, but it is not required to begin.

Step 2 selected the bootstrap revisions and package set in
`development-hardware.txt`; that file remains the authoritative baseline.

## Host

| Item | Observed state |
| --- | --- |
| Operating system | Ubuntu 24.04.4 LTS, x86-64 |
| Kernel | Linux 6.14.0-37-generic |
| Virtualization | None detected; this is the physical host |
| CPU | Intel Core Ultra 7 155H, 16 cores / 22 logical CPUs |
| Memory | 30 GiB RAM, approximately 23 GiB available during audit |
| Swap | 30 GiB, unused during audit |
| Project filesystem | Approximately 394 GiB available during audit |
| Desktop session | GNOME on Wayland, with XWayland available |
| Internal display | 1920x1200 at approximately 59.88 Hz |

This is more than sufficient for compiling and running the first milestones.

## Graphics

Two physical graphics adapters are present:

- Intel Meteor Lake-P Arc integrated graphics, using the `i915` kernel driver;
- NVIDIA GeForce RTX 4070 Max-Q / Laptop GPU.

The real desktop session successfully reports the Intel Arc GPU through Vulkan:

| Item | Observed state |
| --- | --- |
| Vulkan instance | 1.3.275 loader |
| Intel Vulkan API | 1.4.318 |
| Intel userspace driver | Mesa 25.2.8 open-source Intel driver |
| Intel conformance | Vulkan 1.4.0.0 reported |
| Software fallback | llvmpipe is also present |

The NVIDIA adapter was detected on PCI, but `lspci` reported no active driver for
it. The `nouveau` module was loaded without owning the device, no NVIDIA device
appeared in `vulkaninfo`, and NVIDIA management tools were absent. Ubuntu currently
advertises proprietary/open NVIDIA driver choices, but changing GPU drivers is a
separate system-administration decision and is not needed for the first triangle.

Develop and measure initially on the Intel GPU. Revisit the NVIDIA configuration
later if discrete-GPU testing becomes useful; do not delay bootstrap work for it.

## Development tools

### Present

| Tool | Observed version or state |
| --- | --- |
| GCC/G++ | 13.3.0; provides an available fallback C++ compiler |
| CMake | 3.28.3 |
| Git | 2.54.0 |
| GDB | 15.1 |
| GNU Make | 4.3 |
| pkg-config | 1.8.1 |
| Vulkan tools | 1.3.275 |
| Mesa Vulkan drivers | 25.2.8, 64-bit and 32-bit runtime packages |
| Visual Studio Code | 1.129.1 |
| Steam client | Installed |

### Missing during audit

- Clang/clang++ and clangd;
- Ninja;
- clang-format and clang-tidy;
- LLDB;
- SDL3 development files;
- SDL_shadercross and shader compiler/validation tools;
- RenderDoc;
- Neovim;
- ccache and mold.

Not every missing item is a bootstrap dependency. Neovim is optional because an
editor is already installed. RenderDoc, ccache, mold, LLDB, and broader analysis
tools can wait until they provide immediate value. Step 2 will identify the
minimum required compiler, build, SDL, shader, and Linux platform packages.

Only `build-essential`, `libudev-dev`, and `libx11-dev` were found from an initial
probe of common SDL Linux build dependencies. Treat that probe as incomplete until
checked against the selected SDL revision's official instructions.

### Installed and verified after the audit

The selected bootstrap package set was installed on 2026-07-27. The development
preset now uses Clang/clang++ 18.1.3 and Ninja 1.11.1; clangd, clang-format, and
clang-tidy 18.1.3 are also available. The pinned repository-local
SDL_shadercross executable is installed and hash-verified.

The Clang/Ninja build confirmed SDL support for Wayland, X11, Vulkan/SDL_GPU,
Linux haptics, HIDAPI, udev, and USB controller access. The static SDL build,
offline HLSL-to-SPIR-V compilation, executable link, source formatting check,
source analysis, and bootstrap run all succeeded. Optional native audio
development backends remain deferred until the audio milestone.

## Input devices

The laptop keyboard, touchpad, mouse-compatible touchpad device, system buttons,
and audio endpoints were visible. No USB or input device identifying as a game
controller was connected during the audit. A Sony Bluetooth headset created the
newest keyboard-like media-control input event; it is not a controller.

Attach the controller intended for development before the bootstrap input check.
At that point verify both SDL discovery and actual button/axis events rather than
inferring support from device files alone.

## Sandbox caveat

The managed coding shell hides `/dev/dri`, `/dev/input`, USB access, and desktop
surface access by default. Inside that sandbox, `vulkaninfo` saw only llvmpipe and
could not inspect the active display. The read-only audit was repeated outside the
sandbox and then correctly saw Intel Arc Vulkan, the 1920x1200 display, and host
device nodes.

Future interactive game, Vulkan, display, USB, or controller validation may need
explicit permission to run outside the restricted shell. Do not diagnose the
laptop GPU as broken from sandbox-only results.

## Step 2 result

Current official SDL sources were checked on 2026-07-27. The project selected SDL
3.4.10 as a pinned Git submodule, selected an exact official SDL_shadercross build
as an offline host tool, defined HLSL-to-SPIR-V/DXIL/MSL outputs, and chose a
minimal practical Ubuntu package set for the first triangle. Exact revisions,
hashes, packages, and official references are in `development-hardware.txt`.

The selected bootstrap packages are installed, and the SDL CMake feature checks
for Wayland, X11, udev/USB, controller/haptic support, and Vulkan have passed. The
next implementation step is the interactive SDL window and SDL_GPU triangle.
