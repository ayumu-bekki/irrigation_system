// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "gpio_control.h"

#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#include "logger.h"

namespace IrrigationSystem {
namespace GPIO {

/// Init GPIO (Output)
void InitOutput(const int32_t gpioNumber, const int32_t level)
{
    gpio_reset_pin(static_cast<gpio_num_t>(gpioNumber));
    gpio_set_direction(static_cast<gpio_num_t>(gpioNumber), GPIO_MODE_OUTPUT);

    SetLevel(gpioNumber, level);
}

/// Set GPIO Level (Output)
void SetLevel(const int32_t gpioNumber, const int32_t level)
{
    gpio_set_level(static_cast<gpio_num_t>(gpioNumber), level);
}

/// Get ADC Voltage (Input) [mV]
uint32_t GetAdcVoltage(const int32_t adcChannelNo, const int32_t round)
{
    adc_oneshot_unit_handle_t adcHandle;
    adc_oneshot_unit_init_cfg_t adcInitConfig = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&adcInitConfig, &adcHandle);

    adc_oneshot_chan_cfg_t adcConfig = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adcHandle, static_cast<adc_channel_t>(adcChannelNo), &adcConfig);

    adc_cali_handle_t adcCaliHandle = nullptr;
    adc_cali_line_fitting_config_t caliConfig = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
#if CONFIG_IDF_TARGET_ESP32
        .default_vref = ADC_CALI_LINE_FITTING_EFUSE_VAL_DEFAULT_VREF,
#endif
    };
    esp_err_t ret = adc_cali_create_scheme_line_fitting(&caliConfig, &adcCaliHandle);
    if (ret != ESP_OK) {
        return 0;
    }

    uint32_t sumVoltage = 0;
    for (int32_t i = 0; i < round; ++i) {
        int32_t adcValue = 0;
        adc_oneshot_read(adcHandle, static_cast<adc_channel_t>(adcChannelNo), reinterpret_cast<int*>(&adcValue));
        int32_t adcVoltage = 0;
        adc_cali_raw_to_voltage(adcCaliHandle, adcValue, reinterpret_cast<int*>(&adcVoltage));
        sumVoltage += adcVoltage;
    }

    // Shutdown
    adc_oneshot_del_unit(adcHandle);
    adc_cali_delete_scheme_line_fitting(adcCaliHandle);

    return sumVoltage / round;
}



} // GPIO
} // IrrigationSystem

// EOF
