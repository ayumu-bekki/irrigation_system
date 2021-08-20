// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "relay_task.h"

#include "logger.h"
#include "util.h"
#include "irrigation_controller.h"
#include "gpio_control.h"


namespace IrrigationSystem {

RelayTask::RelayTask()
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_IsOpen(false)
    ,m_CloseEpoch(0)
{}

void RelayTask::Update()
{
    const std::time_t now = Util::GetEpoch();
    if (m_IsOpen && m_CloseEpoch < now) {
        m_IsOpen = false;
        ESP_LOGI(TAG, "Relay: Close");
        GPIO::SetLevel(CONFIG_RELAY_SIGNAL_GPIO_NO, 0);
    }

    Util::SleepMillisecond(500);
}

void RelayTask::AddOpenSecond(const int second)
{
    static constexpr int MAX_OPEN_SECOND = 180;
    if (second < 0 || MAX_OPEN_SECOND < second) {
        ESP_LOGW(TAG, "Invalid parameter. Out of range AddOpenSecond input:%d max:%d", second, MAX_OPEN_SECOND);
        return;
    }

    if (!m_IsOpen) {
        m_CloseEpoch = Util::GetEpoch();
    }
    m_CloseEpoch += second;
    ESP_LOGI(TAG, "Relay: Set Close Date. Close At:%s", Util::TimeToStr(Util::EpochToLocalTime(m_CloseEpoch)).c_str());

    if (!m_IsOpen) {
        m_IsOpen = true;
        ESP_LOGI(TAG, "Relay: Open");
        GPIO::SetLevel(CONFIG_RELAY_SIGNAL_GPIO_NO, 1);
    }
}

void RelayTask::ForceClose()
{
    m_CloseEpoch = 0;
}


std::time_t RelayTask::GetCloseEpoch() const
{
    if (!m_IsOpen) { 
        // Return 0 if no relay is not open
        return 0;
    }
    return m_CloseEpoch;
}

} // IrrigationSystem

// EOF
