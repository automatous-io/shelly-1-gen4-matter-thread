# Stock Flash Partitions

**[README](../README.md)** > **Stock Flash Partitions** · [Report an issue](../../../issues/new)

This page records how stock Shelly firmware lays out the 8MB flash on Gen4 devices, and what Shelly keeps in its factory `shelly` partition.

> **Scope.** The partition contents below were observed on one device: a Shelly 2PM Gen4 (S4SW-002P16EU) that ran stock firmware 2.0.1 until it was installed through the web UI. They were read afterwards by the installed third-party firmware. The install doesn't write `shelly` or `fs_1`, so those hold what stock left there.

---

## Contents

- [Partition table](#partition-table)
- [What flashing does to each partition](#what-flashing-does-to-each-partition)
- [The shelly partition](#the-shelly-partition)
- [Setup codes and the verifier](#setup-codes-and-the-verifier)
- [The other stock data partitions](#the-other-stock-data-partitions)
- [Check your own unit](#check-your-own-unit)
- [Provenance](#provenance)
- [Status](#status)

---

## Partition table

The partition table sits at `0x10000`. Every variant's `partitions.csv` in this repository follows the stock offsets, so the web UI package lands where stock expects it.

| Name | Type | SubType | Offset | Size | Stock use | This firmware |
|---|---|---|---|---|---|---|
| otadata | data | ota | 0x11000 | 0x2000 | Selects the active app slot | Used |
| nvs | data | nvs | 0x14000 | 0xc000 | Not inspected | Used, for Matter and Thread state |
| app_0 | app | ota_0 | 0x20000 | 0x300000 | App slot A | Used |
| fs_0 | data | 0x82 | 0x320000 | 0xe0000 | File system, likely paired with `app_0` | Not used |
| app_1 | app | ota_1 | 0x400000 | 0x300000 | App slot B | Used, as the second OTA slot |
| fs_1 | data | 0x82 | 0x700000 | 0xe0000 | LittleFS file system, observed | Not used |
| zb_storage | data | 0x81 | 0x7e8000 | 0x8000 | Zigbee storage, by its name | Not used |
| shelly | data | 0x88 | 0x7f0000 | 0x10000 | Factory data: Matter credentials, setup codes, cloud key | Not used |

`fs_0`, `fs_1`, `zb_storage` and `shelly` are kept in this firmware's table only so the offsets match stock. The firmware never reads or writes them. It has no `fctry` partition: the DAC it presents is the esp-matter test DAC compiled into the app. See [Certification](CERTIFICATION.md#test-credentials).

---

## What flashing does to each partition

This follows from the [Flashing Guide](FLASHING.md) and `scripts/make-webui-ota-zip.py`.

| Region | Web UI install | UART full install |
|---|---|---|
| Bootloader (0x0) | Replaced | Erased, then written |
| Partition table (0x10000) | Replaced | Erased, then written |
| otadata | Written | Erased, then written |
| nvs | Filled with 0xFF | Erased |
| app_0 | Written | Erased, then written |
| fs_0 | Written as a blank (0xFF) image | Erased |
| app_1 | Not in the package | Erased |
| fs_1 | Not in the package | Erased |
| zb_storage | Not in the package | Erased |
| shelly | Not in the package | Erased |

The UART path erases the whole chip before writing the merged `.bin` at `0x0`. The merged `.bin` holds what ESP-IDF's flash arguments list for the build: the bootloader, partition table, otadata and app. The `shelly` partition is gone after that and comes back only by restoring the full-chip backup. See [Restoring stock firmware](FLASHING.md#restoring-stock-firmware).

The web UI package's manifest has no part for `app_1`, `fs_1`, `zb_storage` or `shelly`. Observed on the 2PM Gen4: after a web UI install, `shelly` and `fs_1` were still intact. The package used there was built for the 2PM, which this repository does not support yet, and its manifest lists the same parts as `make-webui-ota-zip.py`. That does not make the device restorable. By its manifest the install replaces the bootloader and partition table, overwrites `app_0`, and blanks `nvs` and `fs_0`. A return to stock without a UART backup was not tested. See [Reversibility](REVERSIBILITY.md).

---

## The shelly partition

The 64KB `shelly` partition holds a single UTF-8 JSON object starting at offset 0. On the inspected unit the object was 2602 bytes, followed by one NUL byte, then 0xFF to the end of the partition. There is no header, length field or checksum. A reader should stop at the first NUL or 0xFF byte.

The schema below uses placeholder values. Lengths are as observed on one unit; PEM lengths vary by certificate.

```json
{
  "matter": {
    "hw_version": 4,
    "hw_version_str": "v4",
    "setup_manual_code": "00000000000",
    "setup_qr_code": "MT:XXXXXXXXXXXXXXXXXXX",
    "setup_verifier": "<salt base64>,1000,<verifier base64>",
    "dac": "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n",
    "dak": "-----BEGIN PRIVATE KEY-----\n...\n-----END PRIVATE KEY-----\n",
    "pai": "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n",
    "vendor_name": "Shelly",
    "rotating_dev_uid": "<base64>"
  },
  "zigbee": {
    "install_code": "<hex>"
  },
  "factory": {
    "version": 2,
    "model": "<model code>",
    "batch": "<batch id>",
    "cloud_key": "<opaque>",
    "pm_model": "<hex id>"
  }
}
```

| Field | Type | Observed length | Meaning | Sensitivity |
|---|---|---|---|---|
| `matter.hw_version` | int | — | Matter hardware version, `4` on the 2PM Gen4. Not the PCB revision in the [GPIO Map](GPIO.md). | Public |
| `matter.hw_version_str` | string | 2 chars | The same, as a string (`v4`) | Public |
| `matter.setup_manual_code` | string | 11 digits | Manual pairing code | Printed on the sticker |
| `matter.setup_qr_code` | string | 22 chars | QR payload, `MT:` plus base38 | Printed on the sticker |
| `matter.setup_verifier` | string | 162 chars | SPAKE2+ verifier for the setup passcode, see below | Secret |
| `matter.dac` | string | ~750 chars | Device Attestation Certificate, PEM | Public, unique per unit |
| `matter.dak` | string | ~260 chars | DAC private key, PEM PKCS#8, unencrypted | **Private key** |
| `matter.pai` | string | ~700 chars | Product Attestation Intermediate certificate, PEM | Public |
| `matter.vendor_name` | string | 6 chars | `Shelly` | Public |
| `matter.rotating_dev_uid` | string | 24 chars | Rotating Device ID unique ID, base64 of 16 bytes | Secret |
| `zigbee.install_code` | string | 36 hex chars | Zigbee install code, 18 bytes | Secret |
| `factory.version` | int | — | Format version of this object, `2` | Public |
| `factory.model` | string | 13 chars | Shelly model code, such as `S4SW-002P16EU` | Public |
| `factory.batch` | string | 14 chars | Production batch | Public |
| `factory.cloud_key` | string | 294 chars | Shelly Cloud credential | Secret |
| `factory.pm_model` | string | 7 hex chars | Power meter model identifier | Public |

The `dak` field is the device's Matter attestation private key, stored in plaintext in a readable partition. Treat any dump of this partition like a private key file. Do not post it, attach it to an issue, or commit it.

On the inspected unit:

- The DAC was issued to vendor ID `0x1490` (Shelly) and product ID `0x1032`, with a subject CN that begins `Shelly Batch <date>/1032`.
- The DAC's issuer is `Shelly Matter PAI 01` (vendor ID `0x1490`), and the DAC's signature verifies against the stored PAI. The PAI's issuer is `StrongKey Matter G1 PAA`.
- There was no power meter calibration in this partition. `pm_model` names the meter part only.

---

## Setup codes and the verifier

The QR and manual codes encode the same passcode. They differ in how much of the discriminator they carry.

- **QR code** (`setup_qr_code`): carries the version, vendor ID, product ID, commissioning flow, discovery capabilities, the full 12-bit discriminator and the passcode. On the inspected unit it decoded to version 0, vendor ID `0x1490`, product ID `0x1032`, standard commissioning flow and BLE discovery (`0x02`).
- **Manual code** (`setup_manual_code`): 11 digits with a Verhoeff check digit. It carries only the upper 4 bits of the discriminator (the short discriminator), and no vendor or product ID. On the inspected unit its passcode and short discriminator matched the QR code.

`setup_verifier` is three comma-separated fields:

| Field | Encoding | Observed |
|---|---|---|
| Salt | base64 | 24 chars, 18 bytes |
| Iterations | decimal | `1000` |
| Verifier | base64 | 132 chars, 97 bytes: `w0` (32 bytes) followed by the uncompressed P-256 point `L` (65 bytes) |

This is the Matter SPAKE2+ verifier. PBKDF2-HMAC-SHA256 over the passcode, as a 32-bit little-endian integer, with this salt and iteration count gives 80 bytes. Each 40-byte half, reduced modulo the P-256 group order, gives `w0` and `w1`, and `L = w1·G`. On the inspected unit, recomputing `w0` and `L` from the QR code's passcode matched the stored verifier.

---

## The other stock data partitions

Observed on the Shelly 2PM Gen4, which ran stock firmware 2.0.1 before a web UI install.

**`fs_1`** is a LittleFS image (on-disk version 2.1, 4096-byte blocks, 224 blocks). It held the stock firmware's file system.

| File | Contents |
|---|---|
| `conf*.json` | Stock configuration: device, Wi-Fi, Bluetooth, MQTT, RPC and power monitor settings. Includes the Wi-Fi password and the device location. |
| `storage.json` | Runtime state, such as reboot count and cover state |
| `counter_total_0` | Binary, likely energy counters by its name |
| `matter_kvs.json` | Stock firmware's Matter key-value store |
| `bt_kvs.json` | Stock firmware's Bluetooth key-value store, including a BLE identity resolving key |
| `components_storage.json` | A single integer field |
| `ca.pem` | CA certificate bundle, packed DER rather than PEM text |
| `shelly_cloud.pem` | Shelly Cloud CA certificate, packed DER |
| `index.html.gz` | The stock web UI |
| `tzinfo` | Time zone data |

No power meter calibration was found in `fs_1`. Treat a dump of `fs_1` as private too: it holds the Wi-Fi password, the location and Bluetooth keys.

**`fs_0`** was blank (all 0xFF). The web UI package writes a blank image there, so this says nothing about what stock keeps in it.

**`zb_storage`** was blank (all 0xFF).

---

## Check your own unit

Read the [safety warning](FLASHING.md#safety-warning) first: never connect UART to a device on mains. Then, with the device in [flash mode](FLASHING.md#enter-flash-mode), read only the `shelly` partition. `read_flash` only reads; it does not write to the chip.

```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 \
  read_flash 0x7f0000 0x10000 shelly-<MAC>.bin
```

If you already have a full-chip backup from the [Flashing Guide](FLASHING.md#2-back-up-the-original-shelly-firmware), you don't need to read the chip again. The command below accepts either file.

Then print each key with its type and length, never its value:

```bash
python3 scripts/read-stock-partitions.py shelly-<MAC>.bin --keys
```

Given a full-chip backup, `--keys` also lists the files in `fs_0` and `fs_1` with their sizes, and the keys of their JSON files with their types only, without lengths or values. Share the output if you report a different layout. Do not share the `.bin`.

### Read the contents

`scripts/read-stock-partitions.py` prints what the four stock data partitions hold. It never uses the file name to tell which partition a file is.

- **Full-chip backup:** split using the partition table stored in the backup at `0x10000`. If the table carries an MD5, the script checks it first.
- **Single partition:** identified by its contents. A JSON object at offset 0 is `shelly`. A LittleFS image is `fs_0` or `fs_1`; the data can't say which, because the two have the same format.
- **When contents can't decide:** the size gives a hint, labelled as one. On a blank file, for example, 32KB suggests `zb_storage`. If the size disagrees with the contents, the contents win and the script prints a warning.
- **Exact name:** pass `--offset`, the address you read the partition from. The script looks it up in the stock table, or in your own copy of the table if you pass `--table` (read it with `read_flash 0x10000 0xc00 pt.bin`). The partition's subtype then decides how it's read, and contents that don't match are an error.

```bash
python3 scripts/read-stock-partitions.py shelly-1-gen4-stock-<MAC>.bin
python3 scripts/read-stock-partitions.py partition.bin
python3 scripts/read-stock-partitions.py partition.bin --offset 0x700000
```

What it prints:

- **`shelly`:** the JSON in full.
- **`fs_0` and `fs_1`:** the LittleFS files, and the contents of the JSON ones. Listing the files needs `pip install littlefs-python`.
- **`zb_storage`:** not decoded.

Any of them may also be blank (all 0xFF). `--extract <DIR>` writes `shelly.json` to that folder, and each file system's files to a subfolder named after its partition (`fs` when a single image can't be told apart; pass `--offset` to get `fs_1`), readable only by you.

> ⚠️ **Without `--keys`, the output holds secrets.** It prints the device's Matter private key, its Shelly Cloud key and your Wi-Fi password, and `--extract` writes them to disk. Keep them on your own machine. To report a layout, share the `--keys` output above instead.

---

## Provenance

- **Device:** Shelly 2PM Gen4, model S4SW-002P16EU, hardware version 4.
- **Stock firmware before the install:** 2.0.1.
- **Date:** 2026-10-09.
- **Method:** the device was installed through the stock web UI, then updated over Wi-Fi to another build of the same third-party firmware. That firmware then read the `shelly`, `fs_0`, `fs_1` and `zb_storage` partitions and downloaded them. Nothing was written to those partitions.

---

## Status

This firmware does not read the `shelly` partition. Every unit commissions with the shared esp-matter test setup code, as described in [Certification](CERTIFICATION.md#test-credentials). Commissioning with each unit's own sticker code, read from this partition, is proposed separately.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
