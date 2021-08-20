// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_watering.h"

#include "logger.h"


namespace IrrigationSystem {

ScheduleWatering::ScheduleWatering()
    :ScheduleBase()
    ,m_pIrricationInterface(nullptr)
    ,m_OpenSecond(0)
{}

ScheduleWatering::ScheduleWatering(IrrigationInterface *const pIrricationInterface, const int hour, const int minute, const int openSecond)
    :ScheduleBase(ScheduleBase::STATUS_WAIT, ScheduleWatering::SCHEDULE_NAME, hour, minute, ScheduleWatering::IS_VISIBLE_TASK)
    ,m_pIrricationInterface(pIrricationInterface)
    ,m_OpenSecond(openSecond)
{}

void ScheduleWatering::Exec()
{
    ESP_LOGI(TAG, "Schedule Exec - Watering Executer. %02d:%02d WS:%d", GetHour(), GetMinute(), m_OpenSecond);
    SetStatus(STATUS_EXECUTED);

    if (!m_pIrricationInterface) {
        ESP_LOGE(TAG, "Failed IrricationInterface is null");
        return;
    }
 
    m_pIrricationInterface->RelayAddOpenSecond(m_OpenSecond);
}

} // IrrigationSystem

// EOF