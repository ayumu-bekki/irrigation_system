#ifndef SCHEDULE_MANAGER_H_
#define SCHEDULE_MANAGER_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <chrono>
#include <memory>
#include <vector>

#include "irrigation_interface.h"
#include "schedule_base.h"

namespace IrrigationSystem {

class ScheduleManager final {
 public:
  using ScheduleBaseList = std::vector<ScheduleBaseUniquePtr>;

 public:
  explicit ScheduleManager(
      const IrrigationInterfaceWeakPtr irrigation_interface);

  void Execute();

  /// Date change schedule initialization
  void InitializeNewDay(const std::tm& nowTimeInfo);

  const ScheduleBaseList& GetScheduleList() const;

  void AdjustSchedule();

  int GetCurrentMonth() const;
  int GetCurrentDay() const;

  /// Add a schedule to the list
  void AddSchedule(ScheduleBaseUniquePtr&& scheduleItem);

  /// Sort the schedule in ascending order
  void SortScheduleTime();

 private:
  /// Disable a schedule whose execution time has already expired.
  void DisableExpiredSchedule(const std::tm& time_info);

  /// DebugOnly
  void DebugOutputSchedules();

 private:
  const IrrigationInterfaceWeakPtr irrigation_interface_;
  ScheduleBaseList schedule_list_;
  int current_month_;
  int current_day_;
};

using ScheduleManagerSharedPtr = std::shared_ptr<ScheduleManager>;
using ScheduleManagerWeakPtr = std::weak_ptr<ScheduleManager>;

}  // namespace IrrigationSystem

#endif  // SCHEDULE_MANAGER_H_
// EOF
