#ifndef VALVE_TASK_H_
#define VALVE_TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include <chrono>
#include <memory>
#include <queue>
#include <mutex>

#include "irrigation_interface.h"
#include "pwm.h"
#include "task.h"
#include "valve_executor.h"

namespace IrrigationSystem {

class ValveTask final : public Task {
 public:
  static constexpr char *const TASK_NAME = (char *)"ValveTask";
  static constexpr int PRIORITY = Task::PRIORITY_NORMAL;
  static constexpr int CORE_ID = APP_CPU_NUM;

 public:
  explicit ValveTask(const IrrigationInterfaceWeakPtr pIrrigationInterface);

  void Update() override;

  void AddExecutor(ValveExecutorSharedPtr executor);
  ValveExecutorSharedPtr GetCurrentExecutor();
  void ForceStop();

 private:
  void Open();
  void Close();

 private:
  const IrrigationInterfaceWeakPtr m_pIrrigationInterface;

  ValveExecutorSharedPtr current_executor_;
  std::queue<ValveExecutorSharedPtr> executors_;

  Pwm m_pwm;
  std::mutex mtx_;
};

using ValveTaskUniquePtr = std::unique_ptr<ValveTask>;

}  // namespace IrrigationSystem

#endif  // VALVE_TASK_H_
// EOF
