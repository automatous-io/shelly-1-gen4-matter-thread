# Shelly Gen4 — Matter over Thread

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Release](https://img.shields.io/github/v/release/automatous-io/shelly-1-gen4-matter-thread)](../../releases/latest)
![Last Commit](https://img.shields.io/github/last-commit/automatous-io/shelly-1-gen4-matter-thread)
[![Downloads](https://img.shields.io/github/downloads/automatous-io/shelly-1-gen4-matter-thread/total)](../../releases)
[![Stars](https://img.shields.io/github/stars/automatous-io/shelly-1-gen4-matter-thread?style=social)](../../stargazers)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy_Me_a_Coffee-support-FFDD00?logo=buymeacoffee&logoColor=black)](https://buymeacoffee.com/automatous.io)

> **⚠️ Disclaimer.** Installing this firmware voids your Shelly warranty, and Shelly cannot provide technical support for a device running third-party code. It removes the factory keys that enable Shelly Cloud and official OTA updates. Treat flashing as one-way unless you keep the full-chip backup you make before flashing. Incorrect flashing can brick your device, so always back up your original firmware before proceeding if reversibility is important to you. You assume all responsibility for any damage, data loss, or device failure. This project is not affiliated with Shelly, Allterco Robotics, CSA, or Espressif Systems. See [Warranty, Factory Keys, and Reversibility](docs/REVERSIBILITY.md).

The first third-party open source Matter over Thread firmware for Shelly Gen4 devices. It works natively with Apple Home, Google Home, Alexa, and Home Assistant. No Shelly app, no cloud, no WiFi. The Gen4 line's ESP-Shelly-C68F (ESP32-C6) has an 802.15.4 radio that stock firmware uses for Zigbee. Stock firmware also supports Matter over WiFi. This firmware reconfigures the 802.15.4 radio to run Thread, which unlocks Matter over Thread.

<p align="center">
  <img src="docs/images/apple-home-device-info-v1.0.0.png" alt="Shelly 1 Gen4 running this firmware in Apple Home, showing manufacturer, model, and firmware version" width="300">
</p>

*Shelly 1 Gen4 running the light variant firmware, commissioned to Apple Home. Manufacturer: AUTOMATOUS.IO, Model: Shelly 1 Gen4 MoT Light.*

---

## Contents

- [Supported devices](#supported-devices)
- [Variants](#variants)
- [Quick start](#quick-start)
- [Features](#features)
- [Compatibility](#compatibility)
- [Documentation](#documentation)
- [Repository layout](#repository-layout)
- [Why?](#why)
- [Waiting for official Shelly support](#waiting-for-official-shelly-support)
- [About](#about)
- [In the wild](#in-the-wild)
- [Other projects from Automatous](#other-projects-from-automatous)
- [License](#license)

---

## Supported devices

Each supported device has its own firmware builds under [`source/`](source/).

| Device | Variants | Status |
|---|---|---|
| Shelly 1 Gen4 | Light, Outlet, Opener, Light Switch | Released |
| Shelly 1 Mini Gen4 | Outlet | Released |

More Gen4 devices are planned. See the [Roadmap](docs/ROADMAP.md).

---

## Variants

Matter device types are declared by the firmware at flash time and cannot be changed in the smart home app after commissioning, so each behavior is a separate firmware build.

| Variant | Matter device type | Relay behavior | Good for |
|---|---|---|---|
| Light | On/Off Light *(shown as only a light)* | Latching. Holds on or off until changed. | Lights and lighting circuits. |
| Outlet | On/Off Plug-in Unit *(shown as an outlet, switch, fan, light)* | Latching. Holds on or off until changed. | Outlets, plug-in appliances, fans, heaters, pumps, lights, and other set-and-hold loads. |
| Opener | On/Off Plug-in Unit + Contact Sensor | Momentary pulse, roughly 500ms. | Garage door openers, gates, doorbells, and other pulse-activated devices. |
| Light Switch | On/Off Light + On/Off Light Switch | Latching, but the SW terminal is detached from the relay. | Repurposing the wall switch to control other Matter devices through a binding, while the local relay runs independently. |

All four variants are available for the Shelly 1 Gen4. The Shelly 1 Mini Gen4 ships the Outlet variant. Every variant on both devices includes a temperature sensor endpoint.

Light and Outlet are electrically identical. Both latch and hold state, and both keep the SW terminal as a physical wall toggle. They differ only in the Matter device type they report, which sets how your smart home app names, displays, and voice-controls the device, as a light or as an outlet. Some apps let you recategorize after pairing. Others fix the icon, label, and automations to the reported type, which cannot change after commissioning. Pick the variant that matches the load you wired, so the controls read the way you expect.

The Light variant is the flagship release and the one pictured above. Power-on behavior on the latching variants (Light and Outlet) defaults to off and is configurable through the Matter StartUpOnOff attribute (off, on, toggle, or restore last state). Home Assistant users can write this attribute directly. Apple Home and Google Home have limited Matter attribute editing today, so the firmware default applies there until those apps improve.

The Light Switch variant, contributed by [Tomas McGuinness](https://github.com/tomasmcguinness), decouples the SW terminal from the local relay. Flipping the wall switch no longer toggles the wired load. Instead it sends a Matter Toggle command through a second On/Off Light Switch endpoint, which you bind to other Matter devices such as bulbs, groups, or scenes. The local relay stays on the On/Off Light endpoint and is controlled by your smart home app, by voice, and by the onboard button. Out of the box the wall switch does nothing until you configure a binding. To keep local control as well, bind the switch endpoint back to the device's own light endpoint. You can write the binding with chip-tool. Home Assistant users can use the [Matter Binding Helper](https://github.com/cedricziel/ha-matter-binding-helper) by Cedric Ziel, which makes it straightforward.

> ⚠️ This firmware uses ESP-Matter SDK test credentials and is not VID/PID certified. It is functional for personal use and not suitable for resale as a certified Matter product. See [Certification](docs/CERTIFICATION.md).

---

## Quick start

Flash via the Shelly web UI over your WiFi network, no UART adapter required. This is the fastest way to flash but it cannot create a backup of the stock firmware and should be considered one-way. See [Reversibility](docs/REVERSIBILITY.md).

1. [Download the latest release](../../releases/latest) and grab the web UI package for your [device](#supported-devices) and preferred [variant](#variants).
2. [Install it through the Shelly web UI](docs/FLASHING.md#flash-with-the-shelly-web-ui).
3. [Commission](docs/COMMISSIONING.md) with your smart home app.

Once it is up and running, [updates are available](docs/UPDATING.md) through Home Assistant with Matter OTA.

---

## Features

- Shelly web UI install, with no UART adapter required (unless you want [reversibility](docs/REVERSIBILITY.md)).
- Updates through Home Assistant with Matter OTA.
- Physical button toggle on the onboard relay button.
- External wall switch input on the SW terminal.
- Configurable power-on behavior through the Matter StartUpOnOff attribute.
- Status LED indication for BLE advertising, Thread connecting, and Thread connected.
- Identify support. Blink the LED on command from any Matter controller to locate the device.
- Factory reset via long press of the onboard relay button.
- Full Thread Device mode. Adds a Thread Router to your mesh for other devices.
- Multi-fabric. Commission to multiple ecosystems at once, with state synced across all of them.
- Temperature sensor endpoint reporting the device's internal temperature.

---

## Compatibility

Matter over Thread devices need a path onto a Thread network to commission and reach the Shelly. That can be a Thread Border Router, a device with a built-in Thread radio such as an iPhone 15 Pro or newer, or both. The table below lists tested and supported options.

| Device | Provides Thread | Tested |
|---|---|---|
| Apple HomePod mini | Yes | Yes |
| Apple HomePod (2nd gen) | Yes | Yes |
| Apple TV 4K (2nd gen A2169) | Yes | Yes |
| Apple TV 4K (3rd gen) | Yes | Untested |
| iPhone 15 Pro and newer | Yes | Yes |
| Google Nest Hub (2nd gen) | Yes | Untested |
| Home Assistant (SkyConnect or Yellow) | Yes | Yes |
| Amazon Echo (4th gen) | Yes | Untested |

---

## Documentation

Most of it is in [`docs/`](docs/), with build [`scripts/`](scripts/), the changelog, and the security policy at the repo root. A typical path is to read the [Flashing Guide](docs/FLASHING.md), and its safety warning, flash the firmware, then [commission](docs/COMMISSIONING.md) the device to your smart home.

- [Why Matter over Thread](docs/WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](docs/FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](docs/REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Commissioning](docs/COMMISSIONING.md) — pairing the device and reading the status LED
- [Updating](docs/UPDATING.md) — keeping a device current after flashing
- [Power Consumption](docs/POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](docs/BUILDING.md) — compiling the firmware yourself
- [GPIO Map](docs/GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Certification](docs/CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](docs/ROADMAP.md) — current known limitations and planned work
- [Contributing](docs/CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](docs/CONTRIBUTORS.md) — people who have helped move the project forward
- [Security policy](SECURITY.md) — reporting a vulnerability and how issues are triaged
- [Changelog](CHANGELOG.md) — release history by device and variant

---

## Repository layout

```
shelly-1-gen4-matter-thread/
├── README.md          This file.
├── LICENSE            Apache 2.0.
├── docs/              Documentation and the images it references.
├── scripts/           Build scripts that package the release artifacts.
└── source/
    ├── shelly-1-gen4/
    │   ├── light/         Matter On/Off Light, latching relay. Released.
    │   ├── opener/        Matter On/Off Plug-in Unit + Contact Sensor, momentary pulse. Released.
    │   ├── outlet/        Matter On/Off Plug-in Unit, latching relay, SW kept as a wall toggle. Released.
    │   └── light-switch/  Matter On/Off Light Switch, detached relay with the SW input bound to other Matter devices. Released.
    └── shelly-1-mini-gen4/
        └── outlet/        Matter On/Off Plug-in Unit, latching relay. Released.
```

Each variant directory under `source/` is a self-contained ESP-IDF project. See [Building from Source](docs/BUILDING.md) for what each variant does and how to build it.

---

## Why?

The Shelly Gen4 line's ESP-Shelly-C68F (ESP32-C6) can run Thread, but stock firmware never uses it that way. This firmware repurposes the radio for Thread so the device speaks Matter directly to your smart home, with no WiFi, no cloud, and no bridge or controller required in between. For the full rationale, the comparison to ESPHome and Tasmota, and why Thread over WiFi, see [Why Matter over Thread](docs/WHY.md).

---

## Waiting for official Shelly support

If you would rather not flash third-party firmware or void your warranty, another option is to wait for official support and ask Shelly to add it. Shelly currently has no plans for Thread, but they track it as a feature request and have said that enough requests could move it onto their roadmap. You can submit a request through Shelly's feature proposal form: [Request Thread support from Shelly](https://support.shelly.cloud/en/support/tickets/new?ticket_form=devices_and_features_proposal_archive).

---

## About

This firmware was created from inside an old Dodge Sprinter T1N named Mabel, parked somewhere in the USA.

I put in many sleepless nights obsessing over Thread support, which kickstarted me into writing custom firmware and testing with ESP-IDF, ESP-Matter, and ESPHome examples. That meant figuring out Shelly's custom partition offsets, the GPIO quirks specific to this device, and getting Matter over Thread commissioning working on non-devkit hardware.

If it saved you the same headache, consider leaving a ⭐ on the repo. It helps the project show up in GitHub search and signals to other Shelly Gen4 owners that this firmware exists and works. If you would like to support the work directly, you can [buy me a coffee](https://buymeacoffee.com/automatous.io).

---

## In the wild

Press, blog posts, and mentions of the project.

- [Shelly relay switches finally run on Thread with open-source firmware](https://www.matteralpha.com/news/shelly-relay-switches-finally-run-on-thread-with-open-source-firmware) — Matter Alpha, May 2026
- [Shelly Gen 4: Custom-Firmware bringt Unterstützung für Matter over Thread](https://stadt-bremerhaven.de/shelly-gen-4-custom-firmware-bringt-unterstuetzung-fuer-matter-over-thread/) — Caschys Blog, May 2026 (German)
- [Community discussion on r/homeassistant](https://www.reddit.com/r/homeassistant/comments/1t8bv8q/made_an_open_source_matter_over_thread_firmware/) — Reddit, May 2026
- [Jonathan Hui of the Thread Group / CSA highlights the project](https://www.linkedin.com/posts/jonathanhui_from-the-homeassistant-community-on-reddit-share-7459779233565925377-KUMe/) — LinkedIn, May 2026
- [Tomas McGuinness discusses Matter Bindings via the light-switch firmware](https://youtu.be/4279At06XYE) — YouTube, July 2026
- [Community discussion on r/ShellyUSA, mod credits project with generating a dramatic increase of Thread interest and feature requests to Shelly](https://www.reddit.com/r/ShellyUSA/s/HWfyJbMkMY) — Reddit, July 2026

---

## Other projects from Automatous

Other open source, local-first, Matter over Thread projects from Automatous.

| Project | What it is |
|---|---|
| [T1N Smart Lock](https://github.com/automatous-io/t1n-smart-lock) | Open source Matter over Thread smart lock that integrates with the factory central locking on a 2005 Dodge Sprinter 2500 (T1N chassis). Observation based, OEM respectful. |

---

## License

Apache 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

Based on the [ESP-Matter](https://github.com/espressif/esp-matter) light example by Espressif Systems, licensed under Apache 2.0. The Opener and Outlet variants' Matter device composition was informed by the esp-matter `on_off_plugin_unit` example, and the Light Switch variant's by the esp-matter `light_switch` example.
