// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_watering.h"
#include "watering_record.h"

#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

ScheduleWatering::ScheduleWatering()
    :ScheduleBase()
    ,m_pIrrigationInterface()
    ,m_OpenSecond(0)
{}

ScheduleWatering::ScheduleWatering(const IrrigationInterfaceWeakPtr pIrrigationInterface, const int hour, const int minute, const int openSecond)
    :ScheduleBase(ScheduleBase::STATUS_WAIT, ScheduleWatering::SCHEDULE_NAME, hour, minute, ScheduleWatering::IS_VISIBLE_TASK)
    ,m_pIrrigationInterface(pIrrigationInterface)
    ,m_OpenSecond(openSecond)
{}

void ScheduleWatering::Exec()
{
    ESP_LOGI(TAG, "Schedule Exec - Watering Executer. %02d:%02d WS:%d", GetHour(), GetMinute(), m_OpenSecond);
    SetStatus(STATUS_EXECUTED);

    const IrrigationInterfaceSharedPtr irrigationInterface = m_pIrrigationInterface.lock();
    if (!irrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }
 
    irrigationInterface->ValveAddOpenSecond(m_OpenSecond);

    // Write History
    irrigationInterface->SaveLastWateringEpoch(Util::GetEpoch());
}

} // IrrigationSystem

// EOF
