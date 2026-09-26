# Flashing Guide

**[README](../README.md)** > **Flashing Guide** · [Report an issue](../../../issues/new)

This guide covers flashing the Automatous Matter over Thread firmware onto a supported Shelly Gen4 device. There are two ways to flash:

- **[Shelly web UI](#flash-with-the-shelly-web-ui)** — over the network, with no soldering and no UART adapter. The fastest path but it cannot back up the original firmware. Treat it as effectively one-way.
- **USB-UART** — more setup, but it captures a full backup of the stock firmware first, keeping the device fully reversible. About 15 minutes: 5 minutes to wire up, 5–10 minutes to back up, and 1–2 minutes to flash.

If you want the option to return to stock functionality, take a backup over UART before using the web UI.

Once the firmware is up and running, updates are applied through Home Assistant with Matter OTA.

---

## Safety warning

> ⚠️ **Never connect the Shelly to AC mains while flashing via UART.** The programming header is not galvanically isolated from the relay circuitry. Connecting the Shelly to mains voltage AND a USB-UART adapter simultaneously can cause:
> - Personal electrocution risk
> - Permanent destruction of the Shelly
> - Permanent destruction of your computer's USB port (or the entire computer)
>
> Always flash the Shelly powered only by the 3.3V line from the USB-UART adapter, with no other connections to the device. Do not flash a Shelly that is installed in a wall, junction box, or any AC-connected fixture. Remove the Shelly entirely from any AC wiring before flashing.

> 💡 **Bench testing note.** The Shelly can be fully flashed, commissioned, and tested over the USB-UART 3.3V line, but **the relay will not physically click at 3.3V**. The relay coil needs full mains voltage to actuate. Bench testing over 3.3V verifies firmware flash success, Matter commissioning, GPIO behavior, LED states, and that your smart home app can send commands. You will not hear the relay click during bench testing. This is expected, not a bug. Verify everything else works first, then install with confidence and skip the headache of pulling the Shelly back out of the wall if something didn't work.

> ⚠️ **Warranty and responsibility.** Installing this firmware voids your Shelly warranty, and Shelly cannot provide technical support for a device running third-party code. You assume all responsibility for any damage, data loss, or device failure. Flashing also removes the factory keys that enable Shelly Cloud and official OTA updates. Only the UART method can capture the full-chip backup that restores them, so a web UI flash is permanent and a UART flash is reversible only if you keep that backup. **See [Warranty, Factory Keys, and Reversibility](REVERSIBILITY.md) before attempting to flash.**


## Contents

- [Safety warning](#safety-warning)
- [Hardware revision](#hardware-revision)
- [Flash with the Shelly web UI](#flash-with-the-shelly-web-ui)
- [Flash with USB-UART](#flash-with-usb-uart)
- [What you will need](#what-you-will-need)
- [CP2102 to Shelly wiring](#cp2102-to-shelly-wiring)
- [Enter flash mode](#enter-flash-mode)
- [Flash with ESPConnect](#flash-with-espconnect)
- [Restoring stock firmware](#restoring-stock-firmware)
- [Command line flashing with esptool](#command-line-flashing-with-esptool)

---

## Hardware revision

Tested on several Shelly 1 Gen4s and Shelly 1 Mini Gen4s, all hardware revision **v0.1.2** (printed on the PCB). Other revisions should work but have not been verified. If you have a different revision, please [open an issue](../../../issues/new) with your findings.

---

## Flash with the Shelly web UI

The fastest way to install this firmware is the Shelly web UI, over your network, with no UART adapter, wiring, or soldering required. The USB-UART method in the rest of this guide is more involved but it is the only path that captures a full backup of the original firmware.

> ⚠️ **The web UI flash is effectively one-way.** The web UI cannot read the chip in order to make the full-chip backup that restores the device to a fully functional factory state, including the keys that enable Shelly Cloud and official OTA. This path also writes a permanent marker to the chip's eFuse. If you want the option to return to stock, [back up over UART](#2-back-up-the-original-shelly-firmware) first, or accept that the change is permanent. See [Reversibility](REVERSIBILITY.md).

> **The device must be in its normal WiFi mode**, reachable on your WiFi network, and running recent stock firmware (verified on 1.7.0, 1.7.5, and 2.0.0).

1. Download the web UI package for your device and variant, `automatous-io-{hardware}-{variant}-vX.Y.Z-ota.zip`, from the [latest release](../../../releases/latest). This is the `.zip`, not the `.bin`.
2. Open the Shelly's web UI in a browser at its IP address on your network.
3. In the device's firmware settings, choose to install firmware from a file and select the `.zip` you downloaded.
4. Confirm the update. The device flashes the new firmware and reboots into BLE commissioning mode, and the LED will rapidly blink. If it does not reboot automatically, remove and reapply power. 
5. [Factory reset](COMMISSIONING.md#factory-reset) the Shelly by holding the button for 10 seconds, then release. The device will reboot and the LED will rapidly blink again, indicating it is ready to commission. ⚠️ Do not skip this step. The previous firmware leaves data on the device that will cause Matter OTA updates to silently fail. Factory reset wipes this data. If you skip it and commission anyway, you will need to factory reset and recommission later to fix OTA updates.
6. Continue to [Commissioning](COMMISSIONING.md).

---

## Flash with USB-UART

The USB-UART method is more involved. It needs a USB-UART adapter and a few minutes of wiring but it backs up the original firmware first and keeps the device fully reversible. Everything below, from wiring through flashing and restoring, is part of this method.

The wiring, pinout, and photos in this section are the Shelly 1 Gen4's 7-pin programming header. The Shelly 1 Mini Gen4 exposes different programming pads, but the flash mode, backup, and esptool steps are the same once wired.

---

## What you will need
- Shelly 1 Gen4
- CP2102 USB-UART adapter
- 1.27mm 7-pin to 2.54mm Dupont custom cable or adapter board [(see pinout)](#cp2102-to-shelly-wiring)
- Chrome, Edge, or any Chromium-based browser (for Web Serial) or esptool via CLI
- (Optional) A separate serial monitor for viewing boot logs after flashing. See [Verify the firmware is running](#5-verify-the-firmware-is-running).

> Solid core Cat 5e / 6 ethernet wires should fit into the female square holes on the Shelly if you don't want to custom make an adapter or solder to flash, although much harder and finicky to manage. I purchased a small adapter board and soldered both 1.27mm and 2.54mm pins to it and utilized Dupont wires, as pictured below.

---

## CP2102 to Shelly wiring

> Full pinout and hardware overview: [Shelly 1 Gen4 Knowledge Base](https://kb.shelly.cloud/knowledge-base/shelly-1-gen4-anz)

| CP2102 | Shelly 1 Gen4 |
|--------|---------------|
| N/A | Pin 1 (ESP_DBG_UART) |
| RXD | Pin 2 (TXD) |
| TXD | Pin 3 (RXD) |
| 3.3V | Pin 4 (3.3V)|
| N/A | Pin 5 (RESET) |
| N/A | Pin 6 (GPIO0 - BOOT)|
| GND | Pin 7 (GND) |

> The crossover is built into the table above: connect CP2102 RXD to Shelly TXD, and CP2102 TXD to Shelly RXD. If you're wiring this from the CP2102's "TX" and "RX" labels directly to the Shelly's "TX" and "RX" labels without crossing, it will not work.

> **Do not connect the 5V pin.** The Shelly programming header is 3.3V only. Applying 5V will likely permanently damage the ESP32-C6 and brick the device. Many CP2102 boards have both 3.3V and 5V pins; verify you are using the 3.3V pin before connecting.

![Shelly 1 Gen4 with Automatous flashing adapter and CP2102 USB-UART](images/shelly1gen4-adapter-and-cp2102.jpg)

*The Shelly 1 Gen4 with the flashing adapter mounted, alongside a CP2102 USB-UART adapter.*

![Adapter mounted on Shelly programming header](images/shelly1gen4-adapter-mounted.jpg)

*The 1.27mm-to-2.54mm adapter board seated next to the Shelly's 7-pin programming header.*

![Complete flashing setup with all wires connected](images/shelly1gen4-flash-setup-wired.jpg)

*Full flashing setup: Shelly's programming header connected to the CP2102 via Dupont wires, with the GPIO0-to-GND jumper visible (white wire) for entering flash mode.*

---

## Enter flash mode

The Shelly enters bootloader (flash) mode when GPIO0 is held low at power-up.

1. **Disconnect** the 3.3V line if currently connected.
2. **Bridge** Pin 6 (GPIO0) to Pin 7 (GND) and keep them connected.
3. **Connect** the 3.3V line to power the device. The Shelly boots into flash mode.
4. You can now leave the GPIO0–GND bridge in place for the duration of flashing, or remove it after a second or two. Either works once the bootloader is running.

---

## Flash with ESPConnect

ESPConnect is a browser-based ESP32 flashing tool built on Web Serial. No installation required. It works in Chrome, Edge, or any Chromium-based browser on macOS, Windows, and Linux.

> 💡 **Prefer the command line?** You can do the entire backup, verify, and flash with esptool instead a hybrid of both tools. Skip to [Command line flashing with esptool](#command-line-flashing-with-esptool).

### 1. Connect to the Shelly

1. Put the Shelly into flash mode (Pin 6 GPIO0 bridged to Pin 7 GND, then power up via 3.3V).
2. Open [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/) in your browser.
3. Click **Connect** and select your USB-UART serial port from the browser dialog.
4. Set baud rate to **115200**. Slower than the default, but more reliable for the long backup read.
5. ESPConnect will display chip info confirming the ESP32-C6 is detected, including the device's **MAC address**. Note this down, you will use it to label your backup file. If you see "Failed to connect" or no chip info, the Shelly is not in flash mode. Recheck the GPIO0–GND bridge and re-power the device.

### 2. Back up the original Shelly firmware

> This step is essential. The backup is the only way to restore your device to its fully functional factory state. **I strongly suggest you do not skip it**.

> The full-chip image includes the device's factory-provisioned keys, which enable Shelly Cloud and official OTA updates. This is why a complete backup is what makes restoration possible: a partial backup cannot bring those keys back, and without them the change is one-way.

1. In the left navigation, click **Flash Tools**.
2. Click **Download Flash Backup**.
3. ESPConnect will read the entire flash and download it as a `.bin` file. This takes 5–10 minutes at 115200 baud. Do not disconnect or close the browser during the read.
4. Rename the downloaded file to include the device's MAC address, e.g. `shelly-1-gen4-stock-AABBCCDDEEFF.bin`.
5. **Verify the download against the chip.** ESPConnect can save the backup but has no built-in way to check that downloaded file against the chip (its integrity tool hashes the chip, not your saved file), so this step uses the command line. It is worth the detour. The backup is your only way back to stock, and verifying confirms the file is complete and matches the chip instead of assuming it. With the Shelly still in flash mode (disconnect ESPConnect first to free the serial port), run esptool's `verify_flash` against your saved file. A `verify OK (digest matched)` result means the backup is safe to keep; a `verify FAILED`, or fewer bytes checked than the chip's flash size, means redo it. The command, esptool setup, and how to read the output are in [Command line flashing with esptool](#command-line-flashing-with-esptool).
6. Store the backup somewhere safe. If you are flashing multiple Shellies, keep each MAC-labeled backup separate. They should not be interchanged.

### 3. Flash the Automatous firmware

1. In ESPConnect's **Flash Tools**, click **Flash Firmware** (reconnect ESPConnect first if you left it to verify the backup; the Shelly is still in flash mode).
2. Select the latest release `.bin` from your downloads.
3. Set flash offset to `0x0`.
4. Check **Erase entire flash before writing**.
5. Click **Flash** and wait for the operation to complete (1–2 minutes). ESPConnect shows a progress bar and a success message when done.

> **Re-flashing erases commissioning.** Flashing the merged binary with "Erase entire flash before writing" wipes the whole flash, including stored Matter commissioning. A first flash over stock firmware has nothing to lose, but a device that was already commissioned, or one you are switching to a different variant, will need to be re-added to your ecosystems afterward. To update without re-pairing, the device takes [updates](UPDATING.md) through Home Assistant with Matter OTA, or you can use the ESP-IDF CLI flash path in [Building from Source](BUILDING.md#flash-your-build).

### 4. Boot the new firmware

1. Disconnect from ESPConnect (click **Disconnect** or close the tab).
2. **Remove the GPIO0 ↔ GND bridge.** This is critical. If GPIO0 is still bridged to GND at the next power-up, the Shelly will boot back into flash mode instead of running your firmware.
3. Power-cycle the Shelly: disconnect the 3.3V line, wait two seconds, reconnect.
4. The Shelly is now running the Automatous firmware and is in BLE commissioning mode, ready to be added to your smart home ecosystem.

### 5. Verify the firmware is running

After power-cycling, the Shelly is running your firmware and advertising for Matter commissioning over BLE. You can verify it's working in three ways:

**Easiest: look at the LED.** If you see a rapid blink (~200ms on, 200ms off), the firmware is running and advertising for Matter commissioning. You can proceed directly to [Commissioning](COMMISSIONING.md). See the [Status LED reference](COMMISSIONING.md#status-led-reference) for what each pattern means.

**Try to add it from your smart home app.** Open Apple Home, Google Home, Alexa, or Home Assistant and start the "add device" flow. If the Shelly appears as a discoverable Matter device, the firmware is running correctly.

**For developers / troubleshooting:** ESPConnect requires flash mode and cannot show normal application serial output easily from my experience with this project. Use a separate serial monitor instead:

- **macOS / Linux CLI:** `screen /dev/cu.usbserial-XXXX 115200` or `idf.py -p /dev/cu.usbserial-XXXX monitor` if you have ESP-IDF installed (replace with your actual port)
- **Windows:** [PuTTY](https://putty.org/index.html) configured for Serial at 115200 baud
- **VS Code:** the Serial Monitor extension

You should see boot logs followed by `Commissioning Window Opened`. This confirms Matter is initialized and the device is advertising.

> ⚠️ **Important:** With your firmware running, do not bridge GPIO0 to GND again unless you want to re-enter flash mode. The USB-UART 3.3V line can stay connected for power and serial monitoring during commissioning. Just leave Pin 6 (GPIO0) disconnected.

---

## Restoring stock firmware

> ✅ **Stock restore is verified working, including cloud and OTA.** A full-chip restore returns the device to a fully functional factory state with Shelly Cloud connectivity and official OTA updates intact. Your Shelly will create its `shelly-XXXXXX` setup AP, pair with the Shelly app, and behave identically to a factory unit. This recovery depends on having the full-chip backup. For the test record and the reasoning, see [Warranty, Factory Keys, and Reversibility](REVERSIBILITY.md#uart-path-verified-recoverable).

If you want to revert to the original Shelly firmware:

1. Put the Shelly back into flash mode (GPIO0 ↔ GND, power up via 3.3V).
2. In ESPConnect, click **Flash Firmware**.
3. Select the `.bin` backup file you saved earlier (matching this device's MAC address).
4. Set flash offset to `0x0`, check **Erase entire flash before writing**, click **Flash**.
5. Power-cycle the Shelly. It will boot the restored firmware and behave as factory.

---

## Command line flashing with esptool

Requires [esptool](https://github.com/espressif/esptool) installed. On macOS/Linux: `pip install esptool`. On Windows: install via pip, or download the standalone binary from the releases page.

Throughout these commands, replace:
- `<PORT>` with your serial port (`/dev/cu.usbserial-XXXX` on macOS, `/dev/ttyUSB0` on Linux, `COM3` on Windows)
- `<MAC>` with the Shelly's MAC address (read from `chip_id` below)
- `<VERSION>` with the firmware release version (in the form `vX.Y.Z`)

Put the Shelly into flash mode before running each command (GPIO0 bridged to GND, then power up via 3.3V).

**Identify the chip and read the MAC address:**

Device information that includes Mac Address:
```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 chip_id
```

Backup your original firmware. `ALL` reads from offset `0x0` to the end of flash, so it captures the whole chip whatever its capacity, with no need to know or hardcode the size:

```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 \
  read_flash 0x0 ALL shelly-1-gen4-stock-<MAC>.bin
```

Verify the backup against the chip:

```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 \
  verify_flash 0x0 shelly-1-gen4-stock-<MAC>.bin
```

esptool's output gives you both checks at once. The connection banner reports the chip's flash size (`Features: ... Embedded Flash 8MB`), and the verify line reports how many bytes it checked against your file (`Verifying 0x800000 (8388608) bytes ...`). A complete intact backup shows the verified byte count matching the chip's flash size and ends in `-- verify OK (digest matched)`. A smaller byte count than the chip's flash means the file is truncated; a `verify FAILED` means it is corrupt. Redo the backup in either case. This works on any backup of this device, including one made in ESPConnect.

Erase flash:
```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 erase_flash
```

**Flash the Automatous firmware:**

```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 \
  write_flash 0x0 automatous-io-shelly-1-gen4-light-<VERSION>.bin
```

**Restore the original firmware:**

Erase the chip, then write your saved backup:

```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 erase_flash
```

```bash
esptool.py --chip esp32c6 --port <PORT> --baud 115200 \
  write_flash 0x0 shelly-1-gen4-stock-<MAC>.bin
```

After flashing, remove the GPIO0–GND bridge and power-cycle the Shelly to boot the new firmware.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
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