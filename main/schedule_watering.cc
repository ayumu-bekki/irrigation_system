// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_watering.h"

#include "logger.h"
#include "util.h"
#include "watering_record.h"

namespace IrrigationSystem {

ScheduleWatering::ScheduleWatering()
    : ScheduleBase(), irrigation_interface_(), open_seconds_(0) {}

ScheduleWatering::ScheduleWatering(
    const IrrigationInterfaceWeakPtr irrigation_interface, const int hour,
    const int minute, const int open_seconds)
    : ScheduleBase(ScheduleBase::STATUS_WAIT, ScheduleWatering::SCHEDULE_NAME,
                   hour, minute, ScheduleWatering::IS_VISIBLE_TASK),
      irrigation_interface_(irrigation_interface),
      open_seconds_(open_seconds) {}

void ScheduleWatering::Exec() {
  ESP_LOGI(TAG, "Schedule Exec - Watering Executer. %02d:%02d WS:%d", GetHour(),
           GetMinute(), open_seconds_);
  SetStatus(STATUS_EXECUTED);

  const IrrigationInterfaceSharedPtr irrigation_interface =
      irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }
  
  valve_executor_ = std::make_shared<ValveExecutor>();
  valve_executor_->SetOpenSeconds(open_seconds_);
  valve_executor_->SetStatus(ValveExecutor::ExecutorStatus::EXECUTOR_SCHEDULE);
  irrigation_interface->AddValveExecutor(valve_executor_);

  // Write History
  irrigation_interface->SaveLastWateringEpoch(Util::GetEpoch());
}

int32_t ScheduleWatering::GetWaterFlow() const 
{
  if (!valve_executor_) {
    return -1;
  } 
  return valve_executor_->GetWaterAmount();
}

}  // namespace IrrigationSystem

// EOF
