#ifndef VALVE_EXECUTOR_H_
#define VALVE_EXECUTOR_H_
// ESP32 Irrigation System
// (C)2024 bekki.jp

// Include ----------------------
#include <memory>
#include <chrono>
#include "util.h"
#include "logger.h"

namespace {
  constexpr int MAX_OPEN_SECOND = 180;
}

namespace IrrigationSystem {

class ValveExecutor final {
 public:
  enum ExecutorStatus {
    EXECUTOR_NONE = 0,
    EXECUTOR_SCHEDULE = 1,
    EXECUTOR_MANUAL_START = 2,
    EXECUTOR_MANUAL_CLOSE = 3,
  };
 public:
  ValveExecutor() 
    : status_(EXECUTOR_NONE),
      open_seconds_(0),
      water_amount_(0),
      close_epoch_(0) {}

  void Start() {
    if (status_ == EXECUTOR_SCHEDULE) {
      if (open_seconds_ < 0 || MAX_OPEN_SECOND < open_seconds_) {
        ESP_LOGW(TAG,
                 "Invalid parameter. Out of range input:%d max:%d",
                 open_seconds_, MAX_OPEN_SECOND);
        return;
      }
      close_epoch_ = Util::GetEpoch() + open_seconds_;

      ESP_LOGI(TAG, "Valve: Set Close Date. Close At:%s",
               Util::TimeToStr(Util::EpochToLocalTime(close_epoch_)).c_str());
    } else if (status_ == EXECUTOR_MANUAL_START) {
      close_epoch_ = Util::GetEpoch();
    }
  }

  void Finish() {
    if (status_ == EXECUTOR_MANUAL_CLOSE) {
      open_seconds_ = Util::GetEpoch() - close_epoch_;
      close_epoch_ = Util::GetEpoch();
    }   
  }

  void SetStatus(ExecutorStatus status) {
    status_ = status;
  } 
  ExecutorStatus GetStatus() const { return status_; }
  int GetOpenSeconds() const { return open_seconds_; }
  void SetOpenSeconds(const int open_seconds) {
    open_seconds_ = open_seconds;
  } 
  int GetWaterAmount() const { return water_amount_; }
  void SetWaterAmount(const int water_amount) {
    water_amount_ = water_amount;
  }
  int GetCloseEpoch() const { return close_epoch_; }
  void SetCloseEpoch(const int close_epoch) {
    close_epoch_ = close_epoch;
  }
  bool IsClose() const {
    if (status_ == EXECUTOR_SCHEDULE) {
      return close_epoch_ < Util::GetEpoch();
    } if (status_ == EXECUTOR_MANUAL_START) {
      return false;
    }
    return true;
  }

 private:
  ExecutorStatus status_;
  int open_seconds_;
  int water_amount_;
  std::time_t close_epoch_;
};

using ValveExecutorSharedPtr = std::shared_ptr<ValveExecutor>;

}  // namespace IrrigationSystem

#endif  // VALVE_EXECUTOR_H_
// EOF
