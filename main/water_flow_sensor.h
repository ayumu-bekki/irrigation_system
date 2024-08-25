#ifndef WATER_FLOW_SENSOR_H_
#define WATER_FLOW_SENSOR_H_
// ESP32 Irrigation system
// (C)2024 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include <cstdint>

#include "driver/pulse_cnt.h"
#include "gptimer.h"
#include "message_queue.h"
#include "task.h"

namespace IrrigationSystem {

class WaterFlowSensor final : public Task {
 public:
  static constexpr char *const TASK_NAME = (char *)"WaterFlowSensorTask";
  static constexpr int PRIORITY = Task::PRIORITY_LOW;
  static constexpr int CORE_ID = APP_CPU_NUM;

 public:
  WaterFlowSensor();

  void Initialize() override;
  void Update() override;

  void StartMeasurement();
  int32_t FinishMeasurement();
  int32_t GetSensorHz() const;

  static bool TimerCallback(gptimer_handle_t timer,
                            const gptimer_alarm_event_data_t *edata,
                            void *user_data);

  static float CountToCubicDecimeters(const int32_t count);

 private:
  GPTimer gptimer_;
  MessageQueue<int32_t> queue_;
  pcnt_unit_handle_t pcnt_unit_;
  int32_t sensor_hz_;
  int32_t total_count_;
};

}  // namespace IrrigationSystem

#endif  // WATER_FLOW_SENSOR_H_
// EOF
