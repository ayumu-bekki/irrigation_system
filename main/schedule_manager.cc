// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_manager.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <sstream>

#include "logger.h"
#include "schedule_adjust.h"
#include "schedule_base.h"
#include "schedule_dummy.h"
#include "schedule_watering.h"
#include "util.h"
#include "watering_record.h"
#include "watering_setting.h"
#include "weather_forecast.h"

namespace IrrigationSystem {

ScheduleManager::ScheduleManager(
    const IrrigationInterfaceWeakPtr irrigation_interface)
    : irrigation_interface_(irrigation_interface),
      schedule_list_(),
      current_month_(0),
      current_day_(0) {}

void ScheduleManager::Execute() {
  // Get Current Time
  const std::tm now_time_info = Util::GetLocalTime();

  // Date changed.
  if (current_day_ != now_time_info.tm_mday) {
    InitializeNewDay(now_time_info);
  }

  // Run the schedule
  for (auto&& schedule_item : schedule_list_) {
    if (schedule_item->CanExecute(now_time_info)) {
      schedule_item->Exec();
    }
  }
}

const ScheduleManager::ScheduleBaseList& ScheduleManager::GetScheduleList()
    const {
  return schedule_list_;
}

void ScheduleManager::AdjustSchedule() {
  ESP_LOGI(TAG, "Start Schedule Adjust. %s", Util::GetNowTimeStr().c_str());

  const IrrigationInterfaceSharedPtr irrigation_interface =
      irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }

  // GetWateringSetting
  const WateringSetting& watering_setting =
      irrigation_interface->GetWateringSetting();
  if (!watering_setting.IsActive()) {
    ESP_LOGI(TAG, "Watering Setting is not activated.");
    return;
  }

  // Get TimeInfo
  const tm now_time_info = Util::GetLocalTime();

  // CreateSchedule
  if (watering_setting.GetWateringMode() ==
      WateringSetting::WATERING_MODE_SIMPLE) {
    const WateringSetting::WateringHourList& hourList =
        watering_setting.GetWateringHourList();
    for (const std::int32_t& hour : hourList) {
      AddSchedule(std::make_unique<ScheduleWatering>(
          irrigation_interface, hour, 0, watering_setting.GetWateringSec()));
    }
  } else if (watering_setting.GetWateringMode() ==
             WateringSetting::WATERING_MODE_ADVANCE) {
    // Read History
    const std::tm watering_time_info =
        Util::EpochToLocalTime(irrigation_interface->GetLastWateringEpoch());

    /// WateringWeather
    enum WateringWeather : int {
      WATERING_WEATHER_NONE,
      WATERING_WEATHER_NORMAL,
      WATERING_WEATHER_RAIN,
      MAX_WATERING_WEATHER,
    };

    // Request weather forecast
    WateringWeather watering_weather = WATERING_WEATHER_NONE;
    std::int32_t max_temperature = 0;

    WeatherForecast& weather_forecast =
        irrigation_interface->GetWeatherForecast();
    weather_forecast.SetJMAParamter(watering_setting.GetJMAAreaPathCode(),
                                    watering_setting.GetJMALocalCode(),
                                    watering_setting.GetJMAAMeDAS());
    weather_forecast.Request();
    if (weather_forecast.GetRequestStatus() == WeatherForecast::ACQUIRED) {
      watering_weather = (weather_forecast.IsRain()) ? WATERING_WEATHER_RAIN
                                                     : WATERING_WEATHER_NORMAL;
      max_temperature = weather_forecast.GetCurrentMaxTemperature();
      ESP_LOGI(TAG, "Weather OK. Weather:%s MaxTemperature:%d°C",
               WeatherForecast::WeatherCodeToStr(
                   weather_forecast.GetCurrentWeatherCode()),
               max_temperature);
    } else {
      ESP_LOGW(TAG, "Failed to get the weather forecast.");
    }

    // Match WateringType
    std::string watering_type_str;
    if (watering_weather == WATERING_WEATHER_NONE) {
      // could not Get Weather
      const int month = now_time_info.tm_mon + 1;

      // Reference from the monthly table and treat it as normal weather
      const WateringSetting::MonthToTypeDict& monthToTypeDict =
          watering_setting.GetMonthToTypeDict();
      WateringSetting::MonthToTypeDict::const_iterator iter =
          monthToTypeDict.find(std::to_string(month));
      if (iter != monthToTypeDict.end()) {
        watering_type_str = iter->second;
      }
      watering_weather = WATERING_WEATHER_NORMAL;
    } else {
      const WateringSetting::TemperatureWateringList&
          temperature_watering_list =
              watering_setting.GetTemperatureWateringList();
      for (const WateringSetting::TemperatureWatering& temperatureWatering :
           temperature_watering_list) {
        if (temperatureWatering.Temperature <= max_temperature) {
          watering_type_str = (watering_weather == WATERING_WEATHER_RAIN)
                                  ? temperatureWatering.RainType
                                  : temperatureWatering.NormalType;
        }
      }
    }

    ESP_LOGI(TAG, "Watering Type:%s watering_weather:%d",
             watering_type_str.c_str(), watering_weather);

    // WateringType To Schedule
    const WateringSetting::WateringTypeDict& watering_type_dicst =
        watering_setting.GetWateringTypeDict();
    WateringSetting::WateringTypeDict::const_iterator iter =
        watering_type_dicst.find(watering_type_str);
    if (iter != watering_type_dicst.end()) {
      const WateringSetting::WateringType& watering_type = iter->second;

      const std::int32_t last_watering_duration =
          Util::GregToMJD(now_time_info) - Util::GregToMJD(watering_time_info);
      ESP_LOGI(TAG, "Watering DaysDuration:%d", last_watering_duration);

      if (watering_type.DaySpan <= last_watering_duration) {
        for (const std::int32_t& hour : watering_type.WateringHours) {
          AddSchedule(std::make_unique<ScheduleWatering>(
              irrigation_interface, hour, 0,
              watering_setting.GetWateringSec()));
        }
      } else {
        ESP_LOGI(TAG, "Skip DaysDuration");
      }
    }
  }

#if CONFIG_DEBUG != 0
  // Register Dummy Schedule (Test)
  AddSchedule(std::make_unique<ScheduleDummy>(12, 0));
  AddSchedule(std::make_unique<ScheduleDummy>(0, 0));
#endif

  // Disable
  DisableExpiredSchedule(now_time_info);

  // Sort
  SortScheduleTime();

#if CONFIG_DEBUG != 0
  DebugOutputSchedules();
#endif

  ESP_LOGI(TAG, "Finish Schedule Adjust.");

  return;
}

int ScheduleManager::GetCurrentMonth() const { return current_month_; }

int ScheduleManager::GetCurrentDay() const { return current_day_; }

/// Date change schedule initialization
void ScheduleManager::InitializeNewDay(const std::tm& now_time_info) {
  const IrrigationInterfaceSharedPtr irrigation_interface =
      irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }

  current_month_ = now_time_info.tm_mon + 1;
  current_day_ = now_time_info.tm_mday;

  schedule_list_.clear();
  AddSchedule(std::make_unique<ScheduleAdjust>(irrigation_interface, 0, 30));

  WeatherForecast& weather_forecast =
      irrigation_interface->GetWeatherForecast();
  weather_forecast.Initialize();
}

/// Add a schedule to the list
void ScheduleManager::AddSchedule(ScheduleBaseUniquePtr&& scheduleItem) {
  schedule_list_.emplace_back(std::move(scheduleItem));
}

/// Disable a schedule whose execution time has already expired.
void ScheduleManager::DisableExpiredSchedule(const std::tm& time_info) {
  for (auto&& schedule_item : schedule_list_) {
    schedule_item->DisableExpired(time_info);
  }
}

/// Sort the schedule in ascending order
void ScheduleManager::SortScheduleTime() {
  std::sort(schedule_list_.begin(), schedule_list_.end(),
            [](const ScheduleBaseUniquePtr& left,
               const ScheduleBaseUniquePtr& right) {
              return left->GetDiffTime() < right->GetDiffTime();
            });
}

#if CONFIG_DEBUG != 0
void ScheduleManager::DebugOutputSchedules() {
  for (const auto& schedule_item : schedule_list_) {
    ESP_LOGD(TAG, "Test Schedule Item %02d:%02d %s", schedule_item->GetHour(),
             schedule_item->GetMinute(), schedule_item->GetName().c_str());
  }
}
#endif  // CONFIG_DEBUG

}  // namespace IrrigationSystem

// EOF
