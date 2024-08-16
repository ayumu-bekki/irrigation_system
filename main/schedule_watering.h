#ifndef SCHEDULE_WATERING_H_
#define SCHEDULE_WATERING_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_interface.h"
#include "schedule_base.h"
#include "valve_executor.h"

namespace IrrigationSystem {

class ScheduleWatering final : public ScheduleBase {
 public:
  static constexpr char* SCHEDULE_NAME = (char*)"Watering";
  static constexpr bool IS_VISIBLE_TASK = true;

 private:
  ScheduleWatering();

 public:
  ScheduleWatering(const IrrigationInterfaceWeakPtr irrigation_interface,
                   const int hour, const int minute, const int open_seconds);

  void Exec() override;

  int32_t GetWaterFlow() const override;

 private:
  const IrrigationInterfaceWeakPtr irrigation_interface_;
  int open_seconds_;
  ValveExecutorSharedPtr valve_executor_;
};

}  // namespace IrrigationSystem

#endif  // SCHEDULE_WATERING_H_
// EOF
