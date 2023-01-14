// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "pwm.h"

#include "logger.h"

namespace IrrigationSystem {

Pwm::Pwm()
    :m_channelNo()
    ,m_ledcMode(LEDC_LOW_SPEED_MODE)
{}

void Pwm::Initialize(const ledc_channel_t channelNo, const gpio_num_t gpioNo)
{
    m_channelNo = channelNo;

    constexpr ledc_timer_bit_t LEDC_DUTY_RES = LEDC_TIMER_10_BIT;
    //constexpr uint32_t LEDC_FREQUENCY = 60000; // 60kHz
    constexpr uint32_t LEDC_FREQUENCY = 2000; // 2kHz
    constexpr ledc_timer_t LEDC_TIMER = LEDC_TIMER_0;

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
    // 16bit  1.2207kHz
    // 18bit  305Hz


    // Prepare and then apply the LEDC PWM timer configuration
    const ledc_timer_config_t ledc_timer = {
        .speed_mode       = m_ledcMode,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Prepare and then apply the LEDC PWM channel configuration
    const ledc_channel_config_t ledc_channel = {
        .gpio_num       = gpioNo,
        .speed_mode     = m_ledcMode,
        .channel        = m_channelNo,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .flags {}
    };
    ledc_channel_config(&ledc_channel);
}

void Pwm::SetRate(const float rate)
{
    const int32_t ledc_duty = (std::pow(2, 10) - 1) * rate;
    ledc_set_duty(m_ledcMode, m_channelNo, ledc_duty);
    ledc_update_duty(m_ledcMode, m_channelNo);
}

} // IrrigationSystem

// EOF

