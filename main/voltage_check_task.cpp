// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "voltage_check_task.h"

#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

VoltageCheckTask::VoltageCheckTask()
    : Task(TASK_NAME, PRIORITY, CORE_ID), voltage_(0.0f) {}

void VoltageCheckTask::Initialize() {}

void VoltageCheckTask::Update() {
  voltage_ = Util::GetVoltage();

  static const int32_t NEXT_CHECK_MILLISECOND = 60 * 60 * 1000;
  Util::SleepMillisecond(NEXT_CHECK_MILLISECOND);
}

float VoltageCheckTask::GetVoltage() const { return voltage_; }

}  // namespace IrrigationSystem

// EOF
