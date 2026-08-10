# Contributing

**[README](../README.md)** > **Contributing** · [Report an issue](../../../issues/new)

Contributions are welcome. This is a solo-maintained project, so response times vary, but every issue and PR gets read.

---

## Contents

- [What's in scope](#whats-in-scope)
- [Reporting issues](#reporting-issues)
- [Firmware filename convention](#firmware-filename-convention)
- [Pull requests](#pull-requests)
- [Contribution licensing](#contribution-licensing)
- [Communication](#communication)
- [Code of conduct](#code-of-conduct)

---

## What's in scope

The kinds of contributions most likely to land:

- Bug reports with reproducible serial logs
- Documentation fixes, clarifications, and typo catches
- Hardware compatibility reports for Shelly revisions or ecosystems I haven't tested
- Small, focused PRs that fix a specific issue

The kinds that are harder to land:

- Large refactors without a discussed-in-advance design
- Changes that weaken or remove the relay safety contract (pre-init safe-low, thermal lockout, Thread-loss hold, panic recovery, firmware-guaranteed pulse)
- Architectural changes to the module structure or the variant model

If you want to work on something larger, open an issue first to talk through the approach. Saves both of us time.

---

## Reporting issues

For a suspected security vulnerability, an attacker-reachable flaw or a credential or key exposure, use private reporting as described in the [security policy](../SECURITY.md) rather than a public issue. For functional bugs, including fail-safe and reliability issues, open a [GitHub issue](../../../issues/new) with:

- **Hardware:** which Shelly device, its PCB revision (silkscreened on the board), the variant, and the firmware version you're running.
- **Ecosystem:** Apple Home, Google Home, Alexa, or Home Assistant, and what provides Thread on your network (a Thread Border Router, or an iPhone 15 Pro or newer).
- **What happened:** What you expected, what actually occurred, and steps to reproduce.
- **Logs:** Serial monitor output at 115200 baud, especially anything around the failure point.

If your device won't commission, won't boot, or behaves unexpectedly after flashing, serial logs are usually the fastest path to a diagnosis.

If you built the firmware yourself rather than using a release, say so, and build in the dev container before reporting. A local ESP-IDF or esp-matter install is the usual reason a problem can't be reproduced here.

---

## Firmware filename convention

Release binaries follow this pattern.

```
automatous-io-{hardware}-{variant}-{version}.bin
```

- `{hardware}` is the device, matching its directory under `source/`: `shelly-1-gen4` or `shelly-1-mini-gen4`.
- `{variant}` is the firmware build: `light`, `opener`, `outlet`, or `light-switch`. See [Building from Source](BUILDING.md#repository-structure) for what each variant does.
- `{version}` is the release version, in the form `vX.Y.Z`.

A complete example is `automatous-io-shelly-1-gen4-light-vX.Y.Z.bin`.

---

## Pull requests

Before opening a PR for anything beyond a typo or a small bug fix, open an issue describing what you want to change and why. This avoids merge conflicts with work in progress and saves you from writing code that won't land for architectural reasons.

When the PR is ready:

- Keep it focused on one change. Multiple unrelated changes should be separate PRs.
- Build and test in the dev container. It pins the toolchain and the components the build pulls, so your build matches everyone else's. See [Building from Source](BUILDING.md).
- Match the existing code style and naming conventions.
- Include a brief test plan in the PR description: what hardware scenarios did you verify, and what was the outcome.
- Update relevant docs in the same PR if behavior changes.

---

## Contribution licensing

Contributions are accepted under Apache 2.0, the project's license. This follows from the Apache 2.0 submission terms: unless you state otherwise, what you submit for inclusion is licensed under Apache 2.0. By submitting a contribution you confirm it is your own work and that you have the right to submit it under these terms.

---

## Communication

GitHub issues and PRs are the only supported channels. Email and DMs aren't monitored for project work.

---

## Code of conduct

Be straightforward, technical, and direct. Disagreement is fine; hostility is not.

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
- [Contributors](CONTRIBUTORS.md) — people who have helped move the project forward
