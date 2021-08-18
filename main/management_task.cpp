// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "management_task.h"

#include <esp_log.h>

#include "define.h"
#include "util.h"
#include "irrigation_controller.h"
#include "http_request.h"
#include "schedule_manager.h"

namespace IrrigationSystem {

ManagementTask::ManagementTask(IrrigationInterface *const pIrricationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrricationInterface(pIrricationInterface)
{}

void ManagementTask::Update()
{
    if (!m_pIrricationInterface) {
        return;
    }

    // Schedule Manager
    m_pIrricationInterface->GetScheduleManager().Execute(); 
    
    Util::SleepMillisecond(10 * 1000);
}


} // IrrigationSystem

// EOF
