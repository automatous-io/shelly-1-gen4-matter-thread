//
// Copyright 2026 AUTOMATOUS.IO
//
// Detached relay support (the SW input drives a Matter binding instead of the
// local relay) contributed by Tomas McGuinness (https://github.com/tomasmcguinness).
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

#include <esp_log.h>
#include <esp_matter.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// BooleanState StateValue is read-only / internally managed, so it must be
// updated via the cluster setter, not attribute::update().
#include <clusters/boolean_state/integration.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "switch_input";

// Defined in app_main.cpp. Used to know which Matter endpoint's OnOff
// attribute to toggle when the switch input fires.
extern uint16_t switch_endpoint_id;

// Defined in app_main.cpp. The endpoint whose Boolean State StateValue
// attribute mirrors the wall toggle position. Zero until the endpoint is
// created; edges before then still send the toggle but skip the report.
extern uint16_t contact_sensor_endpoint_id;

// Shelly 1 Gen4 wall switch input.
//
// HARDWARE NOTE: GPIO_PULLUP_DISABLE and GPIO_PULLDOWN_DISABLE are
// required. The Shelly PCB has an external pull resistor on GPIO10.
// Enabling either internal pull will fight the external circuit and
// prevent the ISR from firing. Do not "fix" this by adding a pull.
#define SWITCH_INPUT_GPIO   GPIO_NUM_10
#define DEBOUNCE_MS         50
#define QUEUE_DEPTH         10
#define TASK_STACK_SIZE     4096
#define TASK_PRIORITY       10

// ISR-to-task communication queue. Written by the ISR (FromISR),
// read by the task. File-static so the ISR can reach it.
static QueueHandle_t switch_evt_queue = nullptr;

static void IRAM_ATTR switch_input_isr(void *arg)
{
    uint32_t gpio_num = SWITCH_INPUT_GPIO;
    xQueueSendFromISR(switch_evt_queue, &gpio_num, nullptr);
}

bool switch_input_read_state(void)
{
    // Map the raw GPIO level to the Matter StateValue (true = toggle on).
    int level = gpio_get_level(SWITCH_INPUT_GPIO);
    return (level == (SWITCH_ON_IS_HIGH ? 1 : 0));
}

void switch_input_report(void)
{
    // The endpoint may not exist yet during early boot edges.
    if (contact_sensor_endpoint_id == 0) {
        return;
    }

    bool state_value = switch_input_read_state();

    // Runs outside the Matter task, so take the CHIP stack lock.
    // SetStateValue() updates the attribute and emits the report.
    esp_matter::lock::ScopedChipStackLock lock(portMAX_DELAY);
    auto *cluster = chip::app::Clusters::BooleanState::FindClusterOnEndpoint(contact_sensor_endpoint_id);
    if (cluster == nullptr) {
        ESP_LOGE(TAG, "BooleanState cluster not found on endpoint %u", contact_sensor_endpoint_id);
        return;
    }
    cluster->SetStateValue(state_value);
    ESP_LOGI(TAG, "Switch position updated: %s", state_value ? "ON" : "OFF");
}

static void switch_input_task(void *arg)
{
    while (true) {
        uint32_t io_num;
        if (xQueueReceive(switch_evt_queue, &io_num, portMAX_DELAY)) {
            // 50ms debounce
            vTaskDelay(pdMS_TO_TICKS(50));

            // Drain any additional events during debounce
            while (xQueueReceive(switch_evt_queue, &io_num, 0) == pdTRUE) {}

            ESP_LOGI(TAG, "SW input changed. Sending Toggle command");
            client::request_handle_t req_handle;
            req_handle.type = esp_matter::client::INVOKE_CMD;
            req_handle.command_path.mClusterId = OnOff::Id;
            req_handle.command_path.mCommandId = OnOff::Commands::Toggle::Id;

            // Scoped so the stack lock releases before the report takes
            // its own below.
            {
                lock::ScopedChipStackLock lock(portMAX_DELAY);
                client::cluster_update(switch_endpoint_id, &req_handle);
            }

            ESP_LOGI(TAG, "Switch input toggled");

            switch_input_report();
        }
    }
}

esp_err_t switch_input_init(void)
{
    // Create the ISR-to-task communication queue and the task that
    // services it. The task must exist before the ISR can fire
    // (otherwise events queue up with no consumer), so we create them
    // in this order.
    switch_evt_queue = xQueueCreate(QUEUE_DEPTH, sizeof(uint32_t));
    xTaskCreate(switch_input_task, "switch_input", TASK_STACK_SIZE,
                nullptr, TASK_PRIORITY, nullptr);

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

    ESP_LOGI(TAG, "Switch input initialized on GPIO%d", SWITCH_INPUT_GPIO);
    return ESP_OK;
}