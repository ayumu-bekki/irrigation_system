// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"

#include <chrono>
#include "util.h"

namespace IrrigationSystem {

ScheduleBase::ScheduleBase()
    :m_Status(STATUS_NONE)
    ,m_Name()
    ,m_Hour(0)
    ,m_Minute(0)
    ,m_IsVisible(false)
{}

ScheduleBase::ScheduleBase(const ScheduleBase::Status status, const std::string& name, const int hour, const int minute, const bool isVisible)
    :m_Status(status)
    ,m_Name(name)
    ,m_Hour(hour)
    ,m_Minute(minute)
    ,m_IsVisible(isVisible)
{}

ScheduleBase::Status ScheduleBase::GetStatus() const
{
    return m_Status;
}

void ScheduleBase::SetStatus(const ScheduleBase::Status status)
{
    m_Status = status;
}

void ScheduleBase::DisableExpired(const std::tm& timeInfo)
{
    if (CanExecute(timeInfo)) {
        m_Status = STATUS_DISABLE;
    }
}

const std::string& ScheduleBase::GetName() const
{
    return m_Name;
}

bool ScheduleBase::CanExecute(const std::tm& timeInfo)
{
    const std::chrono::minutes nowChrono = Util::GetChronoHourMinutes(timeInfo);
    return m_Status == STATUS_WAIT && GetChronoMinutes() < nowChrono;
}

int ScheduleBase::GetHour() const
{
    return m_Hour;
}

int ScheduleBase::GetMinute() const
{
    return m_Minute;
}

bool ScheduleBase::IsVisible() const
{
    return m_IsVisible;
}

std::chrono::minutes ScheduleBase::GetChronoMinutes() const 
{
    return std::chrono::hours(m_Hour) + std::chrono::minutes(m_Minute);
}

int ScheduleBase::GetDiffTime() const 
{
    return GetChronoMinutes().count();
}

const char* ScheduleBase::StatusToStr(const ScheduleBase::Status status)
{
    static constexpr char* EmptyStr = (char*)"";
    if (status < ScheduleBase::STATUS_NONE || ScheduleBase::MAX_STATUS <= status) {
        return EmptyStr;
    }

    static constexpr char* StatusStrTbl[ScheduleBase::MAX_STATUS] = {
        (char*)"None",
        (char*)"Wait",
        (char*)"Executed",
        (char*)"Disable",
    };
    return StatusStrTbl[status];
}   

const char* ScheduleBase::StatusToRecordStyle(const ScheduleBase::Status status)
{
    static constexpr char* EmptyStr = (char*)"";
    if (status < ScheduleBase::STATUS_NONE || ScheduleBase::MAX_STATUS <= status) {
        return EmptyStr;
    }

    static constexpr char* StatusRecordStyleTbl[ScheduleBase::MAX_STATUS] = {
        (char*)"schedule_none",
        (char*)"schedule_wait",
        (char*)"schedule_executable",
        (char*)"schedule_disable",
    };
    return StatusRecordStyleTbl[status];
}



} // IrrigationSystem

// EOF
