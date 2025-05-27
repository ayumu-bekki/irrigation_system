// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "voltage_check_task.h"

#include <iomanip>
#include <sstream>

#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

VoltageCheckTask::VoltageCheckTask(
    const IrrigationInterfaceWeakPtr irrigation_interface)
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      irrigation_interface_(irrigation_interface),
      voltage_(0.0f) {}

void VoltageCheckTask::Initialize() {}

void VoltageCheckTask::Update() {
  voltage_ = Util::GetVoltage();

#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  // Publish MQTT
  const IrrigationInterfaceSharedPtr irrigation_interface =
      irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }

  std::stringstream message;
  message << "{\"voltage\":" << std::setfill('0') << std::fixed
          << std::setprecision(2) << voltage_ << "}";
  irrigation_interface->PublishMQTTMessage(
      "irrigation_system/" CONFIG_MQTT_DEVICE_TOPIC_NAME "/telemetry/voltage",
      message.str().c_str());
#endif

  static const int32_t NEXT_CHECK_MILLISECOND = 60 * 60 * 1000;
  Util::SleepMillisecond(NEXT_CHECK_MILLISECOND);
}

float VoltageCheckTask::GetVoltage() const { return voltage_; }

}  // namespace IrrigationSystem

// EOF
