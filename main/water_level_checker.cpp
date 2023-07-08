// ESP32 Irrigation System
// (C)2023 bekki.jp

// Include ----------------------
#include "water_level_checker.h"

#include <cmath>

#include "logger.h"
#include "util.h"
#include "gpio_control.h"

namespace {
    constexpr std::time_t CHECK_WATER_LEVEL_INTERVAL_SEC = 10 * 60;
}

namespace IrrigationSystem {

WaterLevelChecker::WaterLevelChecker()
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_CheckSec(0)
    ,m_WaterLevel(0.0f)
    ,m_pwm()
{}

void WaterLevelChecker::Initialize()
{
    constexpr uint32_t VALVE_FREQUENCY = 1000000; // 1MHz
    constexpr ledc_timer_t VALVE_LEDC_TIMER = LEDC_TIMER_1;
    m_pwm.Initialize(static_cast<ledc_channel_t>(LEDC_CHANNEL_1),
                     VALVE_LEDC_TIMER,
                     static_cast<gpio_num_t>(CONFIG_WATER_LEVEL_CHECK_OUTPUT_GPIO_NO),
                     VALVE_FREQUENCY);

}

void WaterLevelChecker::Update()
{
    if (m_CheckSec < Util::GetEpoch()) {
      m_pwm.SetRate(0.5f);
      Util::SleepMillisecond(500);

      static const int32_t VOLTAGE_ADC_CHECK_ROUND = 10;
      const uint32_t adcVoltage = GPIO::GetAdcVoltage(CONFIG_WATER_LEVEL_CHECK_INPUT_ADC_CHANNEL_NO, VOLTAGE_ADC_CHECK_ROUND);

      m_pwm.SetRate(0.0f);

      const int32_t minVoltage = 420;
      const int32_t maxVoltage = 1900;

      m_WaterLevel = std::max(0.0f, 
                     std::min(1.0f,
                     ((static_cast<float>(adcVoltage) - minVoltage) / (float)(maxVoltage - minVoltage))));
      ESP_LOGI(TAG, "WaterLevelCheck adcVolt:%dmV min:%dmv max:%dmv rate:%0.2f", adcVoltage, minVoltage, maxVoltage, m_WaterLevel);

      m_CheckSec = Util::GetEpoch() + CHECK_WATER_LEVEL_INTERVAL_SEC;
    }

    static const int32_t NEXT_CHECK_MILLISECOND = 1000;
    Util::SleepMillisecond(NEXT_CHECK_MILLISECOND);
}

void WaterLevelChecker::Check()
{
    m_CheckSec = 0;
}

float WaterLevelChecker::GetWaterLevel() const
{
    return m_WaterLevel;
}


} // IrrigationSystem

// EOF
