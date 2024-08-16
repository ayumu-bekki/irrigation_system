#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <esp_event.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace IrrigationSystem {

class WifiManager final {
 public:
  WifiManager();
  ~WifiManager();

  void Connect();

  void Disconnect();

  void EventHandler(const esp_event_base_t event_base, const int32_t event_id,
                    void *const event_data);

 private:
  int retry_num_;
};

}  // namespace IrrigationSystem

#endif  // WIFI_MANAGER_H_
// EOF
