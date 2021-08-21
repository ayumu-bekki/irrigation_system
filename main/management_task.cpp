// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "management_task.h"

#include "logger.h"
#include "util.h"
#include "irrigation_controller.h"
#include "http_request.h"
#include "schedule_manager.h"

namespace IrrigationSystem {

ManagementTask::ManagementTask(IrrigationInterface *const pIrrigationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrrigationInterface(pIrrigationInterface)
{}

void ManagementTask::Update()
{
    if (!m_pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    // Schedule Manager
    m_pIrrigationInterface->GetScheduleManager().Execute(); 
    
    Util::SleepMillisecond(10 * 1000);
}


} // IrrigationSystem

// EOF
