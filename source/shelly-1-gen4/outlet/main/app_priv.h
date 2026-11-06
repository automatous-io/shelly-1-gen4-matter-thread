//
// Copyright 2026 AUTOMATOUS.IO
// Portions derived from Espressif esp-matter examples,
// originally released into the public domain / CC0.
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

// app_priv.h

#pragma once

#include <esp_err.h>
#include <esp_matter.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include "esp_openthread_types.h"
#endif

// Default attribute values used during initialization
#define DEFAULT_POWER false

typedef void *app_driver_handle_t;

// Driver Update
//
// This API should be called to update the driver for the attribute being updated.
// This is usually called from the common `app_attribute_update_cb()`.
//
// @param[in] endpoint_id Endpoint ID of the attribute.
// @param[in] cluster_id Cluster ID of the attribute.
// @param[in] attribute_id Attribute ID of the attribute.
// @param[in] val Pointer to `esp_matter_attr_val_t`. Use appropriate elements as per the value type.
//
// @return ESP_OK on success.
// @return error in case of failure.
//
esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val);

// Set defaults for the relay driver
//
// Set the attribute drivers to their default values from the created data model.
//
// @param[in] endpoint_id Endpoint ID of the driver.
//
// @return ESP_OK on success.
// @return error in case of failure.

esp_err_t app_driver_relay_set_defaults(uint16_t endpoint_id);

// Local relay control for the physical inputs (SW terminal and onboard
// button). These go through the Matter OnOff attribute rather than
// relay_set() directly, so controllers see the change and the attribute
// callback still applies the relay's safety checks (thermal latch).

// Current value of the relay endpoint's OnOff attribute.
bool app_driver_relay_get(void);

// Set the relay endpoint's OnOff attribute.
esp_err_t app_driver_relay_update(bool on);

// Invert the relay endpoint's OnOff attribute.
esp_err_t app_driver_relay_toggle(void);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                           \
    {                                                                                   \
        .radio_mode = RADIO_MODE_NATIVE,                                                \
    }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                            \
    {                                                                                   \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,                              \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                            \
    {                                                                                   \
        .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10, \
    }
#endif
