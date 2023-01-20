#ifndef SCHEDULE_ADJUST_H_
#define SCHEDULE_ADJUST_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"
#include "irrigation_interface.h"

namespace IrrigationSystem {

class ScheduleAdjust final : public ScheduleBase
{
public:
    static constexpr char* SCHEDULE_NAME = (char*)"Adjust";
    static constexpr bool IS_VISIBLE_TASK = true;

private:
    ScheduleAdjust();

public:
    ScheduleAdjust(const IrrigationInterfaceWeakPtr pIrrigationInterface, const int hour, const int minute);

    void Exec() override;

private:
    const IrrigationInterfaceWeakPtr m_pIrrigationInterface;
};

} // IrrigationSystem

#endif // SCHEDULE_ADJUST_H_
// EOF
