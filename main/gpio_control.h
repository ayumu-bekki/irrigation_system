#ifndef GPIO_H_
#define GPIO_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <stdint.h>

namespace IrrigationSystem {
namespace GPIO {

/// Init GPIO (Output)
void InitOutput(const int32_t gpioNumber, const int32_t level = 0);

/// Set GPIO Level (Output)
void SetLevel(const int32_t gpioNumber, const int32_t level);

/// Init ADC (Input)
void InitAdc(const int32_t adcChannelNo);

/// Get ADC Voltage (Input) [mV]
uint32_t GetAdcVoltage(const int32_t adcChannelNo, const int32_t round = 1);


} // GPIO
} // IrrigationSystem

#endif // GPIO_H_
// EOF
