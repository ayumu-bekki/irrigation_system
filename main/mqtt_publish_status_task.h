#ifndef MQTT_PUBLISH_STATUS_TASK_H_
#define MQTT_PUBLISH_STATUS_TASK_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include "irrigation_interface.h"
#include "task.h"

namespace IrrigationSystem {

class MQTTPublishStatusTask final : public Task {
 public:
  static constexpr char *const TASK_NAME = (char *)"MQTTPublishStatusTask";
  static constexpr int PRIORITY = Task::PRIORITY_LOW;
  static constexpr int CORE_ID = APP_CPU_NUM;

 public:
  MQTTPublishStatusTask(const IrrigationInterfaceWeakPtr irrigation_interface);

  void Initialize() override;

  void Update() override;

 private:
  const IrrigationInterfaceWeakPtr irrigation_interface_;
};

using MQTTPublishStatusTaskUniquePtr = std::unique_ptr<MQTTPublishStatusTask>;

}  // namespace IrrigationSystem

#endif  // MQTT_PUBLISH_STATUS_TASK_H_
// EOF
