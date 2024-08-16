#ifndef GPIO_H_
#define GPIO_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <stdint.h>

namespace IrrigationSystem::GPIO {

/// Init GPIO (Output)
void InitOutput(const int32_t gpio_num, const int32_t level = 0);

/// Set GPIO Level (Output)
void SetLevel(const int32_t gpio_num, const int32_t level);

/// Init GPIO (Input)
void InitInput(const int32_t gpio_num);

/// Init ADC (Input)
void InitAdc(const int32_t adc_channel_no);

/// Get ADC Voltage (Input) [mV]
uint32_t GetAdcVoltage(const int32_t adc_channel_no, const int32_t round = 1);

}  // namespace IrrigationSystem::GPIO

#endif  // GPIO_H_

// EOF