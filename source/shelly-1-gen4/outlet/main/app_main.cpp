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

// app_main.cpp

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <common_macros.h>
#include <log_heap_numbers.h>

#include <app_priv.h>
#include <app_reset.h>
#include <status_led.h>
#include <button.h>
#include <thermal.h>
#include <switch_input.h>
#include <relay.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#ifdef CONFIG_ENABLE_SET_CERT_DECLARATION_API
#include <esp_matter_providers.h>
#include <lib/support/Span.h>
#ifdef CONFIG_SEC_CERT_DAC_PROVIDER
#include <platform/ESP32/ESP32SecureCertDACProvider.h>
#elif defined(CONFIG_FACTORY_PARTITION_DAC_PROVIDER)
#include <platform/ESP32/ESP32FactoryDataProvider.h>
#endif
using namespace chip::DeviceLayer;
#endif

static const char *TAG = "app_main";
// The On/Off (Plug-in Unit) endpoint that drives the relay.
uint16_t relay_endpoint_id = 0;
// The Temperature Sensor endpoint that reports the die temperature.
uint16_t temperature_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;

constexpr auto k_timeout_seconds = 300;

#ifdef CONFIG_ENABLE_SET_CERT_DECLARATION_API
extern const uint8_t cd_start[] asm("_binary_certification_declaration_der_start");
extern const uint8_t cd_end[] asm("_binary_certification_declaration_der_end");

const chip::ByteSpan cdSpan(cd_start, static_cast<size_t>(cd_end - cd_start));
#endif // CONFIG_ENABLE_SET_CERT_DECLARATION_API

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");

static const char *s_decryption_key = decryption_key_start;
static const uint16_t s_decryption_key_len = decryption_key_end - decryption_key_start;
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;
    
    case chip::DeviceLayer::DeviceEventType::kThreadStateChange:
        ESP_LOGI(TAG, "Thread state change");
        if (chip::DeviceLayer::ConnectivityMgr().IsThreadAttached()) {
            ESP_LOGI(TAG, "Thread is attached");
            status_led_set_state(LED_STATE_THREAD_CONNECTED);
        } else {
            ESP_LOGI(TAG, "Thread is NOT attached");
            // Only switch to CONNECTING if we're not still advertising
            // (avoid overwriting the rapid-blink state during initial setup)
            if (chip::Server::GetInstance().GetFabricTable().FabricCount() > 0) {
                status_led_set_state(LED_STATE_THREAD_CONNECTING);
            }
        }
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        MEMORY_PROFILER_DUMP_HEAP_STAT("commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        MEMORY_PROFILER_DUMP_HEAP_STAT("commissioning window opened");
        // Only show BLE advertising LED if device is truly uncommissioned.
        // Commissioned devices may open windows for Multi-Admin pairing.
        // In that case, preserve the connected state to avoid confusing users.
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
            status_led_set_state(LED_STATE_BLE_ADVERTISING);
        } else {
            ESP_LOGI(TAG, "Device already commissioned, keeping current LED state");
        }
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved: {
        ESP_LOGI(TAG, "Fabric removed successfully");
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
            chip::CommissioningWindowManager  &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
            constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
            if (!commissionMgr.IsCommissioningWindowOpen()) {
                // After removing last fabric, re-open commissioning window via DNS-SD
                CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
                                                                            chip::CommissioningWindowAdvertisement::kDnssdOnly);
                if (err != CHIP_NO_ERROR) {
                    ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
                }
            }
        }
        break;
    }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        MEMORY_PROFILER_DUMP_HEAP_STAT("BLE deinitialized");
        break;

    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
// Triggered by controllers (Apple Home, HA, Google Home) when a user wants
// to physically identify which device is which. The LED enters an asymmetric
// blink pattern for the duration of the Identify command, then restores.
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);

    switch (type) {
    case identification::callback_type_t::START:
        ESP_LOGI(TAG, "Identify START - blinking status LED");
        status_led_start_identify();
        break;

    case identification::callback_type_t::STOP:
        ESP_LOGI(TAG, "Identify STOP - restoring status LED");
        status_led_stop_identify();
        break;

    case identification::callback_type_t::EFFECT:
        // TriggerEffect command - effect_id specifies which effect.
        // All effects currently map to the basic identify blink.
        ESP_LOGI(TAG, "Identify EFFECT %u variant %u - using default blink", effect_id, effect_variant);
        status_led_start_identify();
        break;

    default:
        ESP_LOGW(TAG, "Unknown identification callback type: %u", type);
        break;
    }

    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        /* Driver update */
        app_driver_handle_t driver_handle = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    // Initialize the ESP NVS layer
    nvs_flash_init();

    // Initialize status LED
    status_led_init();

    MEMORY_PROFILER_DUMP_HEAP_STAT("Bootup");

    // Initialize subsystems. Order matters for safety:
    // relay first so the GPIO is in known-safe state before anything else.
    // thermal second so overtemp protection is active as early as possible.
    // switch_input and button last for user-facing inputs.
    relay_init();
    thermal_init();
    switch_input_init();
    shelly_button_handle_t button_handle = button_init();
    app_reset_button_register(button_handle);

    // Create a Matter node and add the mandatory Root Node device type on endpoint 0
    node::config_t node_config;

    // node handle can be used to add/modify other endpoints.
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    MEMORY_PROFILER_DUMP_HEAP_STAT("node created");

    // On/Off Plug-in Unit drives the outlet relay (presents as an outlet,
    // not a light). Latching on/off; the wall switch (SW) and button toggle it.
    on_off_plug_in_unit::config_t plugin_config;
    plugin_config.on_off.on_off = DEFAULT_POWER;

    // endpoint handles can be used to add/modify clusters.
    endpoint_t *endpoint = on_off_plug_in_unit::create(node, &plugin_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create on_off_plug_in_unit endpoint"));

    relay_endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Outlet relay created with endpoint_id %d", relay_endpoint_id);

    // Temperature Sensor endpoint exposes the die temperature the thermal
    // monitor already polls. Starts null until the first reading; min/max
    // match the 20-100°C range the sensor is configured for in thermal.cpp.
    temperature_sensor::config_t temp_sensor_config;
    temp_sensor_config.temperature_measurement.measured_value = nullable<int16_t>();
    temp_sensor_config.temperature_measurement.min_measured_value = 2000;
    temp_sensor_config.temperature_measurement.max_measured_value = 10000;

    endpoint = temperature_sensor::create(node, &temp_sensor_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create temperature_sensor endpoint"));

    temperature_endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Temperature sensor created with endpoint_id %d", temperature_endpoint_id);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD && CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION
    // Enable secondary network interface
    secondary_network_interface::config_t secondary_network_interface_config;
    endpoint = endpoint::secondary_network_interface::create(node, &secondary_network_interface_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create secondary network interface endpoint"));
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    // Set OpenThread platform config
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

#ifdef CONFIG_ENABLE_SET_CERT_DECLARATION_API
    auto * dac_provider = get_dac_provider();
#ifdef CONFIG_SEC_CERT_DAC_PROVIDER
    static_cast<ESP32SecureCertDACProvider *>(dac_provider)->SetCertificationDeclaration(cdSpan);
#elif defined(CONFIG_FACTORY_PARTITION_DAC_PROVIDER)
    static_cast<ESP32FactoryDataProvider *>(dac_provider)->SetCertificationDeclaration(cdSpan);
#endif
#endif // CONFIG_ENABLE_SET_CERT_DECLARATION_API

    // Matter start
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    MEMORY_PROFILER_DUMP_HEAP_STAT("matter started");

    // Starting driver with default values
    app_driver_relay_set_defaults(relay_endpoint_id);

#if CONFIG_ENABLE_ENCRYPTED_OTA
    err = esp_matter_ota_requestor_encrypted_init(s_decryption_key, s_decryption_key_len);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to initialized the encrypted OTA, err: %d", err));
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::attribute_register_commands();
#if CONFIG_OPENTHREAD_CLI
    esp_matter::console::otcli_register_commands();
#endif
    esp_matter::console::init();
#endif

    while (true) {
        MEMORY_PROFILER_DUMP_HEAP_STAT("Idle");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
