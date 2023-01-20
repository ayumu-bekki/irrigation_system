// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_manager.h"

#include <algorithm>
#include <limits>
#include <chrono>
#include <sstream>

#include "logger.h"
#include "util.h"
#include "weather_forecast.h"
#include "schedule_base.h"
#include "schedule_dummy.h"
#include "schedule_adjust.h"
#include "schedule_watering.h"
#include "watering_record.h"
#include "watering_setting.h"


namespace IrrigationSystem {

ScheduleManager::ScheduleManager(const IrrigationInterfaceWeakPtr pIrrigationInterface)
    :m_pIrrigationInterface(pIrrigationInterface)
    ,m_ScheduleList()
    ,m_CurrentMonth(0)
    ,m_CurrentDay(0)
{}

void ScheduleManager::Execute()
{
    // Get Current Time   
    const std::tm nowTimeInfo = Util::GetLocalTime();
    
    // Date changed.
    if (m_CurrentDay != nowTimeInfo.tm_mday) {
        InitializeNewDay(nowTimeInfo);
    }

    // Run the schedule 
    for (auto&& pScheduleItem : m_ScheduleList) {
        if (pScheduleItem->CanExecute(nowTimeInfo)) {
            pScheduleItem->Exec();
        }
    }
}

const ScheduleManager::ScheduleBaseList& ScheduleManager::GetScheduleList() const
{
    return m_ScheduleList;
}

void ScheduleManager::AdjustSchedule()
{
    ESP_LOGI(TAG, "Start Schedule Adjust. %s", Util::GetNowTimeStr().c_str());

    const IrrigationInterfaceSharedPtr irrigationInterface = m_pIrrigationInterface.lock();
    if (!irrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    // GetWateringSetting
    const WateringSetting& wateringSetting = irrigationInterface->GetWateringSetting();
    if (!wateringSetting.IsActive()) {
        ESP_LOGI(TAG, "Watering Setting is not activated.");
        return;
    }

 
    // Get TimeInfo
    const tm nowTimeInfo = Util::GetLocalTime();
    
    // CreateSchedule
    if (wateringSetting.GetWateringMode() == WateringSetting::WATERING_MODE_SIMPLE) {
        const WateringSetting::WateringHourList& hourList = wateringSetting.GetWateringHourList();
        for (const std::int32_t& hour : hourList) {
            AddSchedule(
                std::make_unique<ScheduleWatering>(irrigationInterface, hour, 0, wateringSetting.GetWateringSec())
            );
        }
    } else if (wateringSetting.GetWateringMode() == WateringSetting::WATERING_MODE_ADVANCE) {
        // Read History 
        const std::tm wateringTm = Util::EpochToLocalTime(irrigationInterface->GetLastWateringEpoch());

        /// WateringWeather
        enum WateringWeather : int {
            WATERING_WEATHER_NONE,
            WATERING_WEATHER_NORMAL,
            WATERING_WEATHER_RAIN,
            MAX_WATERING_WEATHER,
        };

        // Request weather forecast
        WateringWeather wateringWeather = WATERING_WEATHER_NONE;
        std::int32_t maxTemperature = 0;

        WeatherForecast &weatherForecast = irrigationInterface->GetWeatherForecast();
        weatherForecast.SetJMAParamter(wateringSetting.GetJMAAreaPathCode(), wateringSetting.GetJMALocalCode(), wateringSetting.GetJMAAMeDAS());
        weatherForecast.Request();
        if (weatherForecast.GetRequestStatus() == WeatherForecast::ACQUIRED) {   
            wateringWeather = (weatherForecast.IsRain()) ? WATERING_WEATHER_RAIN : WATERING_WEATHER_NORMAL;
            maxTemperature = weatherForecast.GetCurrentMaxTemperature();
            ESP_LOGI(TAG, "Weather OK. Weather:%s MaxTemperature:%d°C", WeatherForecast::WeatherCodeToStr(weatherForecast.GetCurrentWeatherCode()), maxTemperature);
        } else {
            ESP_LOGW(TAG, "Failed to get the weather forecast.");
        }

        // Match WateringType   
        std::string wateringTypeStr;
        if (wateringWeather == WATERING_WEATHER_NONE) {
            // could not Get Weather
            const int month = nowTimeInfo.tm_mon + 1;
    
            // Reference from the monthly table and treat it as normal weather
            const WateringSetting::MonthToTypeDict& monthToTypeDict = wateringSetting.GetMonthToTypeDict();
            WateringSetting::MonthToTypeDict::const_iterator iter = monthToTypeDict.find(std::to_string(month));
            if (iter != monthToTypeDict.end()) {
                wateringTypeStr = iter->second;
            }
            wateringWeather = WATERING_WEATHER_NORMAL;
        } else {
            const WateringSetting::TemperatureWateringList& temperatureWateringList = wateringSetting.GetTemperatureWateringList();
            for (const WateringSetting::TemperatureWatering& temperatureWatering : temperatureWateringList) {
                if (temperatureWatering.Temperature <= maxTemperature) {
                    wateringTypeStr = (wateringWeather == WATERING_WEATHER_RAIN) ?
                       temperatureWatering.RainType : temperatureWatering.NormalType;
                }
            }
        }

        ESP_LOGI(TAG, "Watering Type:%s wateringWeather:%d", wateringTypeStr.c_str(), wateringWeather);
   
        // WateringType To Schedule
        const WateringSetting::WateringTypeDict& wateringTypeDict = wateringSetting.GetWateringTypeDict();
        WateringSetting::WateringTypeDict::const_iterator iter = wateringTypeDict.find(wateringTypeStr);
        if (iter != wateringTypeDict.end()) {
            const WateringSetting::WateringType& wateringType = iter->second;

            const std::int32_t lastWateringDuration = Util::GregToMJD(nowTimeInfo) - Util::GregToMJD(wateringTm);
            ESP_LOGI(TAG, "Watering DaysDuration:%d", lastWateringDuration);

            if (wateringType.DaySpan <= lastWateringDuration) {
                for (const std::int32_t& hour : wateringType.WateringHours) {
                    AddSchedule(
                        std::make_unique<ScheduleWatering>(irrigationInterface, hour, 0, wateringSetting.GetWateringSec())
                    );
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
    DisableExpiredSchedule(nowTimeInfo);

    // Sort
    SortScheduleTime();

#if CONFIG_DEBUG != 0
    DebugOutputSchedules();
#endif

    ESP_LOGI(TAG, "Finish Schedule Adjust.");
    
    return;
}

int ScheduleManager::GetCurrentMonth() const
{   
    return m_CurrentMonth;
}

int ScheduleManager::GetCurrentDay() const
{
    return m_CurrentDay;
}

/// Date change schedule initialization
void ScheduleManager::InitializeNewDay(const std::tm& nowTimeInfo)
{
    const IrrigationInterfaceSharedPtr irrigationInterface = m_pIrrigationInterface.lock();
    if (!irrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    m_CurrentMonth = nowTimeInfo.tm_mon + 1;
    m_CurrentDay = nowTimeInfo.tm_mday;

    m_ScheduleList.clear();
    AddSchedule(std::make_unique<ScheduleAdjust>(irrigationInterface, 0, 30));

    WeatherForecast &weatherForecast = irrigationInterface->GetWeatherForecast();
    weatherForecast.Initialize();
}

/// Add a schedule to the list
void ScheduleManager::AddSchedule(ScheduleBaseUniquePtr&& scheduleItem)
{
    m_ScheduleList.emplace_back(std::move(scheduleItem));
}


/// Disable a schedule whose execution time has already expired.
void ScheduleManager::DisableExpiredSchedule(const std::tm& timeInfo)
{
    for (auto&& pScheduleItem : m_ScheduleList) {
        pScheduleItem->DisableExpired(timeInfo);
    }
}

/// Sort the schedule in ascending order
void ScheduleManager::SortScheduleTime()
{
    std::sort(  
        m_ScheduleList.begin(), 
        m_ScheduleList.end(),
        [](const ScheduleBaseUniquePtr& left, const ScheduleBaseUniquePtr& right){
            return left->GetDiffTime() < right->GetDiffTime();
        }
    );
}

#if CONFIG_DEBUG != 0
void ScheduleManager::DebugOutputSchedules()
{
    for (const auto& pScheduleItem : m_ScheduleList) {
        ESP_LOGD(TAG, "Test Schedule Item %02d:%02d %s", pScheduleItem->GetHour(), pScheduleItem->GetMinute(), pScheduleItem->GetName().c_str());
    }
}
#endif // CONFIG_DEBUG

} // IrrigationSystem

// EOF
