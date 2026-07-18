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

// thermal.cpp

#include "thermal.h"
#include "relay.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <esp_matter.h>
#include <driver/temperature_sensor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

// Defined in app_main.cpp. Used to know which Matter endpoint's OnOff
// attribute to update on thermal fault.
extern uint16_t relay_endpoint_id;

// Defined in app_main.cpp. The Temperature Sensor endpoint that publishes
// the die temperature readings.
extern uint16_t temperature_endpoint_id;

static const char *TAG = "thermal";

// Trip threshold: sustained 75°C for 10 seconds.
// Selected to be well below safe operating limits while providing
// margin for transient peaks during high-load switching.
#define TEMP_TRIP_CELSIUS   75.0f
#define TEMP_SUSTAINED_MS   10000

// Polling interval for the monitor task.
#define TEMP_POLL_INTERVAL_MS 2000

// Stack size for the monitor task. attribute::update() runs the Matter
// data-model and reporting on this stack; 2048 overflows it.
#define THERMAL_TASK_STACK_SIZE 4096

// Internal state.
static temperature_sensor_handle_t temp_sensor = NULL;
static int64_t over_temp_start_ms = 0;
static volatile bool fault_active = false;

static void thermal_check(void)
{
    float temp = 0;
    esp_err_t err = temperature_sensor_get_celsius(temp_sensor, &temp);
    if (err != ESP_OK) {
        return;
    }

    ESP_LOGD(TAG, "Board temp: %.1f C", temp);

    // Publish to the Temperature Sensor endpoint in Matter units (0.01°C).
    // Only on a change of 0.5°C or more, so a steady temperature does not
    // generate a Thread report every poll. Skip while the endpoint id is
    // still 0: the first poll runs before the Matter node exists.
    static int16_t last_reported_centi = INT16_MIN;
    int16_t centi = (int16_t)(temp * 100.0f);
    if (temperature_endpoint_id != 0 && abs(centi - last_reported_centi) >= 50) {
        last_reported_centi = centi;
        esp_matter_attr_val_t temp_val = esp_matter_nullable_int16(centi);
        attribute::update(temperature_endpoint_id, TemperatureMeasurement::Id,
                          TemperatureMeasurement::Attributes::MeasuredValue::Id, &temp_val);
    }

    if (temp > TEMP_TRIP_CELSIUS) {
        // Above threshold. Start timing if we haven't already.
        if (over_temp_start_ms == 0) {
            over_temp_start_ms = esp_timer_get_time() / 1000;
        } else {
            int64_t elapsed = (esp_timer_get_time() / 1000) - over_temp_start_ms;
            if (elapsed >= TEMP_SUSTAINED_MS && !fault_active) {
                ESP_LOGE(TAG, "Thermal fault, temp: %.1f C, forcing relay off", temp);
                fault_active = true;

                // Belt-and-suspenders fault response.
                // 1. Direct hardware: relay_set(false) drives the GPIO low.
                //    Always succeeds; turn-off is never blocked.
                // 2. Matter attribute update: so connected ecosystems see
                //    the state change.
                relay_set(false);

                uint32_t cluster_id = OnOff::Id;
                uint32_t attribute_id = OnOff::Attributes::OnOff::Id;
                esp_matter_attr_val_t val = esp_matter_bool(false);
                attribute::update(relay_endpoint_id, cluster_id, attribute_id, &val);
            }
        }
    } else {
        // Back below threshold. Clear timing and fault state.
        over_temp_start_ms = 0;
        fault_active = false;
    }
}

static void thermal_monitor_task(void *arg)
{
    while (true) {
        thermal_check();
        vTaskDelay(pdMS_TO_TICKS(TEMP_POLL_INTERVAL_MS));
    }
}

esp_err_t thermal_init(void)
{
    temperature_sensor_config_t temp_cfg = {
        .range_min = 20,
        .range_max = 100,
    };
    temperature_sensor_install(&temp_cfg, &temp_sensor);
    temperature_sensor_enable(temp_sensor);

    xTaskCreate(thermal_monitor_task, "thermal_monitor",
                THERMAL_TASK_STACK_SIZE, NULL, 5, NULL);

    ESP_LOGI(TAG, "Thermal monitor initialized (trip: %.1f C sustained %dms)",
             TEMP_TRIP_CELSIUS, TEMP_SUSTAINED_MS);
    return ESP_OK;
}

bool thermal_is_faulted(void)
{
    return fault_active;
}