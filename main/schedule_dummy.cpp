// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_dummy.h"

#include <esp_log.h>

#include "define.h"
#include "util.h"

namespace IrrigationSystem {

ScheduleDummy::ScheduleDummy()
    :ScheduleBase()
{}

ScheduleDummy::ScheduleDummy(const int hour, const int minute)
    :ScheduleBase(ScheduleBase::STATUS_WAIT, ScheduleDummy::SCHEDULE_NAME, hour, minute, ScheduleDummy::IS_VISIBLE_TASK)
{}

void ScheduleDummy::Exec()
{
    ESP_LOGI(TAG, "Schedule Exec - Dummy Executer. %02d:%02d", GetHour(), GetMinute());
    SetStatus(STATUS_EXECUTED);
}

} // IrrigationSystem

// EOF
