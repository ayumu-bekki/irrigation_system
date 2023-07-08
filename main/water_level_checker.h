#ifndef WATER_LEVEL_CHECKER_H_
#define WATER_LEVEL_CHECKER_H_
// ESP32 Irrigation system
// (C)2023 bekki.jp

// Include ----------------------
#include <soc/soc.h>
#include <chrono>

#include "pwm.h"
#include "task.h"

namespace IrrigationSystem {

class WaterLevelChecker final : public Task
{
public:
    static constexpr char *const TASK_NAME = (char*)"WaterLevelCheckTask";
    static constexpr int PRIORITY = Task::PRIORITY_LOW;
    static constexpr int CORE_ID = APP_CPU_NUM;

public:
    WaterLevelChecker();

    void Initialize() override;
    void Update() override;

    void Check();

    float GetWaterLevel() const;

private:
    std::time_t m_CheckSec;
    float m_WaterLevel;
    Pwm m_pwm;
};

} // IrrigationSystem


#endif // WATER_LEVEL_CHECKER_H_
// EOF
