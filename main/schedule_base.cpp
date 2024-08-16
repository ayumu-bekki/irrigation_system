// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"

#include <chrono>

#include "util.h"

namespace IrrigationSystem {

ScheduleBase::ScheduleBase()
    : status_(STATUS_NONE), name_(), hour_(0), minute_(0), is_visible_(false) {}

ScheduleBase::ScheduleBase(const ScheduleBase::Status status,
                           const std::string& name, const int hour,
                           const int minute, const bool is_visible)
    : status_(status),
      name_(name),
      hour_(hour),
      minute_(minute),
      is_visible_(is_visible) {}

ScheduleBase::Status ScheduleBase::GetStatus() const { return status_; }

void ScheduleBase::SetStatus(const ScheduleBase::Status status) {
  status_ = status;
}

void ScheduleBase::DisableExpired(const std::tm& time_info) {
  if (CanExecute(time_info)) {
    status_ = STATUS_DISABLE;
  }
}

const std::string& ScheduleBase::GetName() const { return name_; }

bool ScheduleBase::CanExecute(const std::tm& time_info) {
  const std::chrono::minutes nowChrono = Util::GetChronoHourMinutes(time_info);
  return status_ == STATUS_WAIT && GetChronoMinutes() <= nowChrono;
}

int ScheduleBase::GetHour() const { return hour_; }

int ScheduleBase::GetMinute() const { return minute_; }

bool ScheduleBase::IsVisible() const { return is_visible_; }

std::chrono::minutes ScheduleBase::GetChronoMinutes() const {
  return std::chrono::hours(hour_) + std::chrono::minutes(minute_);
}

int ScheduleBase::GetDiffTime() const { return GetChronoMinutes().count(); }

int32_t ScheduleBase::GetWaterFlow() const { return -1; }

const char* ScheduleBase::StatusToStr(const ScheduleBase::Status status) {
  static constexpr char* EmptyStr = (char*)"";
  if (status < ScheduleBase::STATUS_NONE ||
      ScheduleBase::MAX_STATUS <= status) {
    return EmptyStr;
  }

  static constexpr char* StatusStrTbl[ScheduleBase::MAX_STATUS] = {
      (char*)"None",   (char*)"Wait",    (char*)"Executed",
      (char*)"Manual", (char*)"Disable",
  };
  return StatusStrTbl[status];
}

const char* ScheduleBase::StatusToRecordStyle(
    const ScheduleBase::Status status) {
  static constexpr char* EmptyStr = (char*)"";
  if (status < ScheduleBase::STATUS_NONE ||
      ScheduleBase::MAX_STATUS <= status) {
    return EmptyStr;
  }

  static constexpr char* StatusRecordStyleTbl[ScheduleBase::MAX_STATUS] = {
      (char*)"schedule_none",       (char*)"schedule_wait",
      (char*)"schedule_executable", (char*)"schedule_manual",
      (char*)"schedule_disable",
  };
  return StatusRecordStyleTbl[status];
}

}  // namespace IrrigationSystem

// EOF
