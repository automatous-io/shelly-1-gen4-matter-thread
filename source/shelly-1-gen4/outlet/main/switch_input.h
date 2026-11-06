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

// Initialize the wall switch input (GPIO10 on Shelly 1 Gen4).
//
// Configures GPIO10 as an interrupt-driven input. An edge wakes a task
// that samples the pin until its level has held steady for the debounce
// window. Only a settled change of level counts; a burst that settles
// back where it started is treated as noise and ignored.
//
// What a settled change does depends on the SW input mode (input_mode.h):
//
// - Toggle (default): any change of position toggles the relay, so a
//   standard latching wall switch works from either position.
// - Momentary: only a change to closed (a press) toggles the relay; the
//   release does nothing. Presses closer together than a short lockout
//   are ignored, to absorb contact chatter on cheap bell pushes.
//
// The mode is read on every event, so a change takes effect on the next
// press without a reboot.
//
// HARDWARE NOTE: The Shelly 1 Gen4 PCB has an external pull resistor
// on GPIO10. Do not enable the internal pullup or pulldown — internal
// pulls will fight the external circuit and prevent the ISR from
// firing. See switch_input.cpp for details.
//
// Returns ESP_OK on success, error code on failure.
esp_err_t switch_input_init(void);
