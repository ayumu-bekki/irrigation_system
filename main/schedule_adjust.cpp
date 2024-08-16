// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_adjust.h"

#include "logger.h"
#include "schedule_manager.h"
#include "util.h"

namespace IrrigationSystem {

ScheduleAdjust::ScheduleAdjust() : ScheduleBase(), irrigation_interface_() {}

ScheduleAdjust::ScheduleAdjust(
    const IrrigationInterfaceWeakPtr irrigation_interface, const int hour,
    const int minute)
    : ScheduleBase(ScheduleBase::STATUS_WAIT, ScheduleAdjust::SCHEDULE_NAME,
                   hour, minute, ScheduleAdjust::IS_VISIBLE_TASK),
      irrigation_interface_(irrigation_interface) {}

void ScheduleAdjust::Exec() {
  ESP_LOGI(TAG, "Schedule Exec - Adjust Executer. %02d:%02d", GetHour(),
           GetMinute());
  SetStatus(STATUS_EXECUTED);

  const IrrigationInterfaceSharedPtr irrigation_interface =
      irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }

  // Schedule Manager
  const ScheduleManagerSharedPtr schedule_manager =
      irrigation_interface->GetScheduleManager().lock();
  if (!schedule_manager) {
    ESP_LOGE(TAG, "Failed ScheduleManager is null");
    return;
  }

  schedule_manager->AdjustSchedule();
}

}  // namespace IrrigationSystem

// EOF
