#ifndef SCHEDULE_WATERING_H_
#define SCHEDULE_WATERING_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"
#include "irrigation_interface.h"

namespace IrrigationSystem {

class ScheduleWatering final : public ScheduleBase
{
public:
    static constexpr char* SCHEDULE_NAME = (char*)"Watering";
    static constexpr bool IS_VISIBLE_TASK = true;

private:
    ScheduleWatering();

public:
    ScheduleWatering(const IrrigationInterfaceWeakPtr pIrrigationInterface, const int hour, const int minute, const int openSecond);

    void Exec() override;

private:
    const IrrigationInterfaceWeakPtr m_pIrrigationInterface;
    int m_OpenSecond;
};

} // IrrigationSystem

#endif // SCHEDULE_WATERING_H_
// EOF
