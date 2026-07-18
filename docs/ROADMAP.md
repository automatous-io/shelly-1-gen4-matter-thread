# Roadmap

**[README](../README.md)** > **Roadmap** · [Report an issue](../../../issues/new)

This document tracks the current known limitations and the planned work. To prioritize something, vote on the [roadmap discussion](../../../discussions) or open an issue with your use case.

---

## Contents

- [Known limitations](#known-limitations)
- [Planned](#planned)

---

## Known limitations

- Matter OTA updates are supported and documented in Home Assistant only. Because the firmware uses test credentials, the assumption is that no other major Matter ecosystem will serve OTA updates to it. Commissioning-preserving updates run through Home Assistant's Matter Server. See [Updating](UPDATING.md).
- Thermal protection is validated on the Shelly 1 Mini Gen4. All firmware variants read the silicon die temperature and cut the relay above 75°C sustained for 10 seconds. The trip, the latch that blocks turn-on while hot, and the automatic recovery were tested on Shelly 1 Mini Gen4 hardware, and the trip logic is shared across all devices and variants.

---

## Planned

- Shelly 1PM Gen4 support.
- Shelly 2PM Gen4 support.
- Shelly Flood Gen4 support.
- NTC on devices that carry one instead of reporting internal die temperature.
- A Sleepy End Device variant for battery and solar constrained installs, trading mesh extension for lower power.
- Build ergonomics. Migrate the build to consume esp-matter through the component library instead of a separately installed SDK, and fold the C6 and Thread settings into the base sdkconfig.defaults, so a plain `idf.py build` works without extra setup.
- Shared components directory across all variant builds.

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
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
