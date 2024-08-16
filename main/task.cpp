// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace IrrigationSystem {

Task::Task(const std::string& taskName, const int priority, const int coreId)
    : status_(TASK_STATUS_READY),
      task_name_(taskName),
      priority_(priority),
      core_id_(coreId) {}

Task::~Task() { Stop(); }

void Task::Start() {
  if (status_ != TASK_STATUS_READY) {
    return;
  }
  status_ = TASK_STATUS_RUN;
  xTaskCreatePinnedToCore(this->Listener, task_name_.c_str(), TASK_STAC_DEPTH,
                          this, priority_, nullptr, core_id_);
}

void Task::Stop() {
  if (status_ != TASK_STATUS_RUN) {
    return;
  }
  status_ = TASK_STATUS_END;
}

void Task::Run() {
  Initialize();
  while (status_ == TASK_STATUS_RUN) {
    Update();
  }
}

void Task::Listener(void* const pParam) {
  if (pParam) {
    static_cast<Task*>(pParam)->Run();
  }
  vTaskDelete(nullptr);
}

}  // namespace IrrigationSystem

// EOF
