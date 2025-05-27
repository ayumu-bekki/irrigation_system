#ifndef PWM_H_
#define PWM_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <driver/ledc.h>

#include <cmath>

namespace IrrigationSystem {

class Pwm final {
 public:
  Pwm();

  void Initialize(const ledc_channel_t channel_no,
                  const ledc_timer_t ledc_timer_num, const gpio_num_t gpio_no,
                  const uint32_t frequency);

  void SetRate(const float rate);

 private:
  ledc_channel_t channel_no_;
  ledc_mode_t ledc_mode_;
  ledc_timer_bit_t ledc_duty_bit_;
};

}  // namespace IrrigationSystem

#endif  // PWM_H_
// EOF
