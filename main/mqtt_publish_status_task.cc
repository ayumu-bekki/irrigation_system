// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "mqtt_publish_status_task.h"

#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

MQTTPublishStatusTask::MQTTPublishStatusTask(
    const IrrigationInterfaceWeakPtr irrigation_interface)
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      irrigation_interface_(irrigation_interface) {}

void MQTTPublishStatusTask::Initialize() {}

void MQTTPublishStatusTask::Update() {
  // Publish MQTT
  const IrrigationInterfaceSharedPtr irrigation_interface =
      irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }

  irrigation_interface->PublishMQTTMessage(
      "irrigation_system/" CONFIG_MQTT_DEVICE_TOPIC_NAME "/status",
      "{\"status\":\"ok\"}");

  static const int32_t NEXT_PUBLISH_MILLISECOND = 1000 * 60 * 10;
  Util::SleepMillisecond(NEXT_PUBLISH_MILLISECOND);
}

}  // namespace IrrigationSystem

// EOF
