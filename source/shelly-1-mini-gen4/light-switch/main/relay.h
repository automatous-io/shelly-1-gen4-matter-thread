//
// Copyright 2026 AUTOMATOUS.IO
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// relay.h

#pragma once

#include <esp_err.h>
#include <stdbool.h>

// Relay control for the Shelly 1 Mini Gen4 mains-switching relay (GPIO10).
//
// This module owns the relay GPIO and is the single point of relay
// control in the firmware. All relay state changes from Matter
// commands, the onboard button, the wall switch input, thermal faults,
// or boot-time defaults go through relay_set().
//
// SAFETY INVARIANT
// ----------------
// The relay shall de-energize (drive LOW) under all conditions where
// the firmware cannot guarantee the user's intended state:
//
//   1. PRE-INIT / BOOT: relay_init() drives the GPIO LOW immediately
//      after configuring it as an output. The window between chip
//      reset and relay_init() executing is governed by the hardware's
//      reset state. The Shelly PCB pulls the MOSFET gate low in this
//      window, so the relay stays de-energized.
//
//   2. THERMAL FAULT: the thermal monitor (thermal.cpp) calls
//      relay_set(false) when the board exceeds 75°C sustained for 10
//      seconds. relay_set() enforces a lockout. While thermal_is_faulted()
//      returns true, calls to relay_set(true) are refused.
//
//   3. THREAD / NETWORK LOSS: the relay state is INTENTIONALLY held.
//      Loss of Thread connectivity does not change physical state.
//      This is by design. Users should not lose control of their
//      lighting because of a network blip. It is not a safety failure.
//
//   4. UNHANDLED EXCEPTION / PANIC: the ESP32 hardware watchdog resets
//      the chip. On the next boot, relay_init() re-enforces the
//      safe-low invariant. The relay is briefly in its hardware
//      reset state during the panic-to-reboot window.
//
// CONTRIBUTOR CONVENTION
// ----------------------
// Any change that touches relay control must preserve these
// invariants. In particular:
//   - Do not add bypass paths to relay_set() that skip the thermal
//     lockout check.
//   - Do not remove or reorder the safe-low GPIO drive in relay_init().
//   - Do not add code paths that turn the relay ON without going
//     through relay_set().
//
// Reviewers should reject pull requests that weaken these guarantees
// without explicit discussion of the safety implications.

// Initialize the relay subsystem. Configures GPIO10 as an output and
// drives it LOW. Must be called once during boot, before any other
// relay-related code runs.
//
// Returns ESP_OK on success, error code on failure.
esp_err_t relay_init(void);

// Set the relay state.
//
// power=true:  Energize the relay (close the contacts, turn the load ON).
//              REFUSED with ESP_ERR_INVALID_STATE if thermal_is_faulted()
//              is true.
// power=false: De-energize the relay (open the contacts, turn the load OFF).
//              Always allowed. Turning off is never blocked.
//
// Safe to call from any task. Does not block.
//
// Returns ESP_OK on success, ESP_ERR_INVALID_STATE if blocked by
// thermal lockout, or error code from the underlying GPIO driver.
esp_err_t relay_set(bool power);