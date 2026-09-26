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

// status_led.h

#pragma once

#include <esp_err.h>

typedef enum {
    LED_STATE_OFF,                  // LED off (idle, not advertising)
    LED_STATE_BLE_ADVERTISING,      // Rapid blink — uncommissioned, advertising
    LED_STATE_THREAD_CONNECTING,    // Slow blink — joining Thread network
    LED_STATE_THREAD_CONNECTED,     // Solid on — fully operational
    LED_STATE_FAULT,                // Very rapid blink — error condition
    LED_STATE_IDENTIFY,             // Asymmetric blink — Matter Identify command

} led_state_t;

// Initialize the status LED system. Call once during boot.
esp_err_t status_led_init(void);

// Change the LED state. Safe to call from any task.
void status_led_set_state(led_state_t state);

// Enter Identify mode. Saves current state for later restoration.
void status_led_start_identify(void);

// Exit Identify mode. Restores previous state.
void status_led_stop_identify(void);