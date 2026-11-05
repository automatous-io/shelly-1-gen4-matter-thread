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

// button.h

#pragma once

#include <esp_err.h>

typedef void *shelly_button_handle_t;

// Initialize the device button (GPIO4 on Shelly 1 Gen4).
//
// Configures the onboard button as an active-low input. On press-down,
// toggles the OnOff attribute on the relay endpoint.
//
// Holding the button for 3-7 seconds switches the SW input mode between
// toggle and momentary (see input_mode.h). The status LED double-blinks
// while a release would change the mode, then confirms with one blink
// (toggle) or two (momentary); the relay returns to its pre-press state.
// Holding for 10 seconds or more and releasing factory resets the device.
//
// Factory reset is handled here, not by app_reset_button_register().
//
// Returns a button handle on success, NULL on failure.
shelly_button_handle_t button_init(void);