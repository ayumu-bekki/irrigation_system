#ifndef SCHEDULE_DUMMY_H_
#define SCHEDULE_DUMMY_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"
#include "irrigation_interface.h"

namespace IrrigationSystem {

class ScheduleDummy final : public ScheduleBase
{
public:
    static constexpr char* SCHEDULE_NAME = (char*)"Dummy";
    static constexpr bool IS_VISIBLE_TASK = false;

private:
    ScheduleDummy();

public:
    ScheduleDummy(const int hour, const int minute);

    void Exec() override;
};

} // IrrigationSystem

#endif // SCHEDULE_DUMMY_H_
// EOF
