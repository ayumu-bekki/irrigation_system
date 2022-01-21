// ESP32 Irrigation System
// (C)2021 bekki.jp
// Utilities

// Include ----------------------
#include "util.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sntp.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include <sstream>
#include <iomanip>
#include <cmath>

#include "logger.h"

namespace IrrigationSystem {
namespace Util {

/// Sleep 
void SleepMillisecond(const unsigned int sleepMillisecond)
{
    portTickType lastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&lastWakeTime, sleepMillisecond / portTICK_PERIOD_MS);
}

/// Init Sntp
void InitializeSntp()
{
    ESP_LOGI(TAG, "Initializing SNTP");
}

/// Notification Time Synced 
void TimeSyncedCallback(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event. Now:%s", GetNowTimeStr().c_str());
}


/// Start SyncTime
void SyncSntpObtainTime()
{
    ESP_LOGI(TAG, "Start Sync SNTP");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, CONFIG_NTP_SERVER_ADDRESS);
    sntp_set_time_sync_notification_cb(TimeSyncedCallback);
    sntp_init();

    // wait for time to be set
    int retry = 0;
    static constexpr int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        SleepMillisecond(2000);
    }
}

std::time_t GetEpoch()
{
    std::chrono::system_clock::time_point nowTimePoint = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(nowTimePoint);
}

std::tm EpochToLocalTime(const std::time_t epoch)
{
    return *std::localtime(&epoch);
}

std::tm GetLocalTime() 
{
    return EpochToLocalTime(GetEpoch());
}

std::string TimeToStr(const std::tm& timeInfo)
{
    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (timeInfo.tm_year + 1900) << "/" 
       << std::setw(2) << (timeInfo.tm_mon + 1) << "/" 
       << std::setw(2) << timeInfo.tm_mday << " "
       << std::setw(2) << timeInfo.tm_hour << ":" 
       << std::setw(2) << timeInfo.tm_min << ":" 
       << std::setw(2) << timeInfo.tm_sec;
    return ss.str();
}

std::string GetNowTimeStr()
{
    return TimeToStr(GetLocalTime());
}

void InitTimeZone()
{
    setenv("TZ", CONFIG_LOCAL_TIME_ZONE, 1);
    tzset();
}

/// Gregorian calendar to Modified Julian Date
int32_t GregToMJD(const std::tm& timeInfo)
{
    const double year = timeInfo.tm_year + 1900;
    const double month = timeInfo.tm_mon + 1;
    return std::floor(365.25 * year)
         + std::floor(year / 400)
         - std::floor(year / 100)
         + std::floor(30.59 * (month - 2.0))
         + timeInfo.tm_mday
         - 678912;
}

/// Get ChronoMinutes from hours and minutes.
std::chrono::minutes GetChronoHourMinutes(const std::tm& timeInfo)
{
    return std::chrono::hours(timeInfo.tm_hour) + std::chrono::minutes(timeInfo.tm_min);
}

std::vector<std::string> SplitString(const std::string &str, const char delim)
{
    std::vector<std::string> elements;
    std::stringstream ss(str);
    std::string item;
    while (getline(ss, item, delim)) {
        if (!item.empty()) {
            elements.push_back(item);
        }
    }
    return elements;
}


/// Get Original Voltage Divider Resistor
// input outputVoltage[mv] topResistanceValue[kΩ], bottomRegistanceValue[kΩ]
// return voltage[V] 
float GetOriginalVoltageFromDividerRegister(const uint32_t outputVoltage, const float topResistanceValue, const float bottomRegistanceValue)
{
    const float voltageDivRate = bottomRegistanceValue / (topResistanceValue + bottomRegistanceValue);
    return outputVoltage / voltageDivRate / 1000.0f;
}


} // Util
} // IrrigationSystem

// EOF
