// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "relay_task.h"

#include <esp_log.h>

#include "define.h"
#include "util.h"
#include "irrigation_controller.h"
#include "gpio_control.h"

namespace IrrigationSystem {

RelayTask::RelayTask(IrrigationInterface *const pIrricationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrricationInterface(pIrricationInterface)
    ,m_OpenSecond(0)
{}

void RelayTask::Update()
{
    if (!m_pIrricationInterface) {
        return;
    }

    if (0 < m_OpenSecond) {
        ESP_LOGI(TAG, "Open Relay. %dsec", m_OpenSecond);
        GPIO::SetLevel(CONFIG_RELAY_SIGNAL_GPIO_NO, 1);

        const int tempSecond = m_OpenSecond;
        m_OpenSecond -= tempSecond;
        Util::SleepMillisecond(tempSecond * 1000);

        GPIO::SetLevel(CONFIG_RELAY_SIGNAL_GPIO_NO, 0);
        ESP_LOGI(TAG, "Close Relay.");

        return;
    }

    Util::SleepMillisecond(10 * 1000);
}

void RelayTask::AddOpenSecond(const int second)
{
    m_OpenSecond += second;
}

} // IrrigationSystem

// EOF
