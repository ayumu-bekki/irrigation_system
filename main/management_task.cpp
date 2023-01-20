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

ManagementTask::ManagementTask(const IrrigationInterfaceWeakPtr pIrrigationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrrigationInterface(pIrrigationInterface)
{}

void ManagementTask::Update()
{
    const IrrigationInterfaceSharedPtr irrigationInterface = m_pIrrigationInterface.lock();
    if (!irrigationInterface) {
        ESP_LOGE(TAG, "Failed IrrigationInterface is null");
        return;
    }

    // Schedule Manager
    const ScheduleManagerSharedPtr scheduleManager = irrigationInterface->GetScheduleManager().lock();
    if (!scheduleManager) {
        ESP_LOGE(TAG, "Failed ScheduleManager is null");
        return;
    }

    scheduleManager->Execute(); 
    Util::SleepMillisecond(10 * 1000);
}


} // IrrigationSystem

// EOF
