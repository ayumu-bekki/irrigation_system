#ifndef VOLTAGE_CHECKER_TASK_H_
#define VOLTAGE_CHECKER_TASK_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include "task.h"

namespace IrrigationSystem {

class VoltageCheckTask final : public Task
{
public:
    static constexpr char *const TASK_NAME = (char*)"VoltageCheckTask";
    static constexpr int PRIORITY = Task::PRIORITY_LOW;
    static constexpr int CORE_ID = APP_CPU_NUM;

public:
    VoltageCheckTask();

    void Initialize() override;

    void Update() override;

    float GetVoltage() const;

private:
    float m_Voltage;
};

} // IrrigationSystem


#endif // VOLTAGE_CHECKER_TASK_H_
// EOF
