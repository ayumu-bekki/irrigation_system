// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "wifi_manager.h"

#include "logger.h"
#include "util.h"

#include <cstring>

namespace IrrigationSystem {

// EventHandler
static void eventHandler(void* callbackObject, esp_event_base_t eventBase, int32_t eventId, void* eventData)
{
    if (callbackObject) {
        static_cast<WifiManager*>(callbackObject)->EventHandler(eventBase, eventId, eventData);
    }
}

WifiManager::WifiManager()
    :m_RetryNum(0)
{}

WifiManager::~WifiManager()
{
    Disconnect();
}

void WifiManager::Connect()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &eventHandler,
                                                        this,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &eventHandler,
                                                        this,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), CONFIG_WIFI_SSID, strlen(CONFIG_WIFI_SSID) + 1);
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), CONFIG_WIFI_PASSWORD, strlen(CONFIG_WIFI_PASSWORD) + 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "Start Wifi Connect.");

    ESP_ERROR_CHECK(esp_wifi_start());
}

void WifiManager::Disconnect()
{
    ESP_LOGI(TAG, "Disconnect WiFi.");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
}

void WifiManager::EventHandler(const esp_event_base_t eventBase, const int32_t eventId, void *const eventData)
{
    if (eventBase == WIFI_EVENT) {
        if (eventId == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (eventId == WIFI_EVENT_STA_DISCONNECTED) {
            if (CONFIG_WIFI_MAXIMUM_RETRY <= m_RetryNum ) {
                ESP_LOGE(TAG, "Failed Wi-Fi Connect");
            } else { 
                ++m_RetryNum;
                ESP_LOGW(TAG, "Disconnect Wi-Fi. retry to connect. try:%d", m_RetryNum);
                esp_wifi_connect();
            }
        }
    } else if (eventBase == IP_EVENT) {
        if (eventId == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
            ESP_LOGI(TAG, "Connected Wi-Fi. ip:" IPSTR, IP2STR(&event->ip_info.ip));
            m_RetryNum = 0;
        }
    }
}

} // IrrigationSystem

// EOF
