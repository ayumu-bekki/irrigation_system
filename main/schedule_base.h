#ifndef SCHEDULE_BASE_H_
#define SCHEDULE_BASE_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <time.h>
#include <string>

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

public:
    ScheduleBase();
    ScheduleBase(const Status status, const std::string& name, const int hour, const int minute, const bool isVisible);

    virtual void Exec();

    virtual bool CanExecute(const tm& nowTimeInfo);

    Status GetStatus() const;
    const std::string& GetName() const;
    int GetHour() const;
    int GetMinute() const;
    bool IsVisible() const;

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

} // IrrigationSystem

#endif // SCHEDULE_BASE_H_
// EOF
