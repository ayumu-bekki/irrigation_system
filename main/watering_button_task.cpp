// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "watering_button_task.h"

#include <driver/gpio.h>

#include "logger.h"

#define ESP_INTR_FLAG_DEFAULT 0

namespace IrrigationSystem {

WateringButtonTask::WateringButtonTask(const IrrigationInterfaceWeakPtr pIrrigationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,SystemQueue()
    ,m_pIrrigationInterface(pIrrigationInterface)
{
    // Gpio Input Setting 
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << CONFIG_WATERING_INPUT_GPIO_NO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; // IO34 don’t have internal pull-down resistors
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
    ESP_LOGI(TAG, "WBT Update");
    StartPoll();
}

void WateringButtonTask::Receive()
{
    const IrrigationInterfaceSharedPtr irrigationInterface = m_pIrrigationInterface.lock();
    if (!irrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    const bool isButtonPush = (gpio_get_level(static_cast<gpio_num_t>(CONFIG_WATERING_INPUT_GPIO_NO)) == 0);
    irrigationInterface->ValveForce(isButtonPush);
    
    ESP_LOGI(TAG, "Watering Button Status:%s", isButtonPush ? "ON" : "OFF");
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
