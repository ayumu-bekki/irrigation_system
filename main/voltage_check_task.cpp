// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "voltage_check_task.h"

#include "logger.h"
#include "util.h"
#include "gpio_control.h"

namespace IrrigationSystem {

VoltageCheckTask::VoltageCheckTask()
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_Voltage(0.0f)
{}

void VoltageCheckTask::Initialize()
{
    ESP_LOGI(TAG, "Initialize ADC");
    GPIO::InitAdc(CONFIG_VAOLTAGE_CHECK_INPUT_ADC_CHANNEL_NO);
}


void VoltageCheckTask::Update()
{
    GPIO::SetLevel(CONFIG_VAOLTAGE_CHECK_OUTPUT_GPIO_NO, 1);
    Util::SleepMillisecond(1000);

    static const int32_t getRound = 10;
    const uint32_t adcVoltage = GPIO::GetAdcVoltage(CONFIG_VAOLTAGE_CHECK_INPUT_ADC_CHANNEL_NO, getRound);

    GPIO::SetLevel(CONFIG_VAOLTAGE_CHECK_OUTPUT_GPIO_NO, 0);

    // Voltage divider rate
    static const float TOP_REGISTER = 12.2f; // kΩ
    static const float BOTTOM_REGISTER = 2.2f; // kΩ
    m_Voltage = Util::GetOriginalVoltageFromDividerRegister(adcVoltage, TOP_REGISTER, BOTTOM_REGISTER);

    ESP_LOGI(TAG, "Voltage:%.2f[V] ADC Voltage:%d[mV]", m_Voltage, adcVoltage);

    Util::SleepMillisecond(60 * 60 * 1000);
}

float VoltageCheckTask::GetVoltage() const
{
    return m_Voltage;
}


} // IrrigationSystem

// EOF
