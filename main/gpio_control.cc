// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "gpio_control.h"

#include <driver/gpio.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>

#include "logger.h"

namespace IrrigationSystem {
namespace GPIO {

/// Init GPIO (Output)
void InitOutput(const int32_t gpio_num, const int32_t level) {
  gpio_reset_pin(static_cast<gpio_num_t>(gpio_num));
  gpio_set_direction(static_cast<gpio_num_t>(gpio_num), GPIO_MODE_OUTPUT);

  SetLevel(gpio_num, level);
}

/// Set GPIO Level (Output)
void SetLevel(const int32_t gpio_num, const int32_t level) {
  gpio_set_level(static_cast<gpio_num_t>(gpio_num), level);
}

/// Init GPIO (Input)
void InitInput(const int32_t gpio_num) {
  gpio_reset_pin(static_cast<gpio_num_t>(gpio_num));
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << gpio_num),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };
  gpio_config(&io_conf);
}

/// Get ADC Voltage (Input) [mV]
uint32_t GetAdcVoltage(const int32_t adc_channel_no, const int32_t round) {
  adc_oneshot_unit_handle_t adc_handle;
  adc_oneshot_unit_init_cfg_t adcInitConfig = {
      .unit_id = ADC_UNIT_1,
      .clk_src = static_cast<adc_oneshot_clk_src_t>(ADC_DIGI_CLK_SRC_DEFAULT),
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  adc_oneshot_new_unit(&adcInitConfig, &adc_handle);

  adc_oneshot_chan_cfg_t adc_config = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
  };
  adc_oneshot_config_channel(
      adc_handle, static_cast<adc_channel_t>(adc_channel_no), &adc_config);

  adc_cali_handle_t adc_cali_handle = nullptr;
  adc_cali_line_fitting_config_t cali_config = {
      .unit_id = ADC_UNIT_1,
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
#if CONFIG_IDF_TARGET_ESP32
      .default_vref = ADC_CALI_LINE_FITTING_EFUSE_VAL_DEFAULT_VREF,
#endif
  };
  esp_err_t ret =
      adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle);
  if (ret != ESP_OK) {
    return 0;
  }

  uint32_t sum_voltage = 0;
  for (int32_t i = 0; i < round; ++i) {
    int32_t adcValue = 0;
    adc_oneshot_read(adc_handle, static_cast<adc_channel_t>(adc_channel_no),
                     reinterpret_cast<int*>(&adcValue));
    int32_t adcVoltage = 0;
    adc_cali_raw_to_voltage(adc_cali_handle, adcValue,
                            reinterpret_cast<int*>(&adcVoltage));
    sum_voltage += adcVoltage;
  }

  // Shutdown
  adc_oneshot_del_unit(adc_handle);
  adc_cali_delete_scheme_line_fitting(adc_cali_handle);

  return sum_voltage / round;
}

}  // namespace GPIO
}  // namespace IrrigationSystem

// EOF
