#ifndef SCHEDULE_BASE_H_
#define SCHEDULE_BASE_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <chrono>
#include <memory>
#include <string>

namespace IrrigationSystem {

class ScheduleBase {
 public:
  enum Status : int {
    STATUS_NONE,
    STATUS_WAIT,
    STATUS_EXECUTED,
    STATUS_MANUAL,
    STATUS_DISABLE,
    MAX_STATUS,
  };

 protected:
  ScheduleBase();
  ScheduleBase(const Status status, const std::string& name, const int hour,
               const int minute, const bool is_visible);

 public:
  virtual ~ScheduleBase() {}

  virtual void Exec() = 0;

  bool CanExecute(const std::tm& time_info);

  Status GetStatus() const;
  void SetStatus(const Status status);

  /// Disable if the time has expired.
  void DisableExpired(const std::tm& time_info);

  const std::string& GetName() const;
  int GetHour() const;
  int GetMinute() const;
  bool IsVisible() const;
  std::chrono::minutes GetChronoMinutes() const;
  virtual int32_t GetWaterFlow() const;

  int GetDiffTime() const;

 public:
  static const char* StatusToStr(const ScheduleBase::Status status);
  static const char* StatusToRecordStyle(const ScheduleBase::Status status);

 private:
  Status status_;
  std::string name_;
  unsigned int hour_;
  unsigned int minute_;
  bool is_visible_;
};

using ScheduleBaseUniquePtr = std::unique_ptr<ScheduleBase>;

}  // namespace IrrigationSystem

#endif  // SCHEDULE_BASE_H_
// EOF
