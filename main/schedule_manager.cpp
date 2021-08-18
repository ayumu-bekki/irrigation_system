// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_manager.h"

#include <esp_log.h>
#include <algorithm>

#include "util.h"
#include "define.h"

#include "schedule_base.h"
#include "schedule_dummy.h"
#include "schedule_adjust.h"
#include "schedule_watering.h"

namespace IrrigationSystem {

ScheduleManager::ScheduleManager(IrrigationInterface *const pIrricationInterface)
    :m_pIrricationInterface(pIrricationInterface)
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

    // By month, how many days apart to water.
    static constexpr int MONTH_TO_SPAN_MOD_DAY[12] = 
    {
        7, 7, 7, 2, 2, 2, 1, 1, 1, 2, 2, 7
    };
    // Watering Time (sec)
    static constexpr int WATERING_SEC = 50;
    // Time of day for watering, by month.
    static constexpr int MONTH_TO_WATERING_HOUR[12][2] = {
        {9,-1},
        {9,-1},
        {9,-1},
        {7,-1},
        {7,-1},
        {7,-1},
        {7,16},
        {7,16},
        {7,16},
        {7,-1},
        {7,-1},
        {9,-1},
    };

    // Get Modified Julian Date. For Day Span
    const tm nowTimeInfo = Util::GetLocalTime();
    const int modifiedJulianDateNo = Util::GregToMJD(nowTimeInfo);

    // Register Dummy Schedule (Test)
    AddSchedule(ScheduleBase::UniquePtr(new ScheduleDummy(0, 0)));

    // month check
    const int month = nowTimeInfo.tm_mon;
    if (month < 0 || 12 <= month) {
        ESP_LOGW(TAG, "Invalid Month Num > %d", month);
        return;
    }
    
    // Check Span
    if ((modifiedJulianDateNo % MONTH_TO_SPAN_MOD_DAY[month]) != 0) {
        ESP_LOGI(TAG, "Finish Schedule Adjust. Skipp watering today.");
        return;
    }

    // Register
    for (int i = 0; i < 2; ++i) {
        const int hour = MONTH_TO_WATERING_HOUR[month][i];
        if (0 <= hour &&
           nowTimeInfo.tm_hour < hour) {
            AddSchedule(ScheduleBase::UniquePtr(new ScheduleWatering(m_pIrricationInterface, hour, 0, WATERING_SEC)));
        }
    }
    
    // Sort
    SortScheduleTime();

    ESP_LOGI(TAG, "Finish Schedule Adjust.");

    // Debug
    DebugOutputSchedules();
}

int ScheduleManager::GetCurrentMonth() const
{   
    return m_CurrentMonth;
}

int ScheduleManager::GetCurrentDay() const
{
    return m_CurrentDay;
}

void ScheduleManager::InitializeNewDay(const std::tm& nowTimeInfo)
{
    m_CurrentMonth = nowTimeInfo.tm_mon + 1;
    m_CurrentDay = nowTimeInfo.tm_mday;

    m_ScheduleList.clear();
    AddSchedule(ScheduleBase::UniquePtr(new ScheduleAdjust(m_pIrricationInterface, 0, 30)));
}

void ScheduleManager::AddSchedule(ScheduleBase::UniquePtr&& scheduleItem)
{
    m_ScheduleList.emplace_back(std::move(scheduleItem));
}

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

void ScheduleManager::DebugOutputSchedules()
{
    for (const auto& pScheduleItem : m_ScheduleList) {
        ESP_LOGI(TAG, "Test Schedule Item %02d:%02d %s", pScheduleItem->GetHour(), pScheduleItem->GetMinute(), pScheduleItem->GetName().c_str());
    }
}

} // IrrigationSystem

// EOF
