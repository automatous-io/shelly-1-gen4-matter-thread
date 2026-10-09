# Matter and Thread Certification

**[README](../README.md)** > **Certification** · [Report an issue](../../../issues/new)

This document explains the Matter and Thread certification status of this firmware, what "uncertified Matter device" means in practice, and the trademarks referenced in this project.

---

## Contents

- [Certification status](#certification-status)
- [Why not certified](#why-not-certified)
- [What this means for users](#what-this-means-for-users)
- [Test credentials](#test-credentials)
- [Trademarks](#trademarks)

---

## Certification status

This firmware is **not Matter Certified** and **not Thread Certified**.

It implements the Matter protocol via Espressif's [esp-matter](https://github.com/espressif/esp-matter) SDK and Thread via [OpenThread](https://openthread.io/), but has not been submitted to the Connectivity Standards Alliance (CSA) or Thread Group for certification.

---

## Why not certified

Matter and Thread certification are designed for commercial products manufactured and distributed at scale. The process requires paid membership in the certifying organization, per-product certification fees, and testing through an authorized lab. Together these run into thousands of dollars per year plus a per-device cost.

That cost structure is built for companies shipping volume products, not for open source firmware that individuals flash onto hardware they already own. Certifying is out of scope for this project.

---

## What this means for users

Devices flashed with this firmware appear in Matter ecosystems as **uncertified Matter devices**. Most ecosystems show a one-time warning during commissioning:

- **Apple Home:** "Uncertified Accessory" prompt with "Add Anyway"
- **Google Home:** "This device hasn't been certified" with an "Add anyway" option
- **Amazon Alexa:** Similar uncertified device prompt
- **Home Assistant:** May or may not prompt depending on version

This is normal and expected. Tapping "Add Anyway" or the equivalent proceeds with commissioning. Once commissioned, the device functions identically to a certified Matter device within your fabric.

---

## Test credentials

The firmware uses the publicly available ESP-Matter test VID/PIDs. These are standard development credentials published for non-commercial use during firmware development.

Because every unit built from this firmware uses the same test credentials, they all share the same commissioning setup code and QR. That is fine for personal use, but it means the setup code is not a secret, and if you flash more than one device, commission them one at a time to avoid ambiguity during pairing. See [Before commissioning](COMMISSIONING.md#before-commissioning).

Once a device is commissioned to your Matter fabric, the setup code is no longer used for authentication. Your Matter ecosystem manages credentials going forward.

Test credentials are appropriate for personal use, development, and open source distribution. They are not appropriate for a commercial product, which would require a real VID/PID from the CSA and certification.

---

## Trademarks

- **Matter** is a trademark of the Connectivity Standards Alliance.
- **Thread** is a trademark of the Thread Group.
- **Shelly** is a trademark of Allterco Robotics.

This project is not affiliated with, endorsed by, or sponsored by any of these organizations.

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
- [Stock Flash Partitions](PARTITIONS.md) — stock flash layout and the factory `shelly` partition
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
