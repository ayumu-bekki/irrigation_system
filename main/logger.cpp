// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "logger.h"

namespace IrrigationSystem {
namespace Logger {

void InitializeLogLevel() 
{
#if CONFIG_DEBUG != 0
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    ESP_LOGI(TAG, "DEBUG MODE");
#else
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);
#endif
}

} // Logger
} // IrrigationSystem

// EOF
