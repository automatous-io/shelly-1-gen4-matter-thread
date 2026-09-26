//
// Copyright 2026 AUTOMATOUS.IO
// Portions derived from Espressif esp-matter examples,
// originally released into the public domain / CC0.
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

// app_driver.cpp

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <esp_matter.h>
#include <esp_matter_client.h>
#include <app_priv.h>
#include <common_macros.h>

#include <relay.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "app_driver";
extern uint16_t light_endpoint_id;
extern uint16_t switch_endpoint_id;

// Invoked by the binding manager once the bound peer's CASE session is ready.
// This is where the queued command is actually put on the wire.
static void send_command_success_callback(void *context, const ConcreteCommandPath &command_path,
                                          const chip::app::StatusIB &status, TLVReader *response_data)
{
    ESP_LOGI(TAG, "Bound command sent successfully");
}

static void send_command_failure_callback(void *context, CHIP_ERROR error)
{
    ESP_LOGE(TAG, "Bound command send failed: %" CHIP_ERROR_FORMAT, error.Format());
}

// Unicast binding callback: build and send the invoke command to the bound peer.
static void app_driver_client_callback(client::peer_device_t *peer_device, client::request_handle_t *req_handle,
                                       void *priv_data)
{
    if (req_handle->type != esp_matter::client::INVOKE_CMD) {
        return;
    }
    if (req_handle->command_path.mClusterId != OnOff::Id) {
        ESP_LOGE(TAG, "Unsupported cluster 0x%08" PRIx32 " for bound command", req_handle->command_path.mClusterId);
        return;
    }
    // OnOff Toggle/On/Off take no fields, so the command data is an empty object.
    const char *command_data_str = "{}";
    client::interaction::invoke::send_request(NULL, peer_device, req_handle->command_path, command_data_str,
                                              send_command_success_callback, send_command_failure_callback,
                                              chip::NullOptional);
}

// Group binding callback: send the command to the bound group.
static void app_driver_client_group_callback(uint8_t fabric_index, client::request_handle_t *req_handle,
                                             void *priv_data)
{
    if (req_handle->type != esp_matter::client::INVOKE_CMD) {
        return;
    }
    if (req_handle->command_path.mClusterId != OnOff::Id) {
        ESP_LOGE(TAG, "Unsupported cluster 0x%08" PRIx32 " for bound group command", req_handle->command_path.mClusterId);
        return;
    }
    const char *command_data_str = "{}";
    client::interaction::invoke::send_group_request(fabric_index, req_handle->command_path, command_data_str);
}

esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;
    if (endpoint_id == light_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                err = relay_set(val->val.b);
            }
        }
    }
    return err;
}

void app_driver_init()
{
    // Register the binding dispatch callbacks. Without these, client::cluster_update()
    // resolves the bound peer and opens a CASE session but never emits the command.
    client::set_request_callback(app_driver_client_callback, app_driver_client_group_callback, NULL);
}

esp_err_t app_driver_light_set_defaults(uint16_t endpoint_id)
{
    esp_err_t err = ESP_OK;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    attribute_t *attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err = relay_set(val.val.b);

    return err;
}