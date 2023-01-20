#ifndef WATERING_BUTTON_TASK_H_
#define WATERING_BUTTON_TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <esp_system.h>
#include <soc/soc.h>

#include "system_queue.h"
#include "task.h"
#include "irrigation_interface.h"


namespace IrrigationSystem {

class WateringButtonTask
    :public Task
    ,public SystemQueue
{
public:
    static constexpr char *const TASK_NAME = (char*)"WateringButtonTask";
    static constexpr int PRIORITY = Task::PRIORITY_NORMAL;
    static constexpr int CORE_ID = APP_CPU_NUM;

public:
    explicit WateringButtonTask(const IrrigationInterfaceWeakPtr pIrrigationInterface);
    ~WateringButtonTask();

private:
    void Update() override;

    void Receive() override;

private:
    static void IRAM_ATTR GpioIsrHandler(void*);

private:
    const IrrigationInterfaceWeakPtr m_pIrrigationInterface;
};

} // IrrigationSystem


#endif // WATERING_BUTTON_TASK_H_
// EOF
