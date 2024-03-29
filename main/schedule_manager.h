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
    using ScheduleBaseList = std::vector<ScheduleBaseUniquePtr>;

public:
    explicit ScheduleManager(const IrrigationInterfaceWeakPtr pIrrigationInterface);

    void Execute();

    /// Date change schedule initialization
    void InitializeNewDay(const std::tm& nowTimeInfo);

    const ScheduleBaseList& GetScheduleList() const;

    void AdjustSchedule();

    int GetCurrentMonth() const;
    int GetCurrentDay() const;

private:

    /// Add a schedule to the list
    void AddSchedule(ScheduleBaseUniquePtr&& scheduleItem);

    /// Disable a schedule whose execution time has already expired.
    void DisableExpiredSchedule(const std::tm& timeInfo);

    /// Sort the schedule in ascending order
    void SortScheduleTime();

    /// DebugOnly
    void DebugOutputSchedules();

private:
    const IrrigationInterfaceWeakPtr m_pIrrigationInterface;
    ScheduleBaseList m_ScheduleList;
    int m_CurrentMonth;
    int m_CurrentDay;
    
};

using ScheduleManagerSharedPtr = std::shared_ptr<ScheduleManager>;
using ScheduleManagerWeakPtr = std::weak_ptr<ScheduleManager>;

} // IrrigationSystem

#endif // SCHEDULE_MANAGER_H_
// EOF
