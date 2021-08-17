// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_manager.h"

#include <esp_log.h>

#include "schedule_base.h"
#include "util.h"
#include "define.h"

namespace IrrigationSystem {

ScheduleManager::ScheduleManager()
    :m_CurrentMonthDay(0)
    ,m_ScheduleList()
{}

void ScheduleManager::Build(const tm& nowTimeInfo)
{
    m_ScheduleList.clear();

    m_ScheduleList.push_back(ScheduleBase(ScheduleBase::STATUS_WAIT, "Check Weather", 0, 30, true));
    m_ScheduleList.push_back(ScheduleBase(ScheduleBase::STATUS_WAIT, "Sync Time", 0, 45, false));
    m_ScheduleList.push_back(ScheduleBase(ScheduleBase::STATUS_DISABLE, "Watering", 06, 00, true));
    m_ScheduleList.push_back(ScheduleBase(ScheduleBase::STATUS_WAIT, "Watering", 13, 00, true));

    /*
    // HttpRequest Test
    HttpRequest httpRequest;
    httpRequest.Request("http://example.net/");
    if (httpRequest.GetStatus() == HttpRequest::STATUS_OK) {
        ESP_LOGI(TAG, "Request OK");
    } else if (httpRequest.GetStatus() == HttpRequest::STATUS_NG) {
        ESP_LOGI(TAG, "Request NG");
    }

    m_pIrricationInterface->RequestRelayOpen(5);
    */
}

void ScheduleManager::Execute()
{
    // Get the current time
    const tm nowTimeInfo = Util::GetLocalTime();
    
    // Build a schedule when the day changes
    if (m_CurrentMonthDay != nowTimeInfo.tm_mday) {
        m_CurrentMonthDay = nowTimeInfo.tm_mday;
           
        Build(nowTimeInfo);
    }

    // Run the schedule
    for (ScheduleBase& scheduleItem : m_ScheduleList) {
        if (scheduleItem.CanExecute(nowTimeInfo)) {
            scheduleItem.Exec();
        }
    }
}

const ScheduleManager::ScheduleBaseList& ScheduleManager::GetScheduleList() const
{
    return m_ScheduleList;
}

} // IrrigationSystem

// EOF
