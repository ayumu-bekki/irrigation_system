#ifndef RELAY_TASK_H_
#define RELAY_TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include "task.h"
#include "irrigation_interface.h"

namespace IrrigationSystem {

class RelayTask : public Task
{
public:
    static constexpr char *const TASK_NAME = (char*)"RelayTask";
    static constexpr int PRIORITY = Task::PRIORITY_LOW;
    static constexpr int CORE_ID = APP_CPU_NUM;

public:
    explicit RelayTask(IrrigationInterface *const pParent);

    void Update() override;
    
    void AddOpenSecond(const int second);

private:
    IrrigationInterface* m_pIrricationInterface;
    int m_OpenSecond;
};

} // IrrigationSystem

#endif // RELAY_TASK_H_
// EOF
