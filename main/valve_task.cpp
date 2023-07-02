// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "valve_task.h"

#include <cmath>

#include "logger.h"
#include "util.h"
#include "watering_setting.h"
#include "gpio_control.h"
#include "irrigation_interface.h"

namespace IrrigationSystem {

ValveTask::ValveTask(const IrrigationInterfaceWeakPtr pIrrigationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrrigationInterface(pIrrigationInterface)
    ,m_IsTimerOpen(false)
    ,m_IsForceOpen(false)
    ,m_CloseEpoch(0)
{
    constexpr uint32_t VALVE_FREQUENCY = 10000; // 10kHz
    constexpr ledc_timer_t VALVE_LEDC_TIMER = LEDC_TIMER_0;
    m_pwm.Initialize(static_cast<ledc_channel_t>(LEDC_CHANNEL_0),
                     VALVE_LEDC_TIMER,
                     static_cast<gpio_num_t>(CONFIG_WATERING_OUTPUT_GPIO_NO),
                     VALVE_FREQUENCY);
}

void ValveTask::Update()
{
    const std::time_t now = Util::GetEpoch();
    if (m_IsTimerOpen && m_CloseEpoch < now) {
        m_IsTimerOpen = false;
        SetValve();
    }

    Util::SleepMillisecond(500);
}

void ValveTask::AddOpenSecond(const int second)
{
    static constexpr int MAX_OPEN_SECOND = 180;
    if (second < 0 || MAX_OPEN_SECOND < second) {
        ESP_LOGW(TAG, "Invalid parameter. Out of range AddOpenSecond input:%d max:%d", second, MAX_OPEN_SECOND);
        return;
    }

    if (!m_IsTimerOpen) {
        m_CloseEpoch = Util::GetEpoch();
    }
    m_CloseEpoch += second;
    ESP_LOGI(TAG, "Valve: Set Close Date. Close At:%s", Util::TimeToStr(Util::EpochToLocalTime(m_CloseEpoch)).c_str());

    if (!m_IsTimerOpen) {
        m_IsTimerOpen = true;
        SetValve();
    }
}

void ValveTask::ResetTimer()
{
    m_CloseEpoch = 0;
}

void ValveTask::Force(const bool isOpen)
{
    m_IsForceOpen = isOpen;
    SetValve();
}

std::time_t ValveTask::GetCloseEpoch() const
{
    if (!m_IsTimerOpen) { 
        // Return 0 if no valve is not open
        return 0;
    }
    return m_CloseEpoch;
}

void ValveTask::SetValve()
{
    ESP_LOGI(TAG, "Valve: TimerOpen:%d Force:%d", m_IsTimerOpen, m_IsForceOpen);
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
    const IrrigationInterfaceSharedPtr irrigationInterface = m_pIrrigationInterface.lock();
    if (!irrigationInterface) {
        return;
    }

    const float voltage = irrigationInterface->GetMainVoltage();
    const WateringSetting &wateringSetting = irrigationInterface->GetWateringSetting();

    float rate = 0.0f;
    if (m_IsTimerOpen || m_IsForceOpen) {
        rate = std::max(0.0f, 
               std::min(1.0f, 
               wateringSetting.GetValvePowerBaseRate() - ((voltage - wateringSetting.GetValvePowerBaseVoltage()) * wateringSetting.GetValvePowerVoltageRate())));
    }
    ESP_LOGI(TAG, "Valve voltage rate Voltage:%fV Rate:%d", voltage, static_cast<int>(rate * 100));
#else
    const float rate = (m_IsTimerOpen || m_IsForceOpen) ? 1.0f : 0.0f;
#endif
    m_pwm.SetRate(rate);

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
    irrigationInterface->CheckWaterLevel();
#endif
}

} // IrrigationSystem

// EOF
