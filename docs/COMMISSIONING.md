# Commissioning

**[README](../README.md)** > **Commissioning** · [Report an issue](../../../issues/new)

This guide covers pairing a freshly flashed Shelly Gen4 device to your smart home ecosystem, sharing one device across multiple ecosystems, reading the status LED, troubleshooting a stalled commissioning, and factory resetting the device. It notes the Opener variant's differences where they apply. If you have not flashed the firmware yet, start with the [Flashing Guide](FLASHING.md). Once it is commissioned, [updates are available](UPDATING.md) through Home Assistant with Matter OTA, with commissioning preserved.

---

## Contents

- [Status LED reference](#status-led-reference)
- [Before commissioning](#before-commissioning)
- [Apple Home](#apple-home)
- [Google Home, Alexa, Home Assistant](#google-home-alexa-home-assistant)
- [Commissioning to multiple ecosystems](#commissioning-to-multiple-ecosystems)
- [Opener variant](#opener-variant)
- [Troubleshooting](#troubleshooting)
- [Factory reset](#factory-reset)

---

## Status LED reference

The LED follows Shelly's standard convention. A solid LED means connected and working.

| LED pattern | Timing | Meaning |
|---|---|---|
| Off | n/a | Device starting up |
| Rapid blink | ~200ms on, 200ms off | BLE advertising, ready for commissioning |
| Slow blink | ~1s on, 1s off | Joining Thread network |
| Solid on | Continuous | Connected and operational |
| Identify blink | 100ms on, 900ms off | Identify command active |

---

## Before commissioning

Commission devices one at a time. Power off or move away any other uncommissioned devices running Automatous firmware while you set each one up. All devices ship with the same pairing code, so a controller can get confused if several are advertising at once. Once a device is commissioned it gets a unique identity on your network. After commissioning, disconnect the UART from the Shelly and install it wherever you need it.

![Matter setup QR code](images/matter_qrcode_20202021_3840.png)

```
Setup code: 3497-011-2332
```

> This is the ESP-Matter SDK test setup code, used by every device running this firmware. Once a device is commissioned to your Matter fabric, the setup code is no longer used for authentication, and your Matter ecosystem manages credentials from then on. Multiple uncommissioned devices broadcasting the same setup code is the reason to commission one device at a time. See [Certification](CERTIFICATION.md) for what the uncertified warning means.

---

## Apple Home

Scan the QR code above with the Home app, or enter the setup code manually.

1. Open the Home app.
2. Tap the plus button, then Add Accessory.
3. Scan the QR code above, or tap More Options to enter the setup code manually.
4. Press Add Anyway when prompted. This is expected. See [Certification](CERTIFICATION.md) for why.
5. Name the device and place it in a room.

> Note for users coming from HomeKit Bridge or Home Assistant setups. HomeKit's reclassify as Fan, Light, or Switch option, available when bridging a stock Shelly through Home Assistant, is not available with native Matter devices. The Matter device type is declared by the firmware at flash time and cannot be changed in the Apple Home app. If you need a different device type, flash the [variant](../README.md#variants) that provides it.

---

## Google Home, Alexa, Home Assistant

Google Home and Alexa support Matter commissioning through their respective apps. Use setup code `3497-011-2332` or the QR code above when prompted. Google and Alexa hubs have not been directly tested, but the firmware should work with any Matter-compliant hub. Let us know and we will update the compatible hubs section.

Home Assistant commissions through its Matter integration. Use the same setup code or QR code. Because this firmware uses Matter test credentials, Home Assistant's Matter Server needs Test Net DCL enabled before it will commission an uncertified device like this one.

Turn on **Enable test-net DCL usage** in the Matter Server add-on configuration and restart the add-on. This is the same setting that later lets Home Assistant serve [Matter OTA updates](UPDATING.md), so enabling it now covers both commissioning and updates.

Verified on **Matter Server add-on, version 9.0.2**.

---

## Commissioning to multiple ecosystems

Matter lets one device belong to several ecosystems at the same time. This is called multi-fabric, or Multi-Admin. You do not run the initial setup again for each one. Instead, an ecosystem the device already belongs to opens a short pairing window and hands out a fresh, temporary setup code that the next ecosystem uses to join.

1. Commission the device to your first ecosystem as described above.
2. In that ecosystem, open the device and choose the option to add it to another platform. In Apple Home this is "Turn On Pairing Mode" in the accessory settings. In Home Assistant it is the share or "Add to another fabric" action on the device page.
3. The first ecosystem displays a temporary setup code.
4. In the second ecosystem's app, start adding a Matter device and enter that temporary code, not the original setup code printed above.
5. Repeat for any further ecosystems.

The status LED stays solid through this process. The device is already on Thread, so it does not return to the BLE advertising blink when the pairing window opens.

---

## Opener variant

The Opener variant commissions exactly as above. It behaves differently from the Light variant once paired.

- It presents as a switch or outlet for the relay trigger, plus a separate contact sensor for door state. It is not a light.
- The trigger is momentary. When you turn it on from a controller, the relay energizes for about 500ms and the firmware returns it to off. The control tile may briefly show on and then revert to off. This is the controller reconciling a momentary action that Matter models as On/Off, and it is normal. Triggering from the onboard button updates almost instantly.

### Apple Home

The contact sensor can be recategorized to Garage Door in its accessory settings, which gives a native garage door status display. Door, Window, and Blinds are also available. This changes only how Apple Home labels and displays the sensor. It does not change the Matter device type, which is fixed at flash time. The relay tile acts as the momentary trigger.

### Home Assistant

Home Assistant exposes the Opener as a switch for the trigger and a binary sensor for door state. To get a single garage tile, combine them into a Cover template with `device_class: garage`, using the switch to trigger and the binary sensor to report open or closed.

---

## Troubleshooting

### Commissioning hangs in Home Assistant

If Home Assistant's Matter integration hangs during commissioning or shows "setting up" indefinitely, restart the Matter Server add-on.

1. Go to Settings, then Apps, then Matter Server, then Restart.
2. Wait 60 seconds for the full restart.
3. [Factory reset the Shelly](#factory-reset).
4. Retry commissioning.

---

## Factory reset

Hold the onboard relay button for several seconds. On this firmware that clears the device's Matter commissioning and returns it to BLE commissioning mode. It does not restore the original Shelly firmware or factory state; the Automatous firmware stays installed, and going back to stock requires the UART backup (see [Reversibility](REVERSIBILITY.md)). The LED returns to the rapid blink pattern from the [status LED reference](#status-led-reference), which confirms the reset worked. Remove the device from any ecosystems you added it to before re-commissioning.

---

## Related documentation

- [README](../README.md) — project overview and quick start
- [Why Matter over Thread](WHY.md) — the rationale for Matter over Thread
- [Flashing Guide](FLASHING.md) — wiring, backing up stock firmware, and flashing
- [Reversibility](REVERSIBILITY.md) — warranty, factory keys, and how reversible flashing is
- [Updating](UPDATING.md) — keeping a device current after flashing
- [Power Consumption](POWER.md) — measured draw and the Thread Router design choice
- [Building from Source](BUILDING.md) — compiling the firmware yourself
- [GPIO Map](GPIO.md) — ESP-Shelly-C68F pin assignments for Gen4 devices
- [Stock Flash Partitions](PARTITIONS.md) — stock flash layout and the factory `shelly` partition
- [Certification](CERTIFICATION.md) — uncertified status and test credentials
- [Roadmap](ROADMAP.md) — current known limitations and planned work
- [Contributing](CONTRIBUTING.md) — reporting bugs and the firmware filename convention
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
