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

// status_led.cpp

#include "status_led.h"

#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "status_led";

#define STATUS_LED_GPIO     GPIO_NUM_5
#define LED_ON_LEVEL        0   // Active-low: pin LOW = LED ON
#define LED_OFF_LEVEL       1   // Active-low: pin HIGH = LED OFF

static volatile led_state_t current_state = LED_STATE_OFF;
static volatile led_state_t pre_identify_state = LED_STATE_OFF;

static void status_led_task(void *arg)
{
    bool led_on = false;
    uint32_t period_ms = 100;  // How often the task wakes up

    while (true) {
        led_state_t state = current_state;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        switch (state) {
        case LED_STATE_OFF:
            gpio_set_level(STATUS_LED_GPIO, LED_OFF_LEVEL);
            break;

        case LED_STATE_BLE_ADVERTISING:
            // Rapid blink: 200ms on, 200ms off (400ms period)
            led_on = (now % 400) < 200;
            gpio_set_level(STATUS_LED_GPIO, led_on ? LED_ON_LEVEL : LED_OFF_LEVEL);
            break;

        case LED_STATE_THREAD_CONNECTING:
            // Slow blink: 500ms on, 1500ms off (2000ms period)
            led_on = (now % 2000) < 500;
            gpio_set_level(STATUS_LED_GPIO, led_on ? LED_ON_LEVEL : LED_OFF_LEVEL);
            break;

        case LED_STATE_THREAD_CONNECTED:
            gpio_set_level(STATUS_LED_GPIO, LED_ON_LEVEL);
            break;

        case LED_STATE_FAULT:
            // Very rapid blink: 100ms on, 100ms off (200ms period)
            led_on = (now % 200) < 100;
            gpio_set_level(STATUS_LED_GPIO, led_on ? LED_ON_LEVEL : LED_OFF_LEVEL);
            break;

        case LED_STATE_IDENTIFY:
            // Asymmetric blink: 100ms on, 900ms off (1000ms period)
            // Distinct short flash with long gap — used for Matter Identify
            led_on = (now % 1000) < 100;
            gpio_set_level(STATUS_LED_GPIO, led_on ? LED_ON_LEVEL : LED_OFF_LEVEL);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}

esp_err_t status_led_init(void)
{
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&led_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIO: %d", err);
        return err;
    }

    // Start with LED off
    gpio_set_level(STATUS_LED_GPIO, LED_OFF_LEVEL);

    // Spawn LED task
    BaseType_t result = xTaskCreate(status_led_task, "status_led", 2048, NULL, 5, NULL);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Status LED initialized on GPIO%d", STATUS_LED_GPIO);
    return ESP_OK;
}

void status_led_set_state(led_state_t state)
{
    if (state != current_state) {
        ESP_LOGI(TAG, "LED state change: %d -> %d", current_state, state);
        current_state = state;
    }
}

void status_led_start_identify(void)
{
    // Only save state if we're not already identifying
    // (prevents losing the original state if Identify is re-triggered)
    if (current_state != LED_STATE_IDENTIFY) {
        pre_identify_state = current_state;
        ESP_LOGI(TAG, "Identify start - saving state %d, entering identify", pre_identify_state);
    } else {
        ESP_LOGI(TAG, "Identify already active - extending");
    }
    current_state = LED_STATE_IDENTIFY;
}

void status_led_stop_identify(void)
{
    if (current_state == LED_STATE_IDENTIFY) {
        ESP_LOGI(TAG, "Identify stop - restoring state %d", pre_identify_state);
        current_state = pre_identify_state;
    } else {
        ESP_LOGI(TAG, "Identify stop called but not identifying (state=%d)", current_state);
    }
}