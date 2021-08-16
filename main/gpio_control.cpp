// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "gpio_control.h"

#include <driver/gpio.h>

namespace IrrigationSystem {
namespace GPIO {

/// Init GPIO (Output)
void InitOutput(const int gpioNumber, const int level)
{
    gpio_pad_select_gpio(static_cast<gpio_num_t>(gpioNumber));
    gpio_set_direction(static_cast<gpio_num_t>(gpioNumber), GPIO_MODE_OUTPUT);
    
    SetLevel(gpioNumber, level);
}

/// Set GPIO Level (Output)
void SetLevel(const int gpioNumber, const int level)
{
    gpio_set_level(static_cast<gpio_num_t>(gpioNumber), level);
}

} // GPIO
} // IrrigationSystem

// EOF
