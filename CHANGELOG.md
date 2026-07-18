# Changelog

All notable changes to this firmware are recorded here. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project follows [semantic versioning](https://semver.org/spec/v2.0.0.html).

Each firmware variant versions independently, and releases are grouped by device and variant below. Switching a device between variants always requires full reset and re-commission, because each variant is a different Matter device type.

## Shelly 1 Gen4, all variants

### [2.1.0] - 2026-07-18

Temperature sensor endpoint across all four variants, and a thermal task stack fix.

#### Added
- Temperature sensor endpoint reporting the ESP32-C6 die temperature, updated on changes of 0.5°C or more. The reading is null until the first poll after the Matter stack starts.

#### Fixed
- Thermal monitor task stack overflow. Publishing readings through `attribute::update()` runs Matter reporting on the thermal task's stack, and the previous 2048-byte stack overflowed. The stack is now 4096 bytes, which also covers the thermal-fault OnOff update that has always shared this stack.

### [2.0.0] - 2026-06-25

Web UI install, Matter OTA updates, and a stock-aligned partition layout, the same change across all four variants. Within a variant's v1.x line the NVS schema is stable, and an in-place upgrade through the ESP-IDF CLI preserves commissioning. Moving from any v1.x build to 2.0.0 is the exception: the partition layout changed and requires a one-time full reflash that erases commissioning, after which Matter OTA preserves it for future updates.

#### Added
- Web UI install. A packaged `automatous-io-shelly-1-gen4-{variant}-vX.Y.Z-ota.zip` installs through the Shelly web UI, with no UART adapter required. Built by `scripts/make-webui-ota-zip.py`.
- Matter OTA updates. A `.ota` image updates a commissioned device over Thread through Home Assistant's Matter Server, preserving commissioning, Thread credentials, and ecosystem pairings. Built by `scripts/make-matter-ota.py`.

#### Changed
- Partition layout now follows the stock Shelly 1 Gen4 offsets, with A/B app slots so one `partitions.csv` serves both UART flashing and the Web UI OTA. Moving to it from any v1.x build is a one-time full reflash over UART that erases commissioning; updates from 2.0.0 onward preserve it. Drops the `fctry` and `phy_init` partitions, since the DAC is compiled in and PHY data lives in the app.
- Distinct Matter Product ID per variant (Light `0x8001`, Light Switch `0x8002`, Opener `0x8003`, Outlet `0x8004`); Matter OTA images can only target the variant it was built for.

## Shelly 1 Gen4 Light

### [1.2.1] - 2026-05-23

Module refactor, safety contract, and earlier init. No user-visible behavior change.

#### Added
- Documented relay safety contract. All relay control flows through a single `relay_set()` function with an auditable invariant: pre-init safe-low, single-point thermal lockout, Thread-loss hold, and panic recovery.

#### Changed
- Refactored the codebase into per-concern modules (`button`, `thermal`, `switch_input`, `relay`) alongside `status_led`, each logging under its own tag.
- Moved subsystem initialization before `esp_matter::start()`, so the relay reaches its safe-low state earlier in boot and thermal protection is active before the Matter stack starts.

### [1.2.0] - 2026-05-14

Identify cluster and Multi-Admin LED fix.

#### Added
- Matter Identify cluster. The status LED blinks 100ms on and 900ms off for the requested duration, then returns to its previous state.
- Hardware version reporting of `v0.1.2`, matching the PCB silkscreen, in place of the SDK test default.

#### Fixed
- Multi-Admin LED state. The LED no longer drops into the BLE advertising blink when a commissioning window opens on an already-commissioned device. It now stays solid through Multi-Admin commissioning.

#### Changed
- Pinned `CONFIG_OPENTHREAD_FTD` explicitly rather than relying on an inherited value.

### [1.1.0] - 2026-05-11

Status LED support.

#### Added
- Status LED indication with four patterns: off while starting up, rapid blink while BLE advertising, slow blink while joining Thread, and solid when connected.

#### Changed
- Release tags now include model, generation, and variant (`shelly-1-gen4-light-vX.Y.Z`) to support future variants in the same repository.

#### Notes
- The status LED is on GPIO15 with active-low polarity, determined by GPIO sweep testing on real hardware, not GPIO0 as ESPHome documentation states.

### [1.0.0] - 2026-05-09

First public release. Matter On/Off Light device type with latching relay behavior.

#### Added
- Matter over Thread. The device joins your Thread mesh and is controlled directly over Matter.
- Multi-fabric commissioning to Apple Home, Home Assistant, and Google Home at the same time.
- Physical button toggle on the onboard relay button.
- External wall switch input on the SW terminal, hardware-debounced.
- State sync across all controllers.
- Thread Router mode to extend your Thread mesh for other devices.
- Factory reset via long press of the onboard button.
- Verified stock firmware backup and restore.

## Shelly 1 Gen4 Opener

### [1.0.1] - 2026-06-22

Closes a fail-open gap in the momentary pulse. No change to normal operation.

#### Fixed
- Pulse fail-open on timer-arm failure. If the one-shot auto-off timer could not be armed, for example under heap pressure during Matter, Thread, and BLE activity, the relay could remain energized instead of pulsing. The firmware now arms the auto-off before energizing and only closes the relay once its release is guaranteed, so a failed arm leaves the trigger de-asserted rather than held. The fix covers both timer creation and timer start. Reported by @456789TZ.

### [1.0.0] - 2026-06-16

First release of the Opener variant. Matter On/Off Plug-in Unit plus a Contact Sensor, built on the Light v1.2.1 module foundation and relay safety contract.

#### Added
- Momentary pulse relay. Turning the device on energizes the relay for a fixed pulse, then firmware returns it to off. The pulse is guaranteed in firmware through `relay_set()`, so a dropped command or a crashed hub cannot leave the relay energized. Default pulse width is 500ms.
- Door contact sensor on the SW terminal (GPIO10), exposed as a Matter Contact Sensor and reported to every commissioned fabric. In this variant the SW terminal is a sensor input, not a wall toggle.
- Three-endpoint Matter composition: an On/Off Plug-in Unit for the relay, a Contact Sensor for door state, and the Root Node.
- Compile-time constants in `main/app_priv.h`: `OPENER_PULSE_MS` (default 500) for pulse width and `CONTACT_OPEN_IS_HIGH` (default 0) for reed polarity.

## Shelly 1 Gen4 Outlet

### [1.0.0] - 2026-06-19

First release of the Outlet variant. Matter On/Off Plug-in Unit with latching relay behavior, built on the Light v1.2.1 module foundation and relay safety contract.

#### Added
- Latching relay presented as a Matter On/Off Plug-in Unit, so smart home apps display and voice-control the device as an outlet rather than a light. Electrically identical to the Light variant.
- External wall switch input on the SW terminal (GPIO10), hardware-debounced, toggling the relay as on the Light variant.
- Two-endpoint Matter composition: an On/Off Plug-in Unit for the relay, and the Root Node.

## Shelly 1 Gen4 Light Switch

### [1.0.0] - 2026-06-20

First release of the Light Switch variant, contributed by [Tomas McGuinness](https://github.com/tomasmcguinness). Derived from the Light 1.2.0 codebase, it adds detached-relay support so the SW terminal controls other Matter devices instead of the local relay.

#### Added
- Detached relay. The SW terminal (GPIO10) no longer drives the local relay. It sends a Matter OnOff Toggle command through a second endpoint's binding, so the wall switch can control other Matter devices such as bulbs, groups, or scenes. With no binding configured, flipping the switch does nothing locally.
- On/Off Light Switch endpoint driven by the SW input, with binding dispatch for both unicast and group bound commands.
- Two-endpoint Matter composition: an On/Off Light for the relay, controlled by app, voice, and the onboard button, and an On/Off Light Switch for the SW input, plus the Root Node.

## Shelly 1 Mini Gen4 Outlet

### [1.0.0] - 2026-07-06

First release for the Shelly 1 Mini Gen4, the first supported device beyond the Shelly 1 Gen4. Matter On/Off Plug-in Unit with latching relay behavior, at parity with the 1 Gen4 Outlet, plus a temperature sensor.

#### Added
- Latching relay presented as a Matter On/Off Plug-in Unit, on the Mini's own GPIO map, confirmed on hardware: relay GPIO10, button GPIO22, wall switch input GPIO12, status LED GPIO5.
- Temperature sensor endpoint reporting the ESP32-C6 die temperature to every commissioned fabric, updated on changes of 0.5°C or more.
- Three-endpoint Matter composition: an On/Off Plug-in Unit for the relay, a Temperature Sensor, and the Root Node.
- Distinct Matter Product ID (`0x8005`), so a Matter OTA image can only target this device and variant.

---

Full release notes, including tested scenarios, are on the [GitHub releases page](../../releases).
