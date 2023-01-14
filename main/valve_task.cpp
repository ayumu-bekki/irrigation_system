// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "valve_task.h"

#include "logger.h"
#include "util.h"
#include "irrigation_controller.h"
#include "gpio_control.h"
#include <cmath>

namespace IrrigationSystem {

ValveTask::ValveTask()
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_IsTimerOpen(false)
    ,m_IsForceOpen(false)
    ,m_CloseEpoch(0)
{}

void ValveTask::Initialize()
{
    m_pwm.Initialize(static_cast<ledc_channel_t>(LEDC_CHANNEL_0), static_cast<gpio_num_t>(CONFIG_WATERING_OUTPUT_GPIO_NO));
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
    const float voltage = Util::GetVoltage();

    float rate = 0.0f;
    if (m_IsTimerOpen || m_IsForceOpen) {
        if (voltage <= 1.0f) {
            rate = 1.0f;
        } else {
           rate = std::max(0.0f, std::min(1.0f, 0.5f - ((voltage - 12.0f) * 0.05f)));
        }
    }
    ESP_LOGI(TAG, "Valve: TimerOpen:%d Force:%d Voltage:%fV Rate:%d", m_IsTimerOpen, m_IsForceOpen, voltage, static_cast<int>(rate * 100));

    m_pwm.SetRate(rate);
}

} // IrrigationSystem

// EOF
