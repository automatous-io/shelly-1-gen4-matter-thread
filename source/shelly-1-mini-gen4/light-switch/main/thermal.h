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

// thermal.h

#pragma once

#include <esp_err.h>
#include <stdbool.h>

// Initialize the thermal monitor.
//
// Configures the ESP32-C6 internal temperature sensor and spawns a
// background task that polls the temperature every 2 seconds. Readings
// are published to the Matter Temperature Sensor endpoint. If the
// board temperature exceeds 75°C sustained for 10 seconds, the thermal
// monitor will:
//   1. Drive the relay off (via relay_set(false))
//   2. Update the Matter OnOff attribute to reflect the off state
//   3. Set the thermal fault flag, blocking further relay-on commands
//      until temperature returns to a safe range.
//
// The thermal fault flag automatically clears when temperature drops
// back below 75°C (no manual reset needed).
//
// Returns ESP_OK on success, error code on failure.
esp_err_t thermal_init(void);

// Returns true if the thermal monitor has tripped and is currently
// blocking relay activation. Used by relay control code to refuse
// turn-on requests while overtemp.
//
// Safe to call from any task.
bool thermal_is_faulted(void);