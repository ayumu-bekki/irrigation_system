// ESP32 Irrigation System
// (C)2021 bekki.jp
// Utilities

// Include ----------------------
#include "util.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include <sstream>
#include <iomanip>

#include "define.h"

namespace IrrigationSystem {
namespace Util {

/// Sleep 
void SleepMillisecond(const unsigned int sleepMillisecond)
{
    portTickType lastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&lastWakeTime, sleepMillisecond / portTICK_PERIOD_MS);
}

/// SyncTime
void SyncSntpObtainTime()
{
    //  Initialize SNTP
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, CONFIG_NTP_SERVER_ADDRESS);
    sntp_init();
    
    // wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        SleepMillisecond(2000);
    }

    ESP_LOGI(TAG, "Finish SNTP");
}

tm GetLocalTime() 
{
    const time_t now = time(nullptr);
    tm timeinfo = {};
    localtime_r(&now, &timeinfo);
    return timeinfo;
}

std::string GetNowTimeStr()
{
    const tm timeinfo = GetLocalTime();
    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (timeinfo.tm_year + 1900) << "/" 
       << std::setw(2) << (timeinfo.tm_mon + 1) << "/" 
       << std::setw(2) << timeinfo.tm_mday << " "
       << std::setw(2) << timeinfo.tm_hour << ":" 
       << std::setw(2) << timeinfo.tm_min << ":" 
       << std::setw(2) << timeinfo.tm_sec;
       //<< " w:" << timeinfo.tm_wday // week
    
    return ss.str();
}

void PrintNow()
{
    ESP_LOGI(TAG, "Now:%s", GetNowTimeStr().c_str());
}

void InitTimeZone()
{
    setenv("TZ", CONFIG_LOCAL_TIME_ZONE, 1);
    tzset();
}


} // Util
} // IrrigationSystem

// EOF
