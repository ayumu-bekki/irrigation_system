#ifndef SCHEDULE_BASE_H_
#define SCHEDULE_BASE_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <string>
#include <memory>
#include <chrono>

namespace IrrigationSystem {

class ScheduleBase
{
public:
    enum Status : int {
        STATUS_NONE,
        STATUS_WAIT,
        STATUS_EXECUTED,
        STATUS_DISABLE,
        MAX_STATUS,
    };

protected:
    ScheduleBase();
    ScheduleBase(const Status status, const std::string& name, const int hour, const int minute, const bool isVisible);
    
public:
    virtual ~ScheduleBase() {}

    virtual void Exec() = 0;

    bool CanExecute(const std::tm& timeInfo);

    Status GetStatus() const;
    void SetStatus(const Status status);

    /// Disable if the time has expired.
    void DisableExpired(const std::tm& timeInfo);

    const std::string& GetName() const;
    int GetHour() const;
    int GetMinute() const;
    bool IsVisible() const;
    std::chrono::minutes GetChronoMinutes() const;

    int GetDiffTime() const;

public: 
    static const char* StatusToStr(const ScheduleBase::Status status);
    static const char* StatusToRecordStyle(const ScheduleBase::Status status);

private:
    Status m_Status;
    std::string m_Name;
    unsigned int m_Hour;
    unsigned int m_Minute;
    bool m_IsVisible;
};

using ScheduleBaseUniquePtr = std::unique_ptr<ScheduleBase>;


} // IrrigationSystem

#endif // SCHEDULE_BASE_H_
// EOF
