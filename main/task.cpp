// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "task.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace IrrigationSystem {

void task(void *pParam)
{
    if (!pParam) {
        return;
    }
    static_cast<Task*>(pParam)->Run();
}


Task::Task(const std::string& taskName, const int coreId)
    :m_Status(TASK_STATUS_READY)
    ,m_TaskName(taskName)
    ,m_CoreId(coreId)
{}

Task::~Task()
{
    Stop();
}

void Task::Start()
{
    if (m_Status != TASK_STATUS_READY) {
        return;
    }
    m_Status = TASK_STATUS_RUN;
    xTaskCreatePinnedToCore(task, m_TaskName.c_str(), 8192, this, 1, nullptr, m_CoreId);
}

void Task::Stop()
{
    if (m_Status != TASK_STATUS_RUN) {
        return;
    }
    m_Status = TASK_STATUS_END;
}

void Task::Run()
{
    while(m_Status == TASK_STATUS_RUN) {
        Update();
    }
    vTaskDelete(nullptr);
}

} // IrrigationSystem

// EOF
