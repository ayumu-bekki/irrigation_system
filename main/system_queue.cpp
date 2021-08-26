// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "system_queue.h"

namespace IrrigationSystem {

SystemQueue::SystemQueue()
    :m_QueueHandle(nullptr)
{}

void SystemQueue::StartPoll()
{
    if (xQueueReceive(m_QueueHandle, nullptr, portMAX_DELAY)) {
        Receive();
    }
}

void SystemQueue::CreateQueue()
{
    m_QueueHandle = xQueueCreate(10, 0);
}

void SystemQueue::SendQueueFromISR()
{
    xQueueSendFromISR(m_QueueHandle, nullptr, nullptr);
}


} // IrrigationSystem

// EOF
