#ifndef RELAY_TASK_H_
#define RELAY_TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include <chrono>

#include "task.h"

namespace IrrigationSystem {

class RelayTask final : public Task
{
public:
    static constexpr char *const TASK_NAME = (char*)"RelayTask";
    static constexpr int PRIORITY = Task::PRIORITY_NORMAL;
    static constexpr int CORE_ID = APP_CPU_NUM;

public:
    RelayTask();

    void Update() override;
    
    void AddOpenSecond(const int second);
    void ResetTimer();
    void Force(const bool isOpen);

    std::time_t GetCloseEpoch() const;

private:
    void SetRelay();

private:
    bool m_IsTimerOpen;
    bool m_IsForceOpen;
    std::time_t m_CloseEpoch;
};

} // IrrigationSystem

#endif // RELAY_TASK_H_
// EOF
