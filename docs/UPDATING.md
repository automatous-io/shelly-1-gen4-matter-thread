# Updating the firmware

**[README](../README.md)** > **Updating** · [Report an issue](../../../issues/new)

This guide covers updating a supported Shelly Gen4 device that is already running the Automatous firmware with Matter OTA support. There are three paths, and the right one depends on whether you want to keep the device's Matter commissioning or not. If you have not flashed the firmware yet, start with the [Flashing Guide](FLASHING.md).

If your Shelly 1 Gen4 is running an Automatous build earlier than v2.0.0, update it with a one-time full reflash over [UART](FLASHING.md#flash-with-usb-uart). The partition layout changed in v2.0.0; this one step erases commissioning and needs to be re-commissioned afterward. From v2.0.0 onward this requirement no longer applies, and further updates are delivered via OTA. The Shelly 1 Mini Gen4 has had the OTA-capable layout since its first release.

---

## Contents

- [Choosing an update path](#choosing-an-update-path)
- [Matter OTA through Home Assistant](#matter-ota-through-home-assistant)
- [ESP-IDF CLI](#esp-idf-cli)
- [Building the OTA image](#building-the-ota-image)

---

## Choosing an update path

| Path | Preserves commissioning | Hardware | Method |
|---|---|---|---|
| Matter OTA through Home Assistant | Yes | None | OTA |
| ESP-IDF CLI | Yes | USB-UART adapter | UART flash |
| ESPConnect | No | USB-UART adapter | UART flash |

The easiest path is Matter OTA. The new firmware is sent to the device over Thread, the device writes it to its inactive slot and reboots, and its Matter commissioning, Thread credentials, and ecosystem pairings all survive. No wires, no re-pairing.

ESPConnect is the full reinstall covered in the [Flashing Guide](FLASHING.md#flash-with-espconnect). It is listed here for comparison only. It erases flash, and therefore commissioning, so it is for a first install or switching variants, and not for an in-place update.

---

## Matter OTA through Home Assistant

Because the firmware uses Matter test credentials, only the Home Assistant ecosystem is supported and documented in this repo to serve OTA updates. Home Assistant's Matter Server acts as the OTA provider; you place an `.ota` image where it can read it, and it offers that image to any eligible commissioned device whose vendor and product ID match.

Verified on **Matter Server add-on, version 9.0.2**.

### 1. Enable Test Net DCL

The Matter Server only serves uncertified OTA images when Test Net DCL is enabled. This is the same setting that lets the test credentialed device commission in the first place.

1. Go to **Settings → Apps → Matter Server → Configuration**.
2. Turn on **Enable test-net DCL usage**.
3. **Restart** the add-on.

Enabling it also points the server at its OTA folder, `/addon_configs/core_matter_server/updates/`. Without Test Net DCL, custom `.ota` files are ignored.

### 2. Get the OTA image

Download the `.ota` for your device and variant from the [latest release](../../../releases/latest), or [build it yourself](#building-the-ota-image). The image is named `automatous-io-{hardware}-{variant}-vX.Y.Z.ota`. Its version must be higher than what the device is running, or the server has nothing newer to offer.

### 3. Place the image

The OTA folder lives inside the add-on's config directory. The simplest way to reach it is the Samba Share add-on.

1. Install and start the **Samba Share** add-on if you have not already.
2. From your computer, open the `addon_configs` share, then `core_matter_server`, then `updates` (full path `/addon_configs/core_matter_server/updates/`). Create the `updates` folder if it does not exist.
3. Copy the `.ota` file into `updates`.

### 4. Restart the add-on to import

The server scans the `updates` folder **at startup**. Restart the Matter Server add-on to import the image right away rather than waiting for the next restart or OTA update availability interval.

1. **Settings → Apps → Matter Server → Restart.**
2. Watch the add-on log. On a successful import it logs `Imported OTA file: …`, and the `.ota` **disappears** from the `updates` folder. That is expected: the server copies the image into its own storage location and deletes the source. The image is now loaded.

### 5. Install the update

Once the image is loaded, the update is available in two places. Use whichever you prefer:

- On the device page in Home Assistant, where its firmware update appears.
- In the Matter Server add-on, by selecting the node and choosing **Update**.

1. Trigger the update from either location above.
2. The device downloads over Thread, writes its inactive slot, verifies, and reboots into the new firmware. If the update does not appear right away, the device queries on its own timer; power-cycling it prompts an immediate check.
3. The device comes back reporting the new version, and the server then offers nothing further. A bad image rolls back to the previous slot. A failed update does not brick a commissioned device.

> 💡 **One image updates every matching device.** The provider offers the stored image by vendor and product ID; a single `.ota` updates every device of that variant on your Thread network. Publish once, and each matching device picks it up the next time it checks in.

---

## ESP-IDF CLI

If you build from source and have a UART adapter on the device, `idf.py flash` updates the bootloader, partition table, and application while leaving NVS and commissioned state intact. See [Flash your build](BUILDING.md#flash-your-build).

---

## Building the OTA image

`scripts/make-matter-ota.py` wraps a variant's built application as a Matter `.ota`, reading the vendor and product ID from the build's `sdkconfig` and the software version from `CHIPProjectConfig.h`; the image always matches the firmware it came from and can only target the device and variant it was built for. See [Building from Source](BUILDING.md#build-the-release-artifacts).

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Stock Flash Partitions](PARTITIONS.md) — stock flash layout and the factory `shelly` partition
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
