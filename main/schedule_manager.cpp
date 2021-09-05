// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_manager.h"

#include <algorithm>
#include <limits>
#include <chrono>

#include "logger.h"
#include "util.h"
#include "weather_forecast.h"
#include "schedule_base.h"
#include "schedule_dummy.h"
#include "schedule_adjust.h"
#include "schedule_watering.h"


namespace IrrigationSystem {

ScheduleManager::ScheduleManager(IrrigationInterface *const pIrrigationInterface)
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

    /// WateringType
    enum WateringType : int { 
        WATERING_TYPE_NONE,
        WATERING_TYPE_COLD,
        WATERING_TYPE_WARM,
        WATERING_TYPE_HOT,
        MAX_WATERING_TYPE,
    };
    
    /// WateringWeather
    enum WateringWeather : int {
        WATERING_WEATHER_NORMAL,
        WATERING_WEATHER_RAIN,
        MAX_WATERING_WEATHER,
    };

    // By month, watering type (Use when weather information is not available.)
    static constexpr int MONTH_MAX = 12;
    static constexpr WateringType MONTH_TO_WATERING_TYPE[MONTH_MAX] = {
        WATERING_TYPE_COLD,
        WATERING_TYPE_COLD,
        WATERING_TYPE_COLD,
        WATERING_TYPE_WARM,
        WATERING_TYPE_WARM,
        WATERING_TYPE_WARM,
        WATERING_TYPE_HOT,
        WATERING_TYPE_HOT,
        WATERING_TYPE_HOT,
        WATERING_TYPE_WARM,
        WATERING_TYPE_WARM,
        WATERING_TYPE_COLD,
    };
    
    // Temperature to Watering Type (Scan from the smaller side.) [Celsius]
    struct TemperatureWatering {
        int moreThanTemperature;
        WateringType wateringType;
    };
    static constexpr size_t TEMPERATION_TO_WATERING_TYPE_LENGTH = 3;
    static constexpr TemperatureWatering TEMPERATURE_TO_WATERING_TYPE[TEMPERATION_TO_WATERING_TYPE_LENGTH] = {
        // WATERING_TYPE_NONE
        { 8, WATERING_TYPE_COLD },  // More than 4 WATERING_TYPE_COLD
        { 16, WATERING_TYPE_WARM }, // More than 16 WATERING_TYPE_WARM
        { 24, WATERING_TYPE_HOT},   // More than 24 WATERING_TYPE_HOT
    };

    // How many days apart to water.
    static constexpr int WATERING_TYPE_TO_SPAN_MOD_DAY[MAX_WATERING_TYPE] = {
        1, // WATERING_TYPE_NONE
        7, // WATERING_TYPE_COLD
        2, // WATERING_TYPE_WARM
        1, // WATERING_TYPE_HOT
    };

    // How many days apart to water.
    static constexpr int WATERING_TYPE_TO_WATERING_SEC[MAX_WATERING_TYPE] = {
        0,  // WATERING_TYPE_NONE
        50, // WATERING_TYPE_COLD
        50, // WATERING_TYPE_WARM
        70, // WATERING_TYPE_HOT
    };
 
    // Time of day for watering
    static constexpr int WATERING_HOUR_MAX = 2;
    static constexpr int NOT_WATERING = -1;
    static constexpr int WATERING_TYPE_TO_WATERING_HOUR[MAX_WATERING_TYPE][MAX_WATERING_WEATHER][WATERING_HOUR_MAX] = {
        {{NOT_WATERING, NOT_WATERING}, {NOT_WATERING, NOT_WATERING}},   // WATERING_TYPE_NONE
        {{9, NOT_WATERING}, {NOT_WATERING, NOT_WATERING}},              // WATERING_TYPE_COLD
        {{7, NOT_WATERING}, {NOT_WATERING, NOT_WATERING}},              // WATERING_TYPE_WARM
        {{7, 16}, {7, NOT_WATERING}},                                   // WATERING_TYPE_HOT
    };

    // Check Irrigation
    if (!m_pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    // Get Modified Julian Date. For Day Span
    const tm nowTimeInfo = Util::GetLocalTime();
    const int modifiedJulianDateNo = Util::GregToMJD(nowTimeInfo);

    // Select watering type from weather forecast.
    WateringType wateringType = WATERING_TYPE_NONE;
    WateringWeather wateringWeather = WATERING_WEATHER_NORMAL;
    
    // Request weather forecast
    WeatherForecast &weatherForecast = m_pIrrigationInterface->GetWeatherForecast();
    weatherForecast.Request();
    if (weatherForecast.GetRequestStatus() == WeatherForecast::ACQUIRED) {   
        const int maxTemperature = weatherForecast.GetCurrentMaxTemperature();
        ESP_LOGI(TAG, "Weather OK. Weather:%s MaxTemperature:%d°C", WeatherForecast::WeatherCodeToStr(weatherForecast.GetCurrentWeatherCode()), maxTemperature);
        for (int tempIdx = 0; tempIdx < TEMPERATION_TO_WATERING_TYPE_LENGTH; ++tempIdx) {
            if (TEMPERATURE_TO_WATERING_TYPE[tempIdx].moreThanTemperature <= maxTemperature) {
                wateringType = TEMPERATURE_TO_WATERING_TYPE[tempIdx].wateringType;
            }
        }
        if (weatherForecast.IsRain()) {
            wateringWeather = WATERING_WEATHER_RAIN;
        }
    } else {
        ESP_LOGW(TAG, "Failed to get the weather forecast.");
        const int month = nowTimeInfo.tm_mon;
        if (month < 0 || MONTH_MAX <= month) {
            ESP_LOGE(TAG, "Invalid Month Num > %d", month);
            return;
        }
        wateringType = MONTH_TO_WATERING_TYPE[month];
    }
    ESP_LOGI(TAG, "Watering Type:%d wateringWeather:%d", wateringType, wateringWeather);

    // Register Dummy Schedule (Test)
#if CONFIG_DEBUG != 0
    AddSchedule(ScheduleBase::UniquePtr(new ScheduleDummy(12, 0)));
    AddSchedule(ScheduleBase::UniquePtr(new ScheduleDummy(0, 0)));
#endif

    // Check Span
    if ((modifiedJulianDateNo % WATERING_TYPE_TO_SPAN_MOD_DAY[wateringType]) != 0) {
        ESP_LOGI(TAG, "Finish Schedule Adjust. Skipp watering today.");
        return;
    }

    // Register
    for (int hourIndex = 0; hourIndex < WATERING_HOUR_MAX; ++hourIndex) {
        const int hour = WATERING_TYPE_TO_WATERING_HOUR[wateringType][wateringWeather][hourIndex];
        const int wateringSec = WATERING_TYPE_TO_WATERING_SEC[wateringType];
        if (hour != NOT_WATERING) {
            AddSchedule(ScheduleBase::UniquePtr(new ScheduleWatering(m_pIrrigationInterface, hour, 0, wateringSec)));
        }
    }

    // Disable 
    DisableExpiredSchedule(nowTimeInfo);

    // Sort
    SortScheduleTime();

#if CONFIG_DEBUG != 0
    DebugOutputSchedules();
#endif

    ESP_LOGI(TAG, "Finish Schedule Adjust.");
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
    if (!m_pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    m_CurrentMonth = nowTimeInfo.tm_mon + 1;
    m_CurrentDay = nowTimeInfo.tm_mday;

    m_ScheduleList.clear();
    AddSchedule(ScheduleBase::UniquePtr(new ScheduleAdjust(m_pIrrigationInterface, 0, 30)));

    WeatherForecast &weatherForecast = m_pIrrigationInterface->GetWeatherForecast();
    weatherForecast.Reset();
}

/// Add a schedule to the list
void ScheduleManager::AddSchedule(ScheduleBase::UniquePtr&& scheduleItem)
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
        [](const ScheduleBase::UniquePtr& left, const ScheduleBase::UniquePtr& right){
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
