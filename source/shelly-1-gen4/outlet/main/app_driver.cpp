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

// app_driver.cpp

#include <esp_log.h>

#include <esp_matter.h>
#include <app_priv.h>
#include <common_macros.h>

#include <relay.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

extern uint16_t relay_endpoint_id;

esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;
    if (endpoint_id == relay_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                err = relay_set(val->val.b);
            }
        }
    }
    return err;
}

esp_err_t app_driver_relay_set_defaults(uint16_t endpoint_id)
{
    esp_err_t err = ESP_OK;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    attribute_t *attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err = relay_set(val.val.b);

    return err;
}

bool app_driver_relay_get(void)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute_t *attribute = attribute::get(relay_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    return val.val.b;
}

esp_err_t app_driver_relay_update(bool on)
{
    // update() takes the CHIP stack lock and runs the attribute callback,
    // which drives the relay through relay_set().
    esp_matter_attr_val_t val = esp_matter_bool(on);
    return attribute::update(relay_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
}

esp_err_t app_driver_relay_toggle(void)
{
    return app_driver_relay_update(!app_driver_relay_get());
}