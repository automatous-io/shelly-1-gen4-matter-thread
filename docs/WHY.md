# Why Matter over Thread

**[README](../README.md)** > **Why Matter over Thread** · [Report an issue](../../../issues/new)

This document explains why this firmware exists, how it compares to other Shelly firmware options, and why it runs Matter over Thread rather than WiFi.

---

## Contents

- [Repurposing the Zigbee radio](#repurposing-the-zigbee-radio)
- [Why Matter, and not ESPHome or Tasmota](#why-matter-and-not-esphome-or-tasmota)
- [Why Thread instead of WiFi](#why-thread-instead-of-wifi)
- [Waiting for official Shelly support](#waiting-for-official-shelly-support)

---

## Repurposing the Zigbee radio

Shelly Gen4 devices ship with an ESP-Shelly-C68F (ESP32-C6) whose 802.15.4 radio can run either Thread or Zigbee. Stock firmware uses that radio for Zigbee. Stock firmware also supports Matter, but over WiFi, so the Thread capability of the radio goes unused. Shelly has stated they have no plans to add Thread support.

This firmware reconfigures the 802.15.4 radio to run Thread through OpenThread, which unlocks Matter over Thread on hardware you already own. Nothing about the hardware changes. The radio was always capable of Thread, and this firmware simply uses it that way.

---

## Why Matter, and not ESPHome or Tasmota

ESPHome and Tasmota are excellent, mature projects. They shine when you have a central controller, Home Assistant for ESPHome or an MQTT broker for Tasmota. If you already run that infrastructure, they are great choices.

This firmware fills a different niche. It speaks Matter directly, so it needs no controller, no broker, and no bridge. There is no Home Assistant instance, no MQTT broker, no ESPHome dashboard, and no YAML to maintain. If you do not already run that infrastructure, native Matter is the simpler path.

That direct-to-Matter design means the same firmware works across ecosystems.

- Apple Home user without Home Assistant. It just works. Commission directly from the Home app.
- Google Home or Alexa user. The same. Direct Matter commissioning, no third-party hub.
- Home Assistant user. Also works, commissioned through the Matter integration.
- Multi-ecosystem household. Multi-fabric commissioning lets the same device live in Apple Home, Home Assistant, and Google Home at the same time.

Matter is an industry-standard protocol, so your devices work with the ecosystems you have today and stay compatible with new Matter ecosystems as they appear.

---

## Why Thread instead of WiFi

Matter runs over either Thread or WiFi. This firmware chooses Thread for several reasons.

- Mesh networking. Thread devices form a self-healing mesh that does not depend on your WiFi. Devices keep working even if your WiFi is down or congested.
- Lower latency for local commands like toggling a light, compared to WiFi-based Matter.
- Reduced WiFi burden. Every Matter over Thread device is one fewer device on your WiFi.
- Power consumption. Thread can run with much lower power than WiFi, but only as a Sleepy End Device. This firmware runs as a Thread Router, which trades sleep-mode savings for mesh extension. See [Power Consumption](POWER.md) for measured numbers and the rationale.

This project began because reliable smart home devices were needed in a Sprinter van where WiFi is not always available. Battery and solar capacity can be limiting in van life, which is why a Sleepy End Device variant may join the roadmap later. The current variants prioritize Thread mesh extension, which serves a broader audience. If you are an offgrid, smart home, or Thread-curious tinkerer, you might find this useful too.

---

## Waiting for official Shelly support

If you would rather not flash third-party firmware or void your warranty, another option is to wait for official support and ask Shelly to add it. Shelly currently has no plans for Thread, but they track it as a feature request and have said that enough requests could move it onto their roadmap. You can submit a request through Shelly's feature proposal form: [Request Thread support from Shelly](https://support.shelly.cloud/en/support/tickets/new?ticket_form=devices_and_features_proposal_archive).

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Stock Flash Partitions](PARTITIONS.md) — stock flash layout and the factory `shelly` partition
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
