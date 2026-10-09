# Power Consumption

**[README](../README.md)** > **Power Consumption** · [Report an issue](../../../issues/new)

A common assumption is that Matter over Thread devices use significantly less power than Matter over WiFi. That is true for Thread devices running as Sleepy End Devices, but not for always-on devices like the current variants of this firmware. This document explains the measured numbers and the design choice behind them.

---

## Contents

- [Measured consumption](#measured-consumption)
- [Why no power savings on this variant](#why-no-power-savings-on-this-variant)
- [When Thread saves real power](#when-thread-saves-real-power)

---

## Measured consumption

Measured on a Shelly 1 Gen4 at 12V DC input, the Shelly itself, with no load on the relay. The Shelly 1 Mini Gen4 has not been measured yet. The bench uses 12V DC because this firmware was developed and tested in an offgrid van electrical system. The Shelly 1 Gen4 also runs on AC mains, where the milliamp figures are much lower for the same power, so compare the wattage, not the current, when reasoning about an AC install.

| Firmware | Relay off | Relay on |
|---|---|---|
| Automatous (v1.2.1, Matter over Thread) | 26 mA / 0.3W | 43 mA / 0.5W |
| Stock Shelly 1 Gen4 (Matter over WiFi) | 29 mA / 0.3W | 44 mA / 0.5W |

The numbers are nearly identical because both radios stay continuously awake. The Shelly's connected load, such as a light, fan, or appliance, adds to the relay-on numbers in a real installation.

---

## Why no power savings on this variant

This firmware runs the Shelly as a Thread Router in Full Thread Device mode, so it actively participates in routing packets for other Thread devices on your mesh. Router mode requires the radio to stay awake to relay traffic, so the device does not get the sleep-mode power savings that Thread is known for.

This design choice prioritizes mesh extension. Each Shelly running this firmware adds a Thread Router to your network, which improves coverage, reliability, latency, and network capacity for other Thread devices nearby.

---

## When Thread saves real power

Thread power savings come from devices running as Sleepy End Devices, where the radio sleeps between brief wake-ups. Battery powered sensors, locks, and buttons typically run as Sleepy End Devices and can last years on a coin cell.

For an AC powered device like the Shelly installed at home, the savings of Sleepy End Device mode are less compelling. You have unlimited power available, and you would lose the mesh extension benefit.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Stock Flash Partitions](PARTITIONS.md) — stock flash layout and the factory `shelly` partition
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
