#ifndef WATER_LEVEL_CHECKER_H_
#define WATER_LEVEL_CHECKER_H_
// ESP32 Irrigation system
// (C)2023 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include <chrono>

#include "irrigation_interface.h"
#include "pwm.h"
#include "task.h"

namespace IrrigationSystem {

class WaterLevelCheckTask final : public Task {
 public:
  static constexpr char *const TASK_NAME = (char *)"WaterLevelCheckTask";
  static constexpr int PRIORITY = Task::PRIORITY_LOW;
  static constexpr int CORE_ID = APP_CPU_NUM;

 public:
  WaterLevelCheckTask(const IrrigationInterfaceWeakPtr irrigation_interface);

  void Initialize() override;
  void Update() override;

  void Check();

  float GetWaterLevel() const;

 private:
  const IrrigationInterfaceWeakPtr irrigation_interface_;
  std::time_t check_sec_;
  float water_level_;
  Pwm pwm_;
};

using WaterLevelCheckTaskUniquePtr = std::unique_ptr<WaterLevelCheckTask>;

}  // namespace IrrigationSystem

#endif  // WATER_LEVEL_CHECKER_H_
// EOF
