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

// relay.cpp

#include "relay.h"
#include "thermal.h"

#include <esp_log.h>
#include <driver/gpio.h>

// SAFETY: This module is the single point of relay control. See relay.h
// for the full safety invariant and contributor convention. Briefly:
//   - relay_init() drives the GPIO LOW immediately after configuring it
//     as output. Do not reorder.
//   - relay_set(true) is gated by thermal_is_faulted(). Do not bypass.
//   - relay_set(false) is always allowed. Do not gate it.

static const char *TAG = "relay";

// Shelly 1 Mini Gen4 mains-switching relay is on GPIO10, active high.
// HIGH = energized, LOW = de-energized.
#define RELAY_GPIO  GPIO_NUM_10

esp_err_t relay_init(void)
{
    gpio_config_t relay_cfg = {
        .pin_bit_mask = (1ULL << RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&relay_cfg);

    // SAFETY: Drive the relay LOW as the first action after output
    // configuration. This enforces the safe-low boot invariant
    // documented in relay.h.
    gpio_set_level(RELAY_GPIO, 0);

    ESP_LOGI(TAG, "Relay initialized on GPIO%d (de-energized)", RELAY_GPIO);
    return ESP_OK;
}

esp_err_t relay_set(bool power)
{
    // SAFETY: Refuse turn-on while thermal lockout is active. Turn-off
    // is always allowed. The safe direction is never blocked.
    if (power && thermal_is_faulted()) {
        ESP_LOGW(TAG, "Thermal fault active, relay turn-on blocked");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Setting relay: %s", power ? "ON" : "OFF");
    return gpio_set_level(RELAY_GPIO, power ? 1 : 0);
}