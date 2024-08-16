#ifndef SCHEDULE_MANUAL_H_
#define SCHEDULE_MANUAL_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_interface.h"
#include "schedule_base.h"

namespace IrrigationSystem {

class ScheduleManual final : public ScheduleBase {
 public:
  static constexpr char* SCHEDULE_NAME = (char*)"Manual";
  static constexpr bool IS_VISIBLE_TASK = true;

 private:
  ScheduleManual();

 public:
  ScheduleManual(const int hour, const int minute, const int32_t water_amount);

  void Exec() override;

  int32_t GetWaterFlow() const override;

 private:
  const IrrigationInterfaceWeakPtr m_pIrrigationInterface;
  int32_t water_amount_;
};

}  // namespace IrrigationSystem

#endif  // SCHEDULE_MANUAL_H_
// EOF
