// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "gpio_control.h"


namespace IrrigationSystem {
namespace GPIO {

/// Init GPIO (Output)
void InitOutput(const gpio_num_t gpioNumber, const int level)
{
    gpio_pad_select_gpio(gpioNumber);
    gpio_set_direction(gpioNumber, GPIO_MODE_OUTPUT);
    
    SetLevel(gpioNumber, level);
}

/// Set GPIO Level (Output)
void SetLevel(const gpio_num_t gpioNumber, const int level)
{
    gpio_set_level(gpioNumber, level);
}

} // GPIO
} // IrrigationSystem

// EOF
