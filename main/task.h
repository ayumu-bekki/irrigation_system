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

    static constexpr int TASK_STAC_DEPTH = 8192;

private:
    Task() {}

public:
    Task(const std::string& taskName, const int priority, const int coreId);
    virtual ~Task();
    
    /// Start Task
    void Start();

    /// Stop Task
    void Stop();

    /// (inner) Task Running
    void Run();

    /// (override) sub class processing
    virtual void Update() = 0;

public:
    /// Task Listener
    static void Listener(void *const pParam);

protected:
    /// Task Status
    TaskStatus m_Status;

    /// Task Name
    std::string m_TaskName;

    /// Task Priority
    int m_Priority;

    /// Use Core Id
    int m_CoreId;
};

} // IrrigationSystem

#endif // TASK_H_
// EOF
