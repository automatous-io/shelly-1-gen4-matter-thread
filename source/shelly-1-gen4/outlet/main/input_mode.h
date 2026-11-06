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

// input_mode.h

#pragma once

#include <esp_err.h>
#include <esp_matter.h>
#include <stdint.h>

// How the SW terminal is interpreted. The values are the Matter Mode
// Select mode numbers, so they are part of the device's external
// interface: do not renumber them.
typedef enum : uint8_t {
    // Latching wall switch (the default). Every change of position
    // toggles the relay, so either position can turn the load on or off.
    INPUT_MODE_TOGGLE = 0,

    // Momentary push button ("bell press"). Each press toggles the relay;
    // the release does nothing.
    INPUT_MODE_MOMENTARY = 1,
} input_mode_t;

// Create the "SW input mode" endpoint: a Matter Mode Select device whose
// CurrentMode selects the input_mode_t above. CurrentMode is stored in
// NVS by esp-matter, so the selection survives reboots and OTA updates
// and is cleared by a factory reset (which returns the device to Toggle).
//
// Controllers change it with the standard ModeSelect ChangeToMode
// command. Call once, after the existing endpoints, so their endpoint
// ids stay stable for devices updating over OTA.
esp_err_t input_mode_create_endpoint(esp_matter::node_t *node);

// The mode currently selected. Returns INPUT_MODE_TOGGLE until the
// endpoint exists. Safe to call from any task.
input_mode_t input_mode_get(void);

// Select a mode locally (the onboard button gesture) and report the
// change to subscribed controllers. Takes the CHIP stack lock internally.
esp_err_t input_mode_set(input_mode_t mode);

// Human-readable name of a mode, for logs.
const char *input_mode_name(input_mode_t mode);
