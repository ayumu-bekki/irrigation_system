// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "pwm.h"

#include "logger.h"

namespace {
ledc_timer_bit_t CalcFrequencyToBit(const uint32_t frequency) {
  // 1 / (12.5e-9 * 2 ^ bit)
  // ex) 1 / (12.5e-9 * 2 ^ 6) = 1.25MHz
  // 1bit  40MHz
  // 2bit  20MHz
  // 4bit  5MHz
  // 6bit  1.25MHz
  // 8bit  312.5kHz
  // 10bit  78.125kHz
  // 12bit  19.53125kHz
  // 14bit  4.8828kHz
  constexpr double ESP32_CLOCK = 12.5e-9;  // 80MHz = 12.5μs

  ledc_timer_bit_t ledc_duty_bit = LEDC_TIMER_BIT_MAX;
  for (int32_t bit = (ledc_duty_bit - 1); bit >= LEDC_TIMER_1_BIT; --bit) {
    const int32_t target_freq = (1.0 / (ESP32_CLOCK * std::pow(2, bit)));
    ledc_duty_bit = static_cast<ledc_timer_bit_t>(bit);
    if (frequency < target_freq) {
      break;
    }
  }
  return ledc_duty_bit;
}
}  // namespace

namespace IrrigationSystem {

Pwm::Pwm()
    : channel_no_(LEDC_CHANNEL_0),
      ledc_mode_(LEDC_LOW_SPEED_MODE),
      ledc_duty_bit_(LEDC_TIMER_1_BIT) {}

void Pwm::Initialize(const ledc_channel_t channel_no,
                     const ledc_timer_t ledc_timer_num,
                     const gpio_num_t gpio_no, const uint32_t frequency) {
  channel_no_ = channel_no;
  ledc_duty_bit_ = CalcFrequencyToBit(frequency);

  ESP_LOGI(TAG, "PWD INIT Channel:%d Timer:%d Bit:%d Freq:%d gpio:%d",
           channel_no_, ledc_timer_num, ledc_duty_bit_, frequency, gpio_no);

  // Prepare and then apply the LEDC PWM timer configuration
  const ledc_timer_config_t ledc_timer_cfg = {.speed_mode = ledc_mode_,
                                              .duty_resolution = ledc_duty_bit_,
                                              .timer_num = ledc_timer_num,
                                              .freq_hz = frequency,
                                              .clk_cfg = LEDC_AUTO_CLK,
                                              .deconfigure = false};
  ledc_timer_config(&ledc_timer_cfg);

  // Prepare and then apply the LEDC PWM channel configuration
  const ledc_channel_config_t ledc_channel = {.gpio_num = gpio_no,
                                              .speed_mode = ledc_mode_,
                                              .channel = channel_no_,
                                              .intr_type = LEDC_INTR_DISABLE,
                                              .timer_sel = ledc_timer_num,
                                              .duty = 0,  // Set duty to 0%
                                              .hpoint = 0,
                                              .flags{}};
  ledc_channel_config(&ledc_channel);
}

void Pwm::SetRate(const float rate) {
  // rate to duty
  const int32_t ledc_duty =
      (std::pow(2, static_cast<int32_t>(ledc_duty_bit_)) - 1) * rate;

  ESP_LOGI(TAG, "PWD RATE rate:%0.2f bitrate:%d ledcbit:%d", rate, ledc_duty,
           ledc_duty_bit_);

  ledc_set_duty(ledc_mode_, channel_no_, ledc_duty);
  ledc_update_duty(ledc_mode_, channel_no_);
}

}  // namespace IrrigationSystem

// EOF
