# Building from Source

**[README](../README.md)** > **Building from Source** · [Report an issue](../../../issues/new)

This guide covers building the Automatous firmware from source. If you just want to flash pre-built firmware, see the [Flashing Guide](FLASHING.md) instead. You do not need to build anything.

Builds run in a dev container. Everything the build needs is installed in the image at fixed versions, and produces the same firmware as a build anywhere else.

---

## Contents

- [Requirements](#requirements)
- [Repository structure](#repository-structure)
- [Set up the container](#set-up-the-container)
- [Build](#build)
- [Build the release artifacts](#build-the-release-artifacts)
- [Flash your build](#flash-your-build)

---

## Requirements

- **[Docker](https://docs.docker.com/get-docker/)**
- **VS Code** with the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension, for the editor workflow. Optional — the container also runs from a plain shell.
- **macOS or Linux.** The Windows build path is not currently tested.

The container is the only supported build environment. A local ESP-IDF and esp-matter install may work, but it isn't tested, and SDK version drift is the usual reason a build comes out different from the released firmware. If a problem doesn't reproduce in the container, it's hard to help.

The image is built from `.devcontainer/Dockerfile` and contains:

| Component | Version |
|---|---|
| ESP-IDF | `v5.5.2` |
| ESP-Matter | `2cb668c95de4f24786d20b7cb03c171d6e27b79e` |
| connectedhomeip (esp-matter submodule) | `8f943388af4d12dc5c484eae21b22723e03c3616` |

These are the versions the released binaries were produced with. Newer versions may build, but they are not what shipped.

ESP-Matter is used as a cloned repository rather than a component from the registry, and the build reads it through `ESP_MATTER_PATH`. The pinned esp-matter commit is on `main`, not a release branch, which is why no published `espressif/esp-matter` image matches it and the image is built here instead.

connectedhomeip is a submodule of esp-matter, so checking out the esp-matter commit and updating its submodules pulls the matching connectedhomeip commit automatically — it is not pinned separately. It lives in Espressif's connectedhomeip fork, not the upstream CSA repository, so those commits will not resolve there.

The Dockerfile pins the toolchain. The `dependencies.lock` in each variant directory pins the components the build pulls from the registry. Between them every input is pinned, so rebuilding a commit gives you the binaries that commit produced.

---

## Repository structure

The repository organizes firmware by product and variant:

```
source/
├── shelly-1-gen4/
│   ├── light/           # Matter On/Off Light, latching relay (released)
│   ├── opener/          # Matter On/Off Plug-in Unit + Contact Sensor, momentary pulse (released)
│   ├── outlet/          # Matter On/Off Plug-in Unit, latching relay, SW kept as a wall toggle (released)
│   └── light-switch/    # Matter On/Off Light Switch, detached relay + SW input bound to other Matter devices (released)
└── shelly-1-mini-gen4/
    └── outlet/          # Matter On/Off Plug-in Unit, latching relay (released)
```

Each variant is a self-contained ESP-IDF project. Build commands run from inside the variant directory. The examples below build the Shelly 1 Gen4 `light` variant; to build a different one, substitute its hardware and variant directories in the `cd` command.

---

## Set up the container

Clone the repository:

```bash
git clone https://github.com/automatous-io/shelly-1-gen4-matter-thread.git
cd shelly-1-gen4-matter-thread
```

Then use **one** of the two below. Same environment either way.

### In VS Code

Open that folder and run **Dev Containers: Reopen in Container** from the Command Palette. VS Code builds the image, starts the container, and reopens the workspace inside it. Nothing else is needed — skip to [Build](#build).

### From a shell

If you are not using VS Code, build the image and start a container directly:

```bash
docker build -t automatous-io/shelly-gen4-builder:idf-v5.5.2-matter-2cb668c9 .devcontainer

docker run --rm -it \
  -v "$PWD":/workspaces/shelly-1-gen4-matter-thread \
  -w /workspaces/shelly-1-gen4-matter-thread \
  -e SDKCONFIG_DEFAULTS=sdkconfig.defaults.c6_thread_shelly \
  automatous-io/shelly-gen4-builder:idf-v5.5.2-matter-2cb668c9 bash
```

The first build takes roughly 20–25 minutes and produces an image of about 18 GB. It compiles the Matter SDK's build environment. After that it starts in seconds, and it only rebuilds if the Dockerfile changes.

Both SDKs are exported in every shell in the container, and `SDKCONFIG_DEFAULTS` is already set. Note that esp-matter's CMake overrides that environment variable during a build, so [Build](#build) still passes the defaults file with `-D`. To confirm which revisions an image holds:

```bash
cat /opt/esp/esp-idf-version.txt          # v5.5.2
cat /opt/esp/esp-matter-commit.txt        # 2cb668c9...
cat /opt/esp/connectedhomeip-commit.txt   # 8f943388...
```

---

## Build

A variant has to be configured once before it can be built. From the repository root inside the container:

```bash
cd source/shelly-1-gen4/light
idf.py -DSDKCONFIG_DEFAULTS=sdkconfig.defaults.c6_thread_shelly set-target esp32c6
idf.py build
```

After that first `set-target`, the variant's `build/` directory records both the target and the defaults file, so every later build in that variant is just:

```bash
idf.py build
```

Repeat the two-command form for each variant you build, and again after deleting a variant's `build/` directory or `sdkconfig`.

Both flags are needed, for different reasons:

- `-DSDKCONFIG_DEFAULTS` supplies the C6 and Thread settings. The container exports `SDKCONFIG_DEFAULTS` too, but esp-matter's `cmake_common/components_include.cmake` overwrites the CMake variable of that name, and ESP-IDF only reads the environment when that variable is empty. Passing `-D` puts the file where esp-matter won't drop it.
- `set-target esp32c6` sets `IDF_TARGET` on the CMake command line. Each variant's `CMakeLists.txt` picks the device HAL before `project.cmake` runs, and an empty `IDF_TARGET` there is treated as `esp32`. Without it a clean checkout picks the esp32 HAL and stops with `please set esp32 as the IDF_TARGET`.

The build produces individual `.bin` files in the `build/` directory. The application image is named after the variant; the light build produces `build/light.bin` and the opener build produces `build/opener.bin` and so on. The `Project build complete` line at the end of the build prints its exact path.

A `build/` directory is tied to the environment that created it, because CMake caches absolute toolchain paths. If you switch between containers, delete `build/` and `sdkconfig` first.

---

## Build the release artifacts

Each install and update path needs a different packaged artifact, and all three come from the same build:

| Artifact | Path it serves | Built with |
|---|---|---|
| `automatous-io-{hardware}-{variant}-vX.Y.Z.bin` | UART / ESPConnect full install | `idf.py merge-bin` |
| `automatous-io-{hardware}-{variant}-vX.Y.Z-ota.zip` | Shelly web UI install | `scripts/make-webui-ota-zip.py` |
| `automatous-io-{hardware}-{variant}-vX.Y.Z.ota` | Matter OTA update | `scripts/make-matter-ota.py` |

The examples below build the `light` variant; for another, point the script at its directory or substitute its name.

### Merged binary (UART)

A single `.bin` for flashing at offset `0x0`, matching the released binaries. From the variant directory:

```bash
idf.py merge-bin
```

It reads the flash mode, flash size, and every partition offset from the build's flash arguments and writes `build/merged-binary.bin` in the `/build` directory. Rename it to `automatous-io-{hardware}-{variant}-v{version}.bin`.

### Web UI package

The `.zip` that installs through the Shelly web UI. Run it from the repository root pointed at a variant directory:

```bash
python3 scripts/make-webui-ota-zip.py source/shelly-1-gen4/light
```

It reads the variant and version from the build, bundles the bootloader, partition table, otadata, and application with an empty filesystem image, and writes `automatous-io-{hardware}-{variant}-vX.Y.Z-ota.zip` next to the build.

### Matter OTA image

The `.ota` served to commissioned devices through Home Assistant:

```bash
python3 scripts/make-matter-ota.py source/shelly-1-gen4/light
```

It calls `ota_image_tool.py` from the connectedhomeip checkout, found via `ESP_MATTER_PATH`. It has to run inside the container. It reads the vendor and product ID from the build's `sdkconfig` and the software version from `CHIPProjectConfig.h`; the image always matches the firmware it came from and can only target the device and variant it was built for. The output is `automatous-io-{hardware}-{variant}-vX.Y.Z.ota`. See [Updating](UPDATING.md) for how to serve it.

---

## Flash your build

This is for pushing a build you just made onto a device already running this firmware. For a first install, wiring, or putting stock firmware back, see the [Flashing Guide](FLASHING.md).

The container has esptool and pyserial, so it can flash directly. What it needs is access to the serial port.

Put the Shelly into [flash mode](FLASHING.md#enter-flash-mode) and connect the UART adapter first.

### Pass the port through (Linux, Docker Engine)

This requires plain Docker Engine — Docker Desktop runs a virtual machine that cannot see USB devices, on any platform. On macOS and Windows, build in the container and flash from the host instead, following [Flash with USB-UART](FLASHING.md#flash-with-usb-uart).

Add the device to `runArgs` in `.devcontainer/devcontainer.json`, adjusting the path to your adapter:

```jsonc
"runArgs": ["--add-host=host.docker.internal:host-gateway", "--device=/dev/ttyUSB0"],
```

Rebuild the container, then from the variant directory:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Don't commit that line. Docker won't start a container when the device is missing, so it breaks for everyone who isn't flashing.

### What flashing does

```bash
idf.py -p <PORT> flash monitor
```

This writes the bootloader, partition table, and application image but leaves the `nvs` partition intact; Matter fabric and Thread credentials are preserved and the device does not need re-commissioning. Do not use `idf.py erase-flash`, which wipes everything including commissioning.

For a first flash, or to install or update through a packaged artifact, see [Build the release artifacts](#build-the-release-artifacts) for which file each path uses, then the [Flashing Guide](FLASHING.md) for the merged `.bin` or web UI `.zip`, or [Updating](UPDATING.md) for the Matter `.ota`.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
