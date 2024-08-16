// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "valve_task.h"

#include <cmath>

#include "gpio_control.h"
#include "irrigation_interface.h"
#include "logger.h"
#include "util.h"
#include "watering_setting.h"

namespace {
  constexpr uint32_t VALVE_FREQUENCY = 10000;  // 10kHz
  constexpr ledc_timer_t VALVE_LEDC_TIMER = LEDC_TIMER_0;
}

namespace IrrigationSystem {

ValveTask::ValveTask(const IrrigationInterfaceWeakPtr pIrrigationInterface)
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      m_pIrrigationInterface(pIrrigationInterface) {
  m_pwm.Initialize(
      static_cast<ledc_channel_t>(LEDC_CHANNEL_0), VALVE_LEDC_TIMER,
      static_cast<gpio_num_t>(CONFIG_WATERING_OUTPUT_GPIO_NO), VALVE_FREQUENCY);
}

void ValveTask::Update() {
  if (current_executor_) {
    if (current_executor_->IsClose()) {
      Close();
    }
  } else {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!executors_.empty()) {
      ValveExecutorSharedPtr executor = executors_.front();
      executors_.pop();

      current_executor_ = std::move(executor);
      Open();
    }
  }

  Util::SleepMillisecond(100);
}

ValveExecutorSharedPtr ValveTask::GetCurrentExecutor() {
  return current_executor_;
}

void ValveTask::ForceStop() {
  if (current_executor_) {
    Close();
  }
  // clear queue
  std::lock_guard<std::mutex> lock(mtx_);
  std::queue<ValveExecutorSharedPtr>().swap(executors_);
}

void ValveTask::AddExecutor(ValveExecutorSharedPtr executor) {
  std::lock_guard<std::mutex> lock(mtx_);
  executors_.emplace(std::move(executor));
}

void ValveTask::Open() {
  if (!current_executor_) {
    return;
  }
  current_executor_->Start();

  const IrrigationInterfaceSharedPtr irrigationInterface =
      m_pIrrigationInterface.lock();
  if (!irrigationInterface) {
    return;
  }

#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  irrigationInterface->StartWaterMeasurement();
#endif

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  const float voltage = irrigationInterface->GetMainVoltage();
  const WateringSetting &wateringSetting =
      irrigationInterface->GetWateringSetting();

  const float rate = std::max(
        0.0f, std::min(1.0f, wateringSetting.GetValvePowerBaseRate() -
                                 ((voltage -
                                   wateringSetting.GetValvePowerBaseVoltage()) *
                                  wateringSetting.GetValvePowerVoltageRate())));
  ESP_LOGI(TAG, "Valve voltage rate Voltage:%fV Rate:%d", voltage,
           static_cast<int>(rate * 100));
  m_pwm.SetRate(rate);
#else
  m_pwm.SetRate(1.0f);
#endif

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  irrigationInterface->CheckWaterLevel();
#endif

}

void ValveTask::Close() {
  if (!current_executor_) {
    return;
  }


  const IrrigationInterfaceSharedPtr irrigationInterface =
      m_pIrrigationInterface.lock();
  if (!irrigationInterface) {
    return;
  }

  m_pwm.SetRate(0);

#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  int water_flow_counter = irrigationInterface->FinishWaterMeasurement();
  current_executor_->SetWaterAmount(water_flow_counter);
#endif

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  irrigationInterface->CheckWaterLevel();
#endif

  current_executor_->Finish();
  current_executor_.reset();
}

}  // namespace IrrigationSystem

// EOF
