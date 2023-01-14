#ifndef PWM_H_
#define PWM_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <driver/ledc.h>
#include <cmath>

namespace IrrigationSystem {

class Pwm final
{
public:
    Pwm();

    void Initialize(const ledc_channel_t channelNo, const gpio_num_t gpioNo);

    void SetRate(const float rate);

private:
    ledc_channel_t m_channelNo;
    ledc_mode_t m_ledcMode;
};

} // IrrigationSystem

#endif // PWM_H_ 
// EOF
