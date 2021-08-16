#ifndef GPIO_H_
#define GPIO_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp


namespace IrrigationSystem {
namespace GPIO {

/// Init GPIO (Output)
void InitOutput(const int gpioNumber, const int level = 0);

/// Set GPIO Level (Output)
void SetLevel(const int gpioNumber, const int level);

} // GPIO
} // IrrigationSystem

#endif // GPIO_H_
// EOF
