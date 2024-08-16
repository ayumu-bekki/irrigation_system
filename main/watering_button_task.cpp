// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "watering_button_task.h"

#include <driver/gpio.h>

#include "logger.h"
#include "gpio_control.h"
#include "util.h"
#include "schedule_manager.h"
#include "schedule_manual.h"

namespace IrrigationSystem {

WateringButtonTask::WateringButtonTask(
    const IrrigationInterfaceWeakPtr pIrrigationInterface)
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      m_pIrrigationInterface(pIrrigationInterface),
      button_current_(false),
      button_counter_(0) {
  // Gpio Input Setting
  GPIO::InitInput(CONFIG_WATERING_INPUT_GPIO_NO);
}

WateringButtonTask::~WateringButtonTask() {
}

void WateringButtonTask::Update() {
  Util::SleepMillisecond(100);

  const bool is_button_push =
      (gpio_get_level(static_cast<gpio_num_t>(CONFIG_WATERING_INPUT_GPIO_NO)) ==
       0);
  //ESP_LOGI(TAG, "Rect Button :%s", is_button_push ? "ON" : "OFF");

  if (button_current_ == is_button_push) {
    return;
  } 
  
  ++button_counter_;

  if (button_counter_ < 3) {
    return;
  }

  button_current_ = is_button_push;
  button_counter_ = 0;


  const IrrigationInterfaceSharedPtr irrigationInterface =
      m_pIrrigationInterface.lock();
  if (!irrigationInterface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }

  if (button_current_) {
    ESP_LOGI(TAG, "BUTTON ON!");

    valve_executor_ = std::make_shared<ValveExecutor>();
    valve_executor_->SetStatus(ValveExecutor::ExecutorStatus::EXECUTOR_MANUAL_START);

    irrigationInterface->AddValveExecutor(valve_executor_);
  } else {
    ESP_LOGI(TAG, "BUTTON OFF!");
    if (valve_executor_) {
      valve_executor_->SetStatus(ValveExecutor::ExecutorStatus::EXECUTOR_MANUAL_CLOSE);

      const ScheduleManagerSharedPtr scheduleManager =
        irrigationInterface->GetScheduleManager().lock();
      if (!scheduleManager) {
        ESP_LOGE(TAG, "Failed Schedule Manager is null");
        valve_executor_.reset();
        return ;
      }

      std::tm now = Util::GetLocalTime();
      scheduleManager->AddSchedule(std::make_unique<ScheduleManual>(now.tm_hour, now.tm_min, valve_executor_->GetWaterAmount()));
      scheduleManager->SortScheduleTime();
 
      valve_executor_.reset();
    }
  }
}


}  // namespace IrrigationSystem

// EOF
