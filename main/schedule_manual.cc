// ESP32 Irrigation system
// (C)2024 bekki.jp

// Include ----------------------
#include "schedule_manual.h"

#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

ScheduleManual::ScheduleManual() : ScheduleBase() {}

ScheduleManual::ScheduleManual(const int hour, const int minute,
                               ValveExecutorSharedPtr&& valve_executor)
    : ScheduleBase(ScheduleBase::STATUS_MANUAL, ScheduleManual::SCHEDULE_NAME,
                   hour, minute, ScheduleManual::IS_VISIBLE_TASK),
      valve_executor_(std::move(valve_executor)) {}

void ScheduleManual::Exec() {
  ESP_LOGI(TAG, "Schedule Exec - Manual Executer. %02d:%02d", GetHour(),
           GetMinute());
}

int32_t ScheduleManual::GetWaterFlow() const {
  if (!valve_executor_) {
    return -1;
  }
  return valve_executor_->GetWaterAmount();
}

}  // namespace IrrigationSystem

// EOF
