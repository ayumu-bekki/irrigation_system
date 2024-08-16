#ifndef UTIL_H_
#define UTIL_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp
// Utilities

// Include ----------------------
#include <chrono>
#include <string>
#include <vector>

namespace IrrigationSystem {
namespace Util {

/// Sleep
void SleepMillisecond(const unsigned int sleep_miliseconds);

/// SyncTime
void SyncSntpObtainTime();

/// GetEpoch
std::time_t GetEpoch();

/// Epoch To Local Time
std::tm EpochToLocalTime(std::time_t epoch);

/// GetLocalTime
std::tm GetLocalTime();

/// Get Time To String (yyyy/dd/mm hh:mm:ss)
std::string TimeToStr(const std::tm& time_info);

/// Get Now Date String (yyyy/dd/mm hh:mm:ss)
std::string GetNowTimeStr();

/// Initialie Local Time Zone
void InitTimeZone();

/// Gregorian calendar to Modified Julian Date(修正ユリウス日)
int32_t GregToMJD(const std::tm& time_info);

/// Get ChronoMinutes from hours and minutes.
std::chrono::minutes GetChronoHourMinutes(const std::tm& time_info);

/// Split Text
std::vector<std::string> SplitString(const std::string& str, const char delim);

/// GetVoltage
float GetVoltage();

/// Get Original Voltage Divider Resistor
float GetOriginalVoltageFromDividerRegister(
    const uint32_t output_volatage, const float top_resistance_value,
    const float bottom_registance_value);

}  // namespace Util
}  // namespace IrrigationSystem

#endif  // UTIL_H_
// EOF
