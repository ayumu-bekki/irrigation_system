// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "wifi_manager.h"

#include <cstring>

#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

// EventHandler
static void eventHandler(void* callback_object, esp_event_base_t event_base,
                         int32_t event_id, void* event_data) {
  if (callback_object) {
    static_cast<WifiManager*>(callback_object)
        ->EventHandler(event_base, event_id, event_data);
  }
}

WifiManager::WifiManager() : retry_num_(0) {}

WifiManager::~WifiManager() { Disconnect(); }

void WifiManager::Connect() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, this, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, this, &instance_got_ip));

  wifi_config_t wifi_config = {};
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), CONFIG_WIFI_SSID,
               strlen(CONFIG_WIFI_SSID) + 1);
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
               CONFIG_WIFI_PASSWORD, strlen(CONFIG_WIFI_PASSWORD) + 1);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  ESP_LOGI(TAG, "Start Wifi Connect.");

  ESP_ERROR_CHECK(esp_wifi_start());
}

void WifiManager::Disconnect() {
  ESP_LOGI(TAG, "Disconnect WiFi.");
  ESP_ERROR_CHECK(esp_wifi_disconnect());
  ESP_ERROR_CHECK(esp_wifi_stop());
}

void WifiManager::EventHandler(const esp_event_base_t event_base,
                               const int32_t event_id, void* const event_data) {
  if (event_base == WIFI_EVENT) {
    if (event_id == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      if (CONFIG_WIFI_MAXIMUM_RETRY <= retry_num_) {
        ESP_LOGE(TAG, "Failed Wi-Fi Connect. System Restart...");
        esp_restart();
      } else {
        ++retry_num_;
        ESP_LOGW(TAG, "Disconnect Wi-Fi. retry to connect. try:%d", retry_num_);
        esp_wifi_connect();
      }
    }
  } else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);
      ESP_LOGI(TAG, "Connected Wi-Fi. ip:" IPSTR, IP2STR(&event->ip_info.ip));
      retry_num_ = 0;
    }
  }
}

}  // namespace IrrigationSystem

// EOF
