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

// Initialize the device button (GPIO22 on Shelly 1 Mini Gen4).
//
// Configures the onboard button as an active-low input. On press-down,
// toggles the OnOff attribute on the light endpoint.
//
// The returned handle can be passed to app_reset_button_register() to
// also wire factory-reset behavior to the same physical button.
//
// Returns a button handle on success, NULL on failure.
shelly_button_handle_t button_init(void);