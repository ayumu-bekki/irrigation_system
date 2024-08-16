#ifndef TASK_H_
#define TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <string>

namespace IrrigationSystem {

/// FreeRTOS xTask Wrap
class Task {
 public:
  enum TaskStatus {
    TASK_STATUS_READY,
    TASK_STATUS_RUN,
    TASK_STATUS_END,
  };

  static constexpr int TASK_STAC_DEPTH = 8192;

  /// Task Priority
  static constexpr int PRIORITY_LOW = 0;
  static constexpr int PRIORITY_NORMAL = 1;
  static constexpr int PRIORITY_HIGH = 2;

 private:
  Task() {}

 public:
  Task(const std::string& taskName, const int priority, const int coreId);
  virtual ~Task();

  /// Start Task
  void Start();

  /// Stop Task
  void Stop();

  /// Initialize (Called when the Start function is executed.)
  virtual void Initialize() {}

  /// (override) sub class processing
  virtual void Update() = 0;

 public:
  /// Task Running
  void Run();

  /// Task Listener
  static void Listener(void* const pParam);

 protected:
  /// Task Status
  TaskStatus status_;

  /// Task Name
  std::string task_name_;

  /// Task Priority
  int priority_;

  /// Use Core Id
  int core_id_;
};

}  // namespace IrrigationSystem

#endif  // TASK_H_
// EOF
