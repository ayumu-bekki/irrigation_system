// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "management_task.h"

#include "logger.h"
#include "http_request.h"
#include "irrigation_controller.h"
#include "schedule_manager.h"
#include "util.h"

namespace IrrigationSystem {

ManagementTask::ManagementTask(
    const IrrigationInterfaceWeakPtr irrigation_interface)
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      irrigation_interface_(irrigation_interface) {}

void ManagementTask::Update() {
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
  schedule_manager->Execute();

  Util::SleepMillisecond(1000);
}

}  // namespace IrrigationSystem

// EOF
