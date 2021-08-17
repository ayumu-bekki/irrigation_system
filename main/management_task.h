#ifndef MANAGEMENT_TASK_H_
#define MANAGEMENT_TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include "task.h"
#include "irrigation_interface.h"

namespace IrrigationSystem {

class ManagementTask : public Task
{
public:
    static constexpr char *const TASK_NAME = (char*)"ManagementTask";
    static constexpr int PRIORITY = Task::PRIORITY_HIGH;
    static constexpr int CORE_ID = APP_CPU_NUM;

public:
    explicit ManagementTask(IrrigationInterface *const pIrricationInterface);

    void Update() override;

private:
    IrrigationInterface* m_pIrricationInterface;
};

} // IrrigationSystem

#endif // MANAGEMENT_TASK_H_
// EOF
