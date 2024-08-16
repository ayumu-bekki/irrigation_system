// ESP32 Irrigation System
// (C)2021 bekki.jp
// Utilities

// Include ----------------------
#include "util.h"

#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include <cmath>
#include <iomanip>
#include <sstream>

#include "gpio_control.h"
#include "logger.h"

namespace IrrigationSystem {
namespace Util {

/// Sleep
void SleepMillisecond(const unsigned int sleep_miliseconds) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  vTaskDelayUntil(&lastWakeTime, sleep_miliseconds / portTICK_PERIOD_MS);
}

/// Init Sntp
void InitializeSntp() { ESP_LOGI(TAG, "Initializing SNTP"); }

/// Notification Time Synced
void TimeSyncedCallback(struct timeval* tv) {
  ESP_LOGI(TAG, "Time synchronization event. Now:%s", GetNowTimeStr().c_str());
}

/// Start SyncTime
void SyncSntpObtainTime() {
  ESP_LOGI(TAG, "Start Sync SNTP");

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, CONFIG_NTP_SERVER_ADDRESS);
  esp_sntp_set_time_sync_notification_cb(TimeSyncedCallback);
  esp_sntp_init();

  // wait for time to be set
  int retry = 0;
  static constexpr int retry_count = 100;
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET &&
         ++retry < retry_count) {
    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry,
             retry_count);
    SleepMillisecond(2000);
  }
}

std::time_t GetEpoch() {
  std::chrono::system_clock::time_point now_time_point =
      std::chrono::system_clock::now();
  return std::chrono::system_clock::to_time_t(now_time_point);
}

std::tm EpochToLocalTime(const std::time_t epoch) {
  return *std::localtime(&epoch);
}

std::tm GetLocalTime() { return EpochToLocalTime(GetEpoch()); }

std::string TimeToStr(const std::tm& timeInfo) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(4) << (timeInfo.tm_year + 1900) << "/"
     << std::setw(2) << (timeInfo.tm_mon + 1) << "/" << std::setw(2)
     << timeInfo.tm_mday << " " << std::setw(2) << timeInfo.tm_hour << ":"
     << std::setw(2) << timeInfo.tm_min << ":" << std::setw(2)
     << timeInfo.tm_sec;
  return ss.str();
}

std::string GetNowTimeStr() { return TimeToStr(GetLocalTime()); }

void InitTimeZone() {
  setenv("TZ", CONFIG_LOCAL_TIME_ZONE, 1);
  tzset();
}

/// Gregorian calendar to Modified Julian Date
int32_t GregToMJD(const std::tm& timeInfo) {
  const double year = timeInfo.tm_year + 1900;
  const double month = timeInfo.tm_mon + 1;
  return std::floor(365.25 * year) + std::floor(year / 400) -
         std::floor(year / 100) + std::floor(30.59 * (month - 2.0)) +
         timeInfo.tm_mday - 678912;
}

/// Get ChronoMinutes from hours and minutes.
std::chrono::minutes GetChronoHourMinutes(const std::tm& timeInfo) {
  return std::chrono::hours(timeInfo.tm_hour) +
         std::chrono::minutes(timeInfo.tm_min);
}

std::vector<std::string> SplitString(const std::string& str, const char delim) {
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
// input output_voltage[mv] top_resistance_value[kΩ], bottom_registance_value[kΩ]
// return voltage[V]
float GetVoltage() {
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  GPIO::SetLevel(CONFIG_VAOLTAGE_CHECK_OUTPUT_GPIO_NO, 1);

  static constexpr int32_t VOLTAGE_ADC_CHECK_DELAY_MILLISECOND = 100;
  Util::SleepMillisecond(VOLTAGE_ADC_CHECK_DELAY_MILLISECOND);

  static const int32_t VOLTAGE_ADC_CHECK_ROUND = 10;
  const uint32_t adc_voltage = GPIO::GetAdcVoltage(
      CONFIG_VAOLTAGE_CHECK_INPUT_ADC_CHANNEL_NO, VOLTAGE_ADC_CHECK_ROUND);

  GPIO::SetLevel(CONFIG_VAOLTAGE_CHECK_OUTPUT_GPIO_NO, 0);

  // Voltage divider rate
  static const float OHM_TO_KOHM = 1000.0f;
  static const float TOP_REGISTER =
      CONFIG_VOLTAGE_CHECK_TOP_REGISTER / OHM_TO_KOHM;  // kΩ
  static const float BOTTOM_REGISTER =
      CONFIG_VOLTAGE_CHECK_BOTTOM_REGISTER / OHM_TO_KOHM;  // kΩ
  float voltage = Util::GetOriginalVoltageFromDividerRegister(
      adc_voltage, TOP_REGISTER, BOTTOM_REGISTER);

  ESP_LOGI(TAG, "Voltage:%.2f[V] ADC Voltage:%d[mV]", voltage, adc_voltage);
  return voltage;
#else
  return 0.0f;
#endif
}

/// Get Original Voltage Divider Resistor
// input output_voltage[mv] top_resistance_value[kΩ], bottom_registance_value[kΩ]
// return voltage[V]
float GetOriginalVoltageFromDividerRegister(const uint32_t output_voltage,
                                            const float top_resistance_value,
                                            const float bottom_registance_value) {
  const float voltage_div_rate =
      bottom_registance_value / (top_resistance_value + bottom_registance_value);
  return output_voltage / voltage_div_rate / 1000.0f;
}

}  // namespace Util
}  // namespace IrrigationSystem

// EOF
