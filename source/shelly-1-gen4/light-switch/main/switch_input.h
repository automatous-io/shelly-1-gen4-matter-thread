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

// switch_input.h

#pragma once

#include <esp_err.h>
#include <stdbool.h>

// Initialize the wall switch input (GPIO10 on Shelly 1 Gen4).
//
// Configures GPIO10 as an interrupt-driven input. On any edge (rising
// or falling), debounces for 50ms and then toggles the Matter OnOff
// attribute on the light endpoint.
//
// This implements the "Toggle" behavior expected when wiring a standard
// wall switch to the SW terminal — every flip of the switch toggles the
// relay state, regardless of switch position.
//
// Each debounced edge also publishes the settled toggle position to the
// Boolean State cluster on the contact sensor endpoint, so controllers
// can see the physical switch state that detached mode otherwise hides.
//
// HARDWARE NOTE: The Shelly 1 Gen4 PCB has an external pull resistor
// on GPIO10. Do not enable the internal pullup or pulldown — internal
// pulls will fight the external circuit and prevent the ISR from
// firing. See switch_input.cpp for details.
//
// Returns ESP_OK on success, error code on failure.
esp_err_t switch_input_init(void);

// Read the current wall switch position as the Matter StateValue
// (true = toggle on/closed, false = off/open), applying
// SWITCH_ON_IS_HIGH. Used to seed the endpoint's initial value at boot.
bool switch_input_read_state(void);

// Push the current switch position to the Boolean State cluster on the
// contact sensor endpoint. Called on each debounced edge, and once after
// Matter starts to publish the boot state. Takes the CHIP stack lock
// internally.
void switch_input_report(void);