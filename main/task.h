#ifndef TASK_H_
#define TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <string>

namespace IrrigationSystem {

/// FreeRTOS xTask Wrap
class Task
{
public:
    enum TaskStatus {
        TASK_STATUS_READY,
        TASK_STATUS_RUN,
        TASK_STATUS_END,
    };

private:
    Task() {}

public:
    Task(const std::string& taskName, const int coreId);
    virtual ~Task();
    
    /// Start Task
    void Start();
    /// Stop Task
    void Stop();

    /// (inner) Task Running
    void Run();

    /// (override) sub class processing
    virtual void Update() = 0;

protected:
    TaskStatus m_Status;
    std::string m_TaskName;
    int m_CoreId;
};

} // IrrigationSystem

#endif // TASK_H_
// EOF
