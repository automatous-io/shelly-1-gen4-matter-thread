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

// switch_input.cpp

#include "switch_input.h"

#include <app_priv.h>
#include <input_mode.h>

#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static const char *TAG = "switch_input";

// Shelly 1 Gen4 wall switch input.
//
// HARDWARE NOTE: GPIO_PULLUP_DISABLE and GPIO_PULLDOWN_DISABLE are
// required. The Shelly PCB has an external pull resistor on GPIO10.
// Enabling either internal pull will fight the external circuit and
// prevent the ISR from firing. Do not "fix" this by adding a pull.
#define SWITCH_INPUT_GPIO   GPIO_NUM_10

// GPIO level while the SW terminal is closed (switch on / button held).
// Confirmed on hardware: SW closed reads HIGH.
#define SW_CLOSED_LEVEL     1

// Debounce. After an edge the pin is sampled every SAMPLE_MS until it has
// held one level for STABLE_MS. Mechanical bounce is typically under
// 10ms; 30ms of steady level is comfortably past it while still catching
// a quick tap on a push button. SAMPLE_MS matches the 10ms FreeRTOS tick.
#define SAMPLE_MS           10
#define STABLE_MS           30

// Give up on an edge burst that has not settled within this long. A
// healthy switch settles in tens of milliseconds; anything longer is
// noise or a wiring fault, and acting on it would be a guess.
#define SETTLE_TIMEOUT_MS   500

// Momentary mode: ignore a press that follows an accepted press by less
// than this. Absorbs re-closure chatter on release from cheap bell
// pushes, which can outlast the debounce window.
#define MOMENTARY_LOCKOUT_MS 250

#define QUEUE_DEPTH         10
#define TASK_STACK_SIZE     4096
#define TASK_PRIORITY       10

// ISR-to-task communication queue. Written by the ISR (FromISR),
// read by the task. File-static so the ISR can reach it.
static QueueHandle_t switch_evt_queue = nullptr;

// Last settled level of the input. Seeded at init so the position the
// switch is in at boot never counts as a change. Tracked in every mode,
// so switching modes needs no resync. Only the task touches it.
static int s_stable_level = 0;

static void IRAM_ATTR switch_input_isr(void *arg)
{
    uint32_t gpio_num = SWITCH_INPUT_GPIO;
    xQueueSendFromISR(switch_evt_queue, &gpio_num, nullptr);
}

// Sample the input until it holds one level for STABLE_MS. Returns true
// with that level in *level, or false if it never settles within
// SETTLE_TIMEOUT_MS.
static bool wait_for_stable_level(int *level)
{
    int last = gpio_get_level(SWITCH_INPUT_GPIO);
    uint32_t stable_ms = 0;

    for (uint32_t elapsed_ms = 0; elapsed_ms < SETTLE_TIMEOUT_MS; elapsed_ms += SAMPLE_MS) {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
        int now = gpio_get_level(SWITCH_INPUT_GPIO);
        if (now == last) {
            stable_ms += SAMPLE_MS;
            if (stable_ms >= STABLE_MS) {
                *level = now;
                return true;
            }
        } else {
            last = now;
            stable_ms = 0;
        }
    }
    return false;
}

static void switch_input_task(void *arg)
{
    // Tick of the last press accepted in momentary mode. Starts far enough
    // in the past that the first press is never locked out.
    TickType_t last_press = xTaskGetTickCount() - pdMS_TO_TICKS(MOMENTARY_LOCKOUT_MS);

    while (true) {
        uint32_t io_num;
        if (!xQueueReceive(switch_evt_queue, &io_num, portMAX_DELAY)) {
            continue;
        }

        int level;
        bool settled = wait_for_stable_level(&level);

        // Edges queued while sampling belong to the burst just handled.
        while (xQueueReceive(switch_evt_queue, &io_num, 0) == pdTRUE) {}

        if (!settled) {
            ESP_LOGW(TAG, "SW input did not settle within %dms, ignoring", SETTLE_TIMEOUT_MS);
            continue;
        }
        if (level == s_stable_level) {
            ESP_LOGD(TAG, "SW input glitch, level unchanged");
            continue;
        }
        s_stable_level = level;

        bool closed = (level == SW_CLOSED_LEVEL);
        input_mode_t mode = input_mode_get();

        if (mode == INPUT_MODE_MOMENTARY) {
            if (!closed) {
                ESP_LOGD(TAG, "SW released (momentary), no action");
                continue;
            }
            TickType_t now = xTaskGetTickCount();
            if ((now - last_press) < pdMS_TO_TICKS(MOMENTARY_LOCKOUT_MS)) {
                ESP_LOGI(TAG, "SW press within lockout, ignoring");
                continue;
            }
            last_press = now;
            ESP_LOGI(TAG, "SW pressed (momentary), toggling relay");
        } else {
            ESP_LOGI(TAG, "SW turned %s (toggle), toggling relay", closed ? "on" : "off");
        }

        // The attribute update runs the standard callback path, which
        // drives the relay via relay_set().
        app_driver_relay_toggle();
    }
}

esp_err_t switch_input_init(void)
{
    // Configure GPIO10 as edge-interrupt input. See HARDWARE NOTE above
    // for why both internal pulls are disabled.
    gpio_config_t switch_cfg = {
        .pin_bit_mask = (1ULL << SWITCH_INPUT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&switch_cfg);

    // Seed the settled level before anything can consume an edge.
    s_stable_level = gpio_get_level(SWITCH_INPUT_GPIO);

    // Create the ISR-to-task communication queue and the task that
    // services it. The task must exist before the ISR can fire
    // (otherwise events queue up with no consumer), so we create them
    // before adding the handler.
    switch_evt_queue = xQueueCreate(QUEUE_DEPTH, sizeof(uint32_t));
    xTaskCreate(switch_input_task, "switch_input", TASK_STACK_SIZE,
                nullptr, TASK_PRIORITY, nullptr);

    // Install the GPIO ISR service. ESP_ERR_INVALID_STATE means it was
    // already installed by another module (the Matter stack or
    // iot_button via button.cpp), which is fine — we share a single
    // ISR service across the whole app.
    esp_err_t isr_err = gpio_install_isr_service(0);
    if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install ISR service: %d", isr_err);
    }

    esp_err_t handler_err = gpio_isr_handler_add(SWITCH_INPUT_GPIO,
                                                  switch_input_isr, nullptr);
    if (handler_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add switch ISR handler: %d", handler_err);
    }

    ESP_LOGI(TAG, "Switch input initialized on GPIO%d (level %d)", SWITCH_INPUT_GPIO, s_stable_level);
    return ESP_OK;
}
