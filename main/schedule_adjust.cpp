// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_adjust.h"

#include "logger.h"
#include "util.h"
#include "schedule_manager.h"


namespace IrrigationSystem {

ScheduleAdjust::ScheduleAdjust()
    :ScheduleBase()
    ,m_pIrrigationInterface(nullptr)
{}

ScheduleAdjust::ScheduleAdjust(IrrigationInterface *const pIrrigationInterface, const int hour, const int minute)
    :ScheduleBase(ScheduleBase::STATUS_WAIT, ScheduleAdjust::SCHEDULE_NAME, hour, minute, ScheduleAdjust::IS_VISIBLE_TASK)
    ,m_pIrrigationInterface(pIrrigationInterface)
{}

void ScheduleAdjust::Exec()
{
    ESP_LOGI(TAG, "Schedule Exec - Adjust Executer. %02d:%02d", GetHour(), GetMinute());
    SetStatus(STATUS_EXECUTED);

    if (!m_pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }
 
    ScheduleManager& scheduleManager = m_pIrrigationInterface->GetScheduleManager();
    scheduleManager.AdjustSchedule();
}

} // IrrigationSystem

// EOF
