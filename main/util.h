#ifndef UTIL_H_
#define UTIL_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp
// Utilities

// Include ----------------------
#include <string>


namespace IrrigationSystem {
namespace Util {

/// Sleep 
void SleepMillisecond(const unsigned int sleepMillisecond);

/// SyncTime
void SyncSntpObtainTime();

/// GetLocalTime
tm GetLocalTime();

/// Get Now Date String (yyyy/dd/mm hh:mm:ss)
std::string GetNowTimeStr();

/// Print Now Date (yyyy/dd/mm hh:mm:ss)
void PrintNow();

/// Initialie Local Time Zone
void InitTimeZone();

} // Util
} // IrrigationSystem

#endif // UTIL_H_
// EOF
