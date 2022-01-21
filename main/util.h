#ifndef UTIL_H_
#define UTIL_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp
// Utilities

// Include ----------------------
#include <string>
#include <vector>
#include <chrono>

namespace IrrigationSystem {
namespace Util {

/// Sleep 
void SleepMillisecond(const unsigned int sleepMillisecond);

/// SyncTime
void SyncSntpObtainTime();

/// GetEpoch
std::time_t GetEpoch();

/// Epoch To Local Time
std::tm EpochToLocalTime(std::time_t epoch);

/// GetLocalTime
std::tm GetLocalTime();

/// Get Time To String (yyyy/dd/mm hh:mm:ss)
std::string TimeToStr(const std::tm& timeInfo);

/// Get Now Date String (yyyy/dd/mm hh:mm:ss)
std::string GetNowTimeStr();

/// Initialie Local Time Zone
void InitTimeZone();

/// Gregorian calendar to Modified Julian Date(修正ユリウス日)
int32_t GregToMJD(const std::tm& timeInfo);

/// Get ChronoMinutes from hours and minutes.
std::chrono::minutes GetChronoHourMinutes(const std::tm& timeInfo);

/// Split Text
std::vector<std::string> SplitString(const std::string &str, const char delim);

/// Get Original Voltage Divider Resistor
float GetOriginalVoltageFromDividerRegister(const uint32_t outputVoltage, const float topResistanceValue, const float bottomRegistanceValue);

} // Util
} // IrrigationSystem

#endif // UTIL_H_
// EOF
