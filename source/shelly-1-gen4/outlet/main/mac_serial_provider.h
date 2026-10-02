#pragma once

// Registers a DeviceInstanceInfoProvider whose SerialNumber is the
// factory eFuse MAC (12 uppercase hex chars). Call before esp_matter::start().
void mac_serial_provider_register();
