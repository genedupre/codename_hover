# Codename Hover

Codename Hover is an early native-Linux prototype for a low-poly futuristic
anti-gravity arcade racer. The current executable provides `runway`, `oval`, and
banked `speedway` regression scenarios plus `speedway_physics`, the first playable
world-space momentum, grip, and directional-drift experiment. `handling_lab`
provides a wide flat surface and additional live telemetry for controlled handling
experiments.

## Build on the development laptop

Initialize the pinned dependencies and configure the development build once:

```bash
git submodule update --init --recursive
./tools/fetch-shadercross-linux-x64.sh
cmake --preset development
```

Build and run:

```bash
cmake --build --preset development
./build/development/codename_hover --scenario speedway_physics
```

Use `--list-scenarios` to see every available development scenario.
For measured handling work, run:

```bash
./build/development/codename_hover --scenario handling_lab
```

## Deploy to Steam Deck

The Deck is a playtest target rather than a development machine. Its MicroSD card
is mounted at `/run/media/deck/SR01T`; development builds live outside Steam's
managed `steamapps` directory at:

```text
/run/media/deck/SR01T/development/codename_hover
```

With the Deck awake on the local network and the `steamdeck` SSH host alias
working, build and deploy from the laptop:

```bash
cmake --build --preset development
./tools/deploy-deck.sh
```

The script creates the target directory, incrementally rsyncs the executable and
compiled `shaders/` directory, and runs `--list-scenarios` remotely as a headless
startup check. It does not copy source files or delete unrelated remote files.

Then run this from a terminal on the Deck's graphical desktop:

```bash
/run/media/deck/SR01T/development/codename_hover/codename_hover --scenario speedway_physics
```

Deployment settings can be overridden when another Deck, mount, or build tree is
used:

```bash
HOVER_DECK_HOST=steamdeck \
HOVER_DECK_ROOT=/run/media/deck/SR01T/development/codename_hover \
HOVER_BUILD_DIRECTORY=build/development \
./tools/deploy-deck.sh
```

Set `HOVER_DECK_SSH_CONFIG` to an explicit SSH config file only when the default
SSH configuration cannot be used.
