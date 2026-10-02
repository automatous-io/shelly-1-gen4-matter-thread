#include "mac_serial_provider.h"

#include <esp_mac.h>
#include <cstdio>
#include <platform/ConfigurationManager.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <platform/internal/GenericDeviceInstanceInfoProvider.h>
#include <platform/ESP32/ESP32Config.h>

using namespace chip::DeviceLayer;

namespace {

class MacSerialInfoProvider
    : public Internal::GenericDeviceInstanceInfoProvider<Internal::ESP32Config>
{
public:
    MacSerialInfoProvider()
        : Internal::GenericDeviceInstanceInfoProvider<Internal::ESP32Config>(
              ConfigurationManagerImpl::GetDefaultInstance())
    {}

    CHIP_ERROR GetSerialNumber(char *buf, size_t bufSize) override
    {
        uint8_t mac[6];
        if (esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY) != ESP_OK) {
            return CHIP_ERROR_INTERNAL;
        }
        int n = snprintf(buf, bufSize, "%02X%02X%02X%02X%02X%02X",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return (n > 0 && static_cast<size_t>(n) < bufSize)
                   ? CHIP_NO_ERROR
                   : CHIP_ERROR_BUFFER_TOO_SMALL;
    }
};

} // namespace

void mac_serial_provider_register()
{
    static MacSerialInfoProvider s_provider;
    SetDeviceInstanceInfoProvider(&s_provider);
}