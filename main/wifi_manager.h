#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_event.h>
#include <esp_wifi.h>

namespace IrrigationSystem {

class WifiManager final
{
public:
    WifiManager();
    ~WifiManager();

    void Connect();

    void Disconnect();

    void EventHandler(const esp_event_base_t eventBase, const int32_t eventId, void *const eventData);

private:
    int m_RetryNum;
};

} // IrrigationSystem

#endif // WIFI_MANAGER_H_
// EOF
