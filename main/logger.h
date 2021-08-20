#ifndef LOGGER_H_
#define LOGGER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------

#include <esp_log.h>

namespace IrrigationSystem {

/// Application Log Tag
static constexpr char TAG[] = "Irrigation System";

namespace Logger {

void InitializeLogLevel();

} // Logger
} // IrrigationSystem

#endif // DEFINE_H_
// EOF
