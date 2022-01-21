// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "gpio_control.h"

#include <esp_adc_cal.h>
#include <driver/adc.h>
#include <driver/gpio.h>

#include "logger.h"

namespace IrrigationSystem {
namespace GPIO {

/// Init GPIO (Output)
void InitOutput(const int32_t gpioNumber, const int32_t level)
{
    gpio_pad_select_gpio(static_cast<gpio_num_t>(gpioNumber));
    gpio_set_direction(static_cast<gpio_num_t>(gpioNumber), GPIO_MODE_OUTPUT);
    
    SetLevel(gpioNumber, level);
}

/// Set GPIO Level (Output)
void SetLevel(const int32_t gpioNumber, const int32_t level)
{
    gpio_set_level(static_cast<gpio_num_t>(gpioNumber), level);
}

/// Init ADC (Input)
void InitAdc(const int32_t adcChannelNo)
{
    adc_power_acquire();
    adc_gpio_init(ADC_UNIT_1, static_cast<adc_channel_t>(adcChannelNo));
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(static_cast<adc1_channel_t>(adcChannelNo), ADC_ATTEN_DB_11);
}

/// Get ADC Voltage (Input) [mV]
uint32_t GetAdcVoltage(const int32_t adcChannelNo, const int32_t round)
{
    static const uint32_t DEFAULT_VREF = 1100;
    esp_adc_cal_characteristics_t adcChar;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adcChar);

    uint32_t sumVoltage = 0;
    for (int32_t i = 0; i < round; ++i) {
        uint32_t adcVoltage = 0;
        esp_adc_cal_get_voltage(static_cast<adc_channel_t>(adcChannelNo), &adcChar, &adcVoltage);
        sumVoltage += adcVoltage;
    }
    return sumVoltage / round;
}



} // GPIO
} // IrrigationSystem

// EOF
