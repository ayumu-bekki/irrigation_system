// ESP32 Irrigation system
// (C)2024 bekki.jp

// Include ----------------------
#include "schedule_manual.h"

#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

ScheduleManual::ScheduleManual() : ScheduleBase() {}

ScheduleManual::ScheduleManual(const int hour, const int minute, const int32_t water_amount)
    : ScheduleBase(ScheduleBase::STATUS_MANUAL, ScheduleManual::SCHEDULE_NAME,
                   hour, minute, ScheduleManual::IS_VISIBLE_TASK),
      water_amount_(water_amount) {}

void ScheduleManual::Exec() {
  ESP_LOGI(TAG, "Schedule Exec - Manual Executer. %02d:%02d", GetHour(),
           GetMinute());
}

int32_t ScheduleManual::GetWaterFlow() const 
{
  return water_amount_;
}

}  // namespace IrrigationSystem

// EOF
