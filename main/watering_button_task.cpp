// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "watering_button_task.h"

#include <driver/gpio.h>

#include "logger.h"

#define ESP_INTR_FLAG_DEFAULT 0

namespace IrrigationSystem {

WateringButtonTask::WateringButtonTask(IrrigationInterface *const pIrrigationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,SystemQueue()
    ,m_pIrrigationInterface(pIrrigationInterface)
{
    // Gpio Input Setting 
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << CONFIG_WATERING_INPUT_GPIO_NO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&io_conf);
    
    CreateQueue();

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(static_cast<gpio_num_t>(CONFIG_WATERING_INPUT_GPIO_NO), this->GpioIsrHandler, this);
}

WateringButtonTask::~WateringButtonTask()
{
    gpio_isr_handler_remove(static_cast<gpio_num_t>(CONFIG_WATERING_INPUT_GPIO_NO));
}

void WateringButtonTask::Update() 
{
    StartPoll();
}

void WateringButtonTask::Receive()
{
    if (!m_pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    const bool isButtonPush = (gpio_get_level(static_cast<gpio_num_t>(CONFIG_WATERING_INPUT_GPIO_NO)) == 0);
    m_pIrrigationInterface->ValveForce(isButtonPush);
}

void IRAM_ATTR WateringButtonTask::GpioIsrHandler(void *pData)
{
    if (!pData) {
        return;
    }
    WateringButtonTask *const pWateringButtonTask = static_cast<WateringButtonTask*>(pData);
    pWateringButtonTask->SendQueueFromISR();
}


} // IrrigationSystem

// EOF
