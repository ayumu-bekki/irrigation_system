#ifndef SYSTEM_QUEUE_H_
#define SYSTEM_QUEUE_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace IrrigationSystem {

/// FreeRTOS Queue Wrap
class SystemQueue
{
public:
    SystemQueue();
    virtual ~SystemQueue() {}

    void StartPoll();

    virtual void Receive() = 0;

protected:
    void CreateQueue();

    void SendQueueFromISR();

protected:
    QueueHandle_t m_QueueHandle;
};


} // IrrigationSystem


#endif // SYSTEM_QUEUE_H_
// EOF
