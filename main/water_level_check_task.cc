// ESP32 Irrigation System
// (C)2023 bekki.jp

// Include ----------------------
#include "water_level_check_task.h"

#include <cmath>

#include "gpio_control.h"
#include "logger.h"
#include "util.h"

namespace {
constexpr std::time_t CHECK_WATER_LEVEL_INTERVAL_SEC = 10 * 60;
}

namespace IrrigationSystem {

WaterLevelCheckTask::WaterLevelCheckTask(
    const IrrigationInterfaceWeakPtr irrigation_interface)
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      irrigation_interface_(irrigation_interface),
      check_sec_(0),
      water_level_(0.0f),
      pwm_() {}

void WaterLevelCheckTask::Initialize() {
  constexpr uint32_t VALVE_FREQUENCY = 1000000;  // 1MHz
  constexpr ledc_timer_t VALVE_LEDC_TIMER = LEDC_TIMER_1;
  pwm_.Initialize(
      static_cast<ledc_channel_t>(LEDC_CHANNEL_1), VALVE_LEDC_TIMER,
      static_cast<gpio_num_t>(CONFIG_WATER_LEVEL_CHECK_OUTPUT_GPIO_NO),
      VALVE_FREQUENCY);
}

void WaterLevelCheckTask::Update() {
  if (check_sec_ < Util::GetEpoch()) {
    pwm_.SetRate(0.5f);

    static const int32_t WATER_LEVEL_VOLTAGE_CHECK_PRE_WARMING_MILISEC = 100;
    Util::SleepMillisecond(WATER_LEVEL_VOLTAGE_CHECK_PRE_WARMING_MILISEC);

    static const int32_t VOLTAGE_ADC_CHECK_ROUND = 10;
    const uint32_t adcVoltage = GPIO::GetAdcVoltage(
        CONFIG_WATER_LEVEL_CHECK_INPUT_ADC_CHANNEL_NO, VOLTAGE_ADC_CHECK_ROUND);

    pwm_.SetRate(0.0f);

    const int32_t minVoltage = 420;
    const int32_t maxVoltage = 1900;

    water_level_ = std::max(
        0.0f, std::min(1.0f, ((static_cast<float>(adcVoltage) - minVoltage) /
                              (float)(maxVoltage - minVoltage))));
    ESP_LOGI(TAG, "WaterLevelCheck adcVolt:%dmV min:%dmv max:%dmv rate:%0.2f",
             adcVoltage, minVoltage, maxVoltage, water_level_);

#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  // Publish MQTT
  const IrrigationInterfaceSharedPtr irrigation_interface =
      irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return;
  }


  // Generate Response
  std::stringstream message;
  message << "{\"level\":" << std::setfill('0') << std::fixed
                << std::setprecision(2) << water_level_ << "}";
  irrigation_interface->PublishMQTTMessage("irrigation_system/" CONFIG_MQTT_DEVICE_TOPIC_NAME "/telemetry/water_level", message.str().c_str());
#endif

    check_sec_ = Util::GetEpoch() + CHECK_WATER_LEVEL_INTERVAL_SEC;
  }

  static const int32_t NEXT_CHECK_MILLISECOND = 1000;
  Util::SleepMillisecond(NEXT_CHECK_MILLISECOND);
}

void WaterLevelCheckTask::Check() { check_sec_ = 0; }

float WaterLevelCheckTask::GetWaterLevel() const { return water_level_; }

}  // namespace IrrigationSystem

// EOF
