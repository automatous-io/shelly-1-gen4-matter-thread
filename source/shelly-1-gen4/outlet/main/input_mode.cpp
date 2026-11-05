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

// input_mode.cpp

#include "input_mode.h"

#include <string.h>
#include <iterator>

#include <esp_log.h>
#include <esp_matter.h>

#include <app/clusters/mode-select-server/supported-modes-manager.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "input_mode";

// Endpoint 0 is the root node, so 0 doubles as "not created yet".
static uint16_t s_endpoint_id = 0;

namespace {

using ModeOption = ModeSelect::Structs::ModeOptionStruct::Type;

ModeOption make_option(const char *label, input_mode_t mode)
{
    ModeOption option;
    option.label = chip::CharSpan::fromCharString(label);
    option.mode = mode;
    // No standard namespace applies to a switch input, so the modes carry
    // no semantic tags; the labels describe them.
    return option;
}

// The SupportedModes list. Labels are what Home Assistant shows in the
// select entity, so keep them short and plain.
const ModeOption k_modes[] = {
    make_option("Toggle switch", INPUT_MODE_TOGGLE),
    make_option("Momentary (bell press)", INPUT_MODE_MOMENTARY),
};

// Serves SupportedModes and validates ChangeToMode for the Mode Select
// server. The server holds a single global manager, which is fine: this
// is the only Mode Select endpoint on the device.
class InputModesManager : public ModeSelect::SupportedModesManager
{
public:
    ModeOptionsProvider getModeOptionsProvider(chip::EndpointId endpointId) const override
    {
        if (s_endpoint_id == 0 || endpointId != s_endpoint_id) {
            return ModeOptionsProvider(nullptr, nullptr);
        }
        return ModeOptionsProvider(std::begin(k_modes), std::end(k_modes));
    }

    chip::Protocols::InteractionModel::Status getModeOptionByMode(chip::EndpointId endpointId, uint8_t mode,
                                                                  const ModeOption **dataPtr) const override
    {
        if (s_endpoint_id == 0 || endpointId != s_endpoint_id) {
            return chip::Protocols::InteractionModel::Status::UnsupportedCluster;
        }
        for (const auto &option : k_modes) {
            if (option.mode == mode) {
                *dataPtr = &option;
                return chip::Protocols::InteractionModel::Status::Success;
            }
        }
        return chip::Protocols::InteractionModel::Status::InvalidCommand;
    }
};

InputModesManager s_modes_manager;

} // namespace

const char *input_mode_name(input_mode_t mode)
{
    switch (mode) {
    case INPUT_MODE_TOGGLE:
        return "toggle";
    case INPUT_MODE_MOMENTARY:
        return "momentary";
    }
    return "unknown";
}

esp_err_t input_mode_create_endpoint(node_t *node)
{
    endpoint::mode_select::config_t config;
    strncpy(config.mode_select.description, "SW input mode",
            sizeof(config.mode_select.description) - 1);
    // Default for a fresh device. A device that has already chosen a mode
    // keeps it: esp-matter restores the non-volatile CurrentMode from NVS
    // when the attribute is created, overriding this value.
    config.mode_select.current_mode = INPUT_MODE_TOGGLE;
    config.mode_select.delegate = &s_modes_manager;

    endpoint_t *endpoint = endpoint::mode_select::create(node, &config, ENDPOINT_FLAG_NONE, nullptr);
    if (endpoint == nullptr) {
        ESP_LOGE(TAG, "Failed to create mode_select endpoint");
        return ESP_FAIL;
    }

    s_endpoint_id = endpoint::get_id(endpoint);

    // A stored value from a future firmware with more modes, or a corrupt
    // one, falls back to Toggle: the behavior of every earlier release.
    esp_matter_attr_val_t val = esp_matter_invalid(nullptr);
    attribute_t *attribute = attribute::get(s_endpoint_id, ModeSelect::Id, ModeSelect::Attributes::CurrentMode::Id);
    attribute::get_val(attribute, &val);
    const ModeOption *option = nullptr;
    if (s_modes_manager.getModeOptionByMode(s_endpoint_id, val.val.u8, &option) !=
        chip::Protocols::InteractionModel::Status::Success) {
        ESP_LOGW(TAG, "Unsupported stored mode %u, using toggle", val.val.u8);
        val = esp_matter_uint8(INPUT_MODE_TOGGLE);
        attribute::set_val(attribute, &val, false);
    }

    ESP_LOGI(TAG, "SW input mode endpoint created with endpoint_id %u, mode %s",
             s_endpoint_id, input_mode_name(input_mode_get()));
    return ESP_OK;
}

input_mode_t input_mode_get(void)
{
    if (s_endpoint_id == 0) {
        return INPUT_MODE_TOGGLE;
    }

    attribute_t *attribute = attribute::get(s_endpoint_id, ModeSelect::Id, ModeSelect::Attributes::CurrentMode::Id);
    if (attribute == nullptr) {
        return INPUT_MODE_TOGGLE;
    }

    esp_matter_attr_val_t val = esp_matter_invalid(nullptr);
    attribute::get_val(attribute, &val);
    switch (val.val.u8) {
    case INPUT_MODE_MOMENTARY:
        return INPUT_MODE_MOMENTARY;
    case INPUT_MODE_TOGGLE:
    default:
        return INPUT_MODE_TOGGLE;
    }
}

esp_err_t input_mode_set(input_mode_t mode)
{
    if (s_endpoint_id == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    // update() takes the CHIP stack lock, persists the non-volatile
    // attribute to NVS, and reports the change to subscribers.
    esp_matter_attr_val_t val = esp_matter_uint8(mode);
    esp_err_t err = attribute::update(s_endpoint_id, ModeSelect::Id, ModeSelect::Attributes::CurrentMode::Id, &val);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set SW input mode: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "SW input mode set to %s", input_mode_name(mode));
    return ESP_OK;
}
