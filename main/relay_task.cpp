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
    :Task(TASK_NAME, CORE_ID)
    ,m_pIrricationInterface(pIrricationInterface)
    ,m_OpenSecond(0)
{}

void RelayTask::Update()
{
    if (!m_pIrricationInterface) {
        return;
    }

    if (0 < m_OpenSecond) {
        ESP_LOGI(TAG, "GPIO RELAY OPEN %dsec", m_OpenSecond);

        GPIO::SetLevel(RELAY_SIGNAL_GPIO, 1);
        Util::SleepMillisecond(m_OpenSecond * 1000);
        GPIO::SetLevel(RELAY_SIGNAL_GPIO, 0);
        ESP_LOGI(TAG, "GPIO RELAY CLOSE");
        m_OpenSecond = 0;
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
