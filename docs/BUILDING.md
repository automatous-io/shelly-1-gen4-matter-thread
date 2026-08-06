# Building from Source

**[README](../README.md)** > **Building from Source** · [Report an issue](../../../issues/new)

This guide covers building the Automatous firmware from source. If you just want to flash pre-built firmware, see the [Flashing Guide](FLASHING.md) instead. You do not need to build anything.

---

## Contents

- [Requirements](#requirements)
- [Repository structure](#repository-structure)
- [Setup](#setup)
- [Build](#build)
- [Build the release artifacts](#build-the-release-artifacts)
- [Flash your build](#flash-your-build)

---

## Requirements

- **ESP-IDF v5.5.5**, Espressif's IoT development framework.
- **ESP-Matter**, Espressif's Matter SDK, installed as a cloned repository. The build reads it through the `ESP_MATTER_PATH` environment variable, which esp-matter's `export.sh` sets. It is not pulled from the component registry. Check it out to the pinned commit below rather than building against `main`.
- **macOS or Linux**. The Windows build path is not currently tested.

These binaries are built against pinned toolchain versions. Newer versions may build, but they are not what the released binaries were produced with.

| Component | Version |
|---|---|
| ESP-IDF | `v5.5.5` |
| ESP-Matter | `36c2634e99c884830897e2b9501e2d9a6c9d60fd` |
| connectedhomeip (esp-matter submodule) | `d46cc8c2886cbefc338544bdb2e2f8128f3e9970` |

connectedhomeip is a submodule of esp-matter, so checking out the esp-matter commit and updating its submodules pulls the matching connectedhomeip commit automatically. It lives in Espressif's connectedhomeip fork, not the upstream CSA repository.

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

## Setup

If you don't already have ESP-IDF and ESP-Matter installed, follow Espressif's official setup guides first:

- [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32c6/get-started/index.html)
- [ESP-Matter Get Started](https://docs.espressif.com/projects/esp-matter/en/latest/esp32c6/developing.html)

This project assumes you have both SDKs installed. The build step below sets the ESP32-C6 target.

After cloning esp-matter, check it out to the pinned commit and sync its submodules so your build matches the released binaries:

```bash
cd ~/esp/esp-matter
git checkout 36c2634e99c884830897e2b9501e2d9a6c9d60fd
git submodule update --init --recursive
```

connectedhomeip's GN build needs a generated `build_overrides/pigweed_environment.gni`, which is not checked into the repo. Run its environment bootstrap once per esp-matter checkout to create it:

```bash
cd connectedhomeip/connectedhomeip
scripts/bootstrap.sh
cd ../..
```

Skipping this step fails the build at the `chip_gn` configure step with `Unable to load ".../build_overrides/pigweed_environment.gni"`.

---

## Build

Clone the repository and build the light variant:

```bash
git clone https://github.com/automatous-io/shelly-1-gen4-matter-thread.git
cd shelly-1-gen4-matter-thread/source/shelly-1-gen4/light

# source from wherever you installed esp-idf and esp-matter
# the locations below are examples, adjust to match your install paths
source ~/esp/esp-idf/export.sh
source ~/esp/esp-matter/export.sh

idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults.c6_thread_shelly" set-target esp32c6
idf.py build
```

Run these in the same shell where you sourced the two `export.sh` scripts. ESP-IDF and the `ESP_MATTER_PATH` the build needs will not be set otherwise.

The `-DSDKCONFIG_DEFAULTS` flag points the build at `sdkconfig.defaults.c6_thread_shelly`, which holds the C6 and Thread settings. Without it the build will not produce a Matter over Thread image. `set-target` applies it once, and after that plain `idf.py build` rebuilds incrementally.

The build produces individual `.bin` files in the `build/` directory. The application image is named after the variant; the light build produces `build/light.bin` and the opener build produces `build/opener.bin` and so on. The `Project build complete` line at the end of the build prints its exact path.

---

## Build the release artifacts

Each install and update path needs a different packaged artifact, and all three come from the same build:

| Artifact | Path it serves | Built with |
|---|---|---|
| `automatous-io-{hardware}-{variant}-vX.Y.Z.bin` | UART / ESPConnect full install | `esptool merge_bin` |
| `automatous-io-{hardware}-{variant}-vX.Y.Z-ota.zip` | Shelly web UI install | `scripts/make-webui-ota-zip.py` |
| `automatous-io-{hardware}-{variant}-vX.Y.Z.ota` | Matter OTA update | `scripts/make-matter-ota.py` |

The examples below build the `light` variant; for another, point the script at its directory or substitute its name.

### Merged binary (UART)

A single `.bin` for flashing at offset `0x0`, matching the released binaries. Replace `<VERSION>` with the release version.

```bash
esptool.py --chip esp32c6 merge_bin \
  -o automatous-io-shelly-1-gen4-light-<VERSION>.bin \
  --flash_mode dio \
  --flash_size 8MB \
  0x0     build/bootloader/bootloader.bin \
  0x10000 build/partition_table/partition-table.bin \
  0x11000 build/ota_data_initial.bin \
  0x20000 build/light.bin
```

### Web UI package

The `.zip` that installs through the Shelly web UI. Run it from the repository root pointed at a variant directory:

```bash
python3 scripts/make-webui-ota-zip.py source/shelly-1-gen4/light
```

It reads the variant and version from the build, bundles the bootloader, partition table, otadata, and application with an empty filesystem image, and writes `automatous-io-{hardware}-{variant}-vX.Y.Z-ota.zip` next to the build.

### Matter OTA image

The `.ota` served to commissioned devices through Home Assistant. Run it in the same esp-matter environment you build in, since it calls `ota_image_tool.py` from the connectedhomeip checkout (found via `ESP_MATTER_PATH`):

```bash
python3 scripts/make-matter-ota.py source/shelly-1-gen4/light
```

It reads the vendor and product ID from the build's `sdkconfig` and the software version from `CHIPProjectConfig.h`; the image always matches the firmware it came from and can only target the device and variant it was built for. The output is `automatous-io-{hardware}-{variant}-vX.Y.Z.ota`. See [Updating](UPDATING.md) for how to serve it.

---

## Flash your build

To push a new build to a device already running this firmware without re-pairing it, put the Shelly into [flash mode](FLASHING.md#enter-flash-mode), connect the UART adapter, and from the variant directory run the following. Replace `<PORT>` with your USB-UART serial port.

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