// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_adjust.h"

#include <esp_log.h>

#include "define.h"
#include "util.h"
#include "schedule_manager.h"

namespace IrrigationSystem {

ScheduleAdjust::ScheduleAdjust()
    :ScheduleBase()
    ,m_pIrricationInterface(nullptr)
{}

ScheduleAdjust::ScheduleAdjust(IrrigationInterface *const pIrricationInterface, const int hour, const int minute)
    :ScheduleBase(ScheduleBase::STATUS_WAIT, ScheduleAdjust::SCHEDULE_NAME, hour, minute, ScheduleAdjust::IS_VISIBLE_TASK)
    ,m_pIrricationInterface(pIrricationInterface)
{}

void ScheduleAdjust::Exec()
{
    ESP_LOGI(TAG, "Schedule Exec - Adjust Executer. %02d:%02d", GetHour(), GetMinute());
    SetStatus(STATUS_EXECUTED);

    if (!m_pIrricationInterface) {
        ESP_LOGW(TAG, "Failed IrricationInterface is null");
        return;
    }
 
    ScheduleManager& scheduleManager = m_pIrricationInterface->GetScheduleManager();
    scheduleManager.AdjustSchedule();
}

} // IrrigationSystem

// EOF
