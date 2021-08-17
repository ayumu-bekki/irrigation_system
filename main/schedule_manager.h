#ifndef SCHEDULE_MANAGER_H_
#define SCHEDULE_MANAGER_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"

#include <vector>
#include <time.h>

namespace IrrigationSystem {

class ScheduleManager
{
public:
    using ScheduleBaseList = std::vector<ScheduleBase>;

public:
    ScheduleManager();

    void Execute();

    const ScheduleBaseList& GetScheduleList() const;

private:
    void Build(const tm& nowTimeInfo);

private:
    int m_CurrentMonthDay;
    ScheduleBaseList m_ScheduleList;
};

} // IrrigationSystem

#endif // SCHEDULE_MANAGER_H_
// EOF
