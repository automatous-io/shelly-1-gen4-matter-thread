# Warranty, Factory Keys, and Reversibility

**[README](../README.md)** > **Warranty, Factory Keys, and Reversibility** · [Report an issue](../../../issues/new)

This page explains what installing third-party firmware does to a Shelly Gen4 device, whether you can return the device to stock, and what that depends on.

> ⚠️ **Default assumption: treat flashing as one-way.** You can only restore the device to a fully functional factory state if you captured a full-chip backup over UART before flashing. Without that backup, flashing is irreversible. This is verified for the UART path and explained below. You are also voiding your warranty. **Read this whole document before attempting to flash your Shelly.**

---

## Contents

- [What flashing changes](#what-flashing-changes)
- [Factory keys](#factory-keys)
- [Can you go back](#can-you-go-back)
- [UART path, verified recoverable](#uart-path-verified-recoverable)
- [Web UI path, one-way](#web-ui-path-one-way)
- [The eFuse marker](#the-efuse-marker)
- [Verification log](#verification-log)

---

## What flashing changes

Installing this firmware replaces the stock Shelly firmware, and that has consequences beyond the new features. It voids your warranty, and Shelly cannot provide technical support for a device running third-party code. It also removes the factory keys. Stock firmware uses factory-provisioned keys to reach Shelly Cloud and to receive official OTA updates; once the stock firmware is overwritten those services are unavailable until the original firmware is restored.

Installing over the web UI has also been verified to burn a permanent marker into the chip's eFuse, which appears to record that the device has run non-Shelly firmware. It does not change how the device works and it does not block a return to stock, but it cannot be removed. See [The eFuse marker](#the-efuse-marker) for additional detail.

This path also depends on Shelly, and they could narrow it. The install works because the stock web UI accepts the package and because a restored device still reconnects to Shelly Cloud and the official OTA service. A future stock release could refuse third-party packages, and the eFuse marker would give Shelly a reliable way to recognize a device flashed with third-party firmware and, if they chose, decline cloud or OTA service to it. None of that is happening as of today, but it is outside this project's control. Treat the current ease of install and restore as a snapshot, not a guarantee.

None of this is a criticism of Shelly. The device is deliberately flashable, which is what makes this project possible. The point of this page is to be honest about the trade-off so you can make an informed decision.

---

## Factory keys

Shelly devices are provisioned at the factory with per-device keys and certificates. These authenticate the device to Shelly Cloud and to the official OTA update service, and they are unique per device.

The keys reside in flash, in a factory or NVS type partition. A full-chip backup captures them regardless of which partition they sit in, because it copies every byte of the 8MB flash. Anything bound to eFuse, the one-time-programmable silicon, cannot be erased by flashing and survives no matter what you do. The recoverable-or-not question is therefore entirely about the flash-resident, and therefore entirely about whether your backup captured them. Restore the full flash image and the keys come back. Lose the image and they are gone.

---

## Can you go back

Reversibility depends on two things, not one. The first is the flash method, meaning UART, which can read the chip before writing, versus the web UI path, which is write-only and cannot read first. The second is backup completeness, meaning a full 8MB image versus a partial image versus none at all.

| Method | Backup possible | Full-chip backup captures keys | Outcome |
|---|---|---|---|
| UART | Yes, reads the chip before writing | Yes | Recoverable |
| Web UI | No, write-only | No image exists | One-way |

This is not purely a matter of UART being safe and OTA being unsafe. A UART user who takes only a partial backup, or who runs `erase_flash` without a complete image to restore, is in the same one-way situation as a web UI user. Completeness, not just method, is what protects you. The rule that actually keeps you safe is to capture a full 8MB backup before flashing, or to accept that the change may be permanent.

---

## UART path, verified recoverable

The flashing procedure in the [Flashing Guide](FLASHING.md) reads the entire 8MB flash to a file before writing anything. Because that image includes the factory keys, restoring it returns the device to a fully functional factory state. Functional is the precise word. The device behaves as factory, but a marker burned into eFuse during a web UI install cannot be undone by a restore. See [The eFuse marker](#the-efuse-marker).

This has been tested, not assumed. On several Shelly 1 Gen4s of hardware revision v0.1.2, a full 8MB backup was taken before flashing then restored as a full image. After the restore the devices connected to Shelly Cloud and successfully applied a cloud-delivered OTA update, moving from `1.7.0-gbe7545d` to `1.7.5-g9979d16`. Both the Shelly Cloud connection and the cloud OTA update require the factory keys; a successful round-trip confirms the keys were preserved in the backup and restored intact.

For the exact backup and restore steps, see [Restoring stock firmware](FLASHING.md#restoring-stock-firmware).

> **Scope.** This result is for three separate devices on hardware revision v0.1.2, with a backup taken before the first flash. It is strong evidence that the UART full-chip method is reversible. It does not generalize to partial backups, other methods, or the OTA flash path below.

---

## Web UI path, one-way

> ⚠️ **Status: supported, and irreversible without a UART backup first.** This project ships a web UI package and documents installing it through the Shelly web UI. A web UI flash cannot be undone unless you captured a full-chip UART backup beforehand.

Shelly's web UI installs firmware by uploading a file over the network, with no UART connection. This path is confirmed working on the Shelly 1 Gen4 and Shelly 1 Mini Gen4 and verified on stock firmware 1.7.0, 1.7.5, and 2.0.0. For the steps, see [Flash with the Shelly web UI](FLASHING.md#flash-with-the-shelly-web-ui).

A web UI flash cannot read the chip before writing, so no full-chip backup can be made, and the factory keys cannot be saved. A device flashed this way cannot be returned to a fully functional stock state unless a UART backup was captured first.

The web UI is the easiest way in, and a UART backup is the only way back. If keeping the option to restore to stock matters to you, take a full-chip UART backup before flashing over the web UI or use the UART flash path which captures that backup as part of flashing.

---

## The eFuse marker

Installing through the Shelly web UI writes to eFuse. We saw this directly on devices whose eFuse read clean beforehand. The web UI install burned user-data eFuse BLOCK3 during the flash and verified the write by reading it back. It is interpreted as a marker that records the device has run non-Shelly firmware — the kind of flag a manufacturer could check on a returned unit. BLOCK3 is `BLOCK_USR_DATA`, a general-purpose user-data block (what Espressif's docs describe as space for user parameters), not secure boot or anti-rollback. The marker cannot be cleared by restoring a flash backup or by any other means.

This is separate from the backup question above, and it does not change how the firmware works. A device flashed over the web UI can be identified afterward as having run third-party firmware.

This is confirmed for the Shelly web UI install path. A direct UART flash does not run `shelly_update.cpp` and likely does not set the marker, but that has not been fully tested yet.

---

## Verification log

A running record of what has been tested, scoped to the exact conditions.

- **2026-06-18 and 2026-06-24.** UART full-chip backup and restore on hardware revision v0.1.2. After restore, Shelly Cloud connected and a cloud OTA update applied, moving from `1.7.0-gbe7545d` to `1.7.5-g9979d16`. Confirms the UART full-chip path is reversible with the keys intact.
- **2026-06-27.** Three Shelly 1 Gen4 units with eFuse read clean beforehand had third-party firmware installed through the Shelly web UI. The install burned user-data eFuse BLOCK3, logged as `BURN BLOCK3 - OK` and verified by read-back. The devices booted and ran the Automatous firmware afterward. Confirms the web UI install path writes a permanent marker into user-data eFuse.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
