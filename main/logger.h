#ifndef LOGGER_H_
#define LOGGER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE // Required before test.h is include
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
