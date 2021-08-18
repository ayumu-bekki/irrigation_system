// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include "schedule_base.h"

#include <esp_log.h>

#include "define.h"

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

const std::string& ScheduleBase::GetName() const
{
    return m_Name;
}

void ScheduleBase::Exec()
{
    ESP_LOGW(TAG, "Schedule Exec - Default Executer. %02d:%02d", m_Hour, m_Minute);
    m_Status = STATUS_EXECUTED;
}

bool ScheduleBase::CanExecute(const tm& nowTimeInfo)
{
    return m_Status == STATUS_WAIT &&
           m_Hour <= nowTimeInfo.tm_hour && 
           m_Minute <= nowTimeInfo.tm_min;
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