#ifndef SCHEDULE_MANAGER_H_
#define SCHEDULE_MANAGER_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"

#include <chrono>
#include <vector>
#include <memory>

#include "irrigation_interface.h"

#include "schedule_base.h"

namespace IrrigationSystem {

class ScheduleManager final
{
public:
    using ScheduleBaseList = std::vector<ScheduleBase::UniquePtr>;

public:
    explicit ScheduleManager(IrrigationInterface *const pIrricationInterface);

    void Execute();

    const ScheduleBaseList& GetScheduleList() const;

    void AdjustSchedule();

    int GetCurrentMonth() const;
    int GetCurrentDay() const;

private:
    void InitializeNewDay(const std::tm& nowTimeInfo);

    void ClearScheduleList();
    void AddSchedule(ScheduleBase::UniquePtr&& scheduleItem);
    void SortScheduleTime();

    void DebugOutputSchedules();


private:
    IrrigationInterface* m_pIrricationInterface;
    ScheduleBaseList m_ScheduleList;
    int m_CurrentMonth;
    int m_CurrentDay;
};

} // IrrigationSystem

#endif // SCHEDULE_MANAGER_H_
// EOF
