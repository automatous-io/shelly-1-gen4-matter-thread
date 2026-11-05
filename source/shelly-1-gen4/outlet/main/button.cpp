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

// button.cpp

#include "button.h"

#include <app_priv.h>
#include <input_mode.h>
#include <status_led.h>

#include <esp_log.h>
#include <esp_matter.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <button_gpio.h>
#include <iot_button.h>

static const char *TAG = "button";

// Shelly 1 Gen4 onboard button is on GPIO4, active low.
#define BUTTON_GPIO         GPIO_NUM_4
#define BUTTON_ACTIVE_LEVEL 0

// Hold gestures, both acting on release and timed from press-down:
//
//   held < 3s        plain press (the relay already toggled on press-down)
//   3s  <= held < 7s switch the SW input mode (LED double-blinks)
//   7s  <= held < 10s nothing: a dead zone so a hold meant for one
//                     gesture can never trigger the other
//   held >= 10s      factory reset
//
// The factory reset is timed here rather than by esp-matter's app_reset,
// which arms at iot_button's CONFIG_BUTTON_LONG_PRESS_TIME_MS. That option
// is capped at 5000ms by the button component's Kconfig, so the 10000 in
// sdkconfig.defaults.c6_thread_shelly was rejected and the reset armed at
// the 1500ms default, leaving no room for the mode gesture.
#define MODE_WINDOW_START_MS     3000
#define MODE_WINDOW_END_MS       7000
#define FACTORY_RESET_HOLD_MS    10000

static_assert(MODE_WINDOW_END_MS < FACTORY_RESET_HOLD_MS,
              "SW input mode window must end before the factory reset hold");

// All of the state below is touched only from iot_button callbacks and
// esp_timer callbacks, which both run on the esp_timer task, so they
// never run concurrently.
static int64_t s_press_start_us = 0;
static bool s_pre_press_on = false;
static esp_timer_handle_t s_arm_timer = nullptr;
static esp_timer_handle_t s_disarm_timer = nullptr;

static void mode_arm_cb(void *arg)
{
    ESP_LOGI(TAG, "Release now to change the SW input mode");
    status_led_mode_armed(true);
}

static void mode_disarm_cb(void *arg)
{
    status_led_mode_armed(false);
}

static void button_press_down_cb(void *arg, void *data)
{
    // Toggle immediately so the button stays responsive. The pre-press
    // state is kept so the mode gesture can put the relay back.
    s_pre_press_on = app_driver_relay_get();
    s_press_start_us = esp_timer_get_time();

    ESP_LOGI(TAG, "Toggle button pressed");
    app_driver_relay_toggle();

    esp_timer_stop(s_arm_timer);
    esp_timer_stop(s_disarm_timer);
    esp_timer_start_once(s_arm_timer, MODE_WINDOW_START_MS * 1000LL);
    esp_timer_start_once(s_disarm_timer, MODE_WINDOW_END_MS * 1000LL);
}

static void button_press_up_cb(void *arg, void *data)
{
    esp_timer_stop(s_arm_timer);
    esp_timer_stop(s_disarm_timer);
    status_led_mode_armed(false);

    int64_t held_ms = (esp_timer_get_time() - s_press_start_us) / 1000;

    if (held_ms >= FACTORY_RESET_HOLD_MS) {
        ESP_LOGI(TAG, "Button held %lldms, starting factory reset", held_ms);
        esp_matter::factory_reset();
        return;
    }
    if (held_ms < MODE_WINDOW_START_MS || held_ms >= MODE_WINDOW_END_MS) {
        return;
    }

    input_mode_t next = (input_mode_get() == INPUT_MODE_TOGGLE) ? INPUT_MODE_MOMENTARY : INPUT_MODE_TOGGLE;
    ESP_LOGI(TAG, "Button held %lldms, switching SW input mode to %s", held_ms, input_mode_name(next));

    if (input_mode_set(next) != ESP_OK) {
        return;
    }

    // The gesture is a configuration change, not a request to switch the
    // load: undo the toggle made on press-down.
    app_driver_relay_update(s_pre_press_on);

    // One blink for toggle, two for momentary.
    status_led_mode_confirm(next == INPUT_MODE_MOMENTARY ? 2 : 1);
}

shelly_button_handle_t button_init(void)
{
    button_handle_t handle = NULL;
    const button_config_t btn_cfg = {0};

    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = BUTTON_GPIO,
        .active_level = BUTTON_ACTIVE_LEVEL,
    };

    if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create button device");
        return NULL;
    }

    const esp_timer_create_args_t arm_args = {
        .callback = mode_arm_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "mode_arm",
        .skip_unhandled_events = true,
    };
    const esp_timer_create_args_t disarm_args = {
        .callback = mode_disarm_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "mode_disarm",
        .skip_unhandled_events = true,
    };
    if (esp_timer_create(&arm_args, &s_arm_timer) != ESP_OK ||
        esp_timer_create(&disarm_args, &s_disarm_timer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create SW input mode timers");
        return NULL;
    }

    iot_button_register_cb(handle, BUTTON_PRESS_DOWN, NULL, button_press_down_cb, NULL);
    iot_button_register_cb(handle, BUTTON_PRESS_UP, NULL, button_press_up_cb, NULL);
    ESP_LOGI(TAG, "Button initialized on GPIO%d", BUTTON_GPIO);
    return (shelly_button_handle_t)handle;
}
