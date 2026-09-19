# GPIO Map

**[README](../README.md)** > **GPIO Map** · [Report an issue](../../../issues/new)

The Gen4 devices are built on the ESP-Shelly-C68F, a Shelly module that carries an Espressif ESP32-C6 with 8MB of flash. The module is common across the Gen4 line, but the pins Shelly wires to the relay, button, switch input, status LED, etc. differ by device.

This page records those pin assignments so that anyone porting this or any other firmware do not have to flash and observe to find them or trust existing incorrect documentation available online.

Pins listed as confirmed were verified on real hardware. Pins listed as `GPIOnn` are unverified at this time.

## Contents

- [Shelly 1 Gen4](#shelly-1-gen4)
- [Shelly 1PM Gen4](#shelly-1pm-gen4)
- [Shelly 1 Mini Gen4](#shelly-1-mini-gen4)
- [Shelly 1PM Mini Gen4](#shelly-1pm-mini-gen4)
- [Shelly 2PM Gen4](#shelly-2pm-gen4)

## Shelly 1 Gen4

Hardware Revision: v0.1.2

Verified: May 2026

| Function | GPIO |
|---|---|
| Relay | GPIO5 |
| Switch input| GPIO10 |
| Button (onboard) | GPIO4 |
| Status LED | GPIO15 |

## Shelly 1PM Gen4

Hardware Revision: v0.1.2

Verified: June 2026

| Function | GPIO |
|---|---|
| Relay | GPIO4 |
| Switch input | GPIO10 |
| Button (onboard) | GPIO1 |
| Status LED | GPIO11 |
| Power meter (BL0942) | TX GPIO6, RX GPIO7, UART1 |
| NTC | GPIO3 |

## Shelly 1 Mini Gen4

Hardware Revision: v0.1.2

Verified: July 2026

| Function | GPIO |
|---|---|
| Relay | GPIO10 |
| Switch input | GPIO12 |
| Button (onboard) | GPIO22 |
| Status LED | GPIO5 |
| NTC | GPIO4 |

## Shelly 1PM Mini Gen4

Hardware Revision: v0.1.1

Verified: September 2026

| Function | GPIO |
|---|---|
| Relay | GPIO10 |
| Switch input | GPIO12 |
| Button (onboard) | GPIO22 |
| Status LED | GPIO5 |
| Power meter (BL0942) | TX GPIO20, RX GPIO19, UART1 |
| NTC | GPIO4 |

## Shelly 2PM Gen4

Hardware Revision: v0.1.2

Verified: September 2026

| Function | GPIO |
|---|---|
| Relay 1 (O1) | GPIO5 |
| Relay 2 (O2) | GPIO3 |
| Switch input 1 (S1) | GPIO11 |
| Switch input 2 (S2) | GPIO10 |
| Button (onboard) | GPIO12 |
| Status LED | GPIO18 |
| Power meter (ADE7953) | SDA GPIO6, SCL GPIO7, IRQ GPIO1, I2C |
| NTC | GPIO4 |

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
