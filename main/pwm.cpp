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
        constexpr double ESP32_CLOCK = 12.5e-9; // 80MHz = 12.5μs

        ledc_timer_bit_t ledcDutyBit = LEDC_TIMER_BIT_MAX;
        for (int32_t bit = (ledcDutyBit - 1); bit >= LEDC_TIMER_1_BIT; --bit) {
            const int32_t targetFreq = (1.0 / (ESP32_CLOCK * std::pow(2, bit)));
            ledcDutyBit = static_cast<ledc_timer_bit_t>(bit);
            if (frequency < targetFreq) {
                break;
            }
        }
        return ledcDutyBit;
    }
}

namespace IrrigationSystem {

Pwm::Pwm()
    :m_channelNo(LEDC_CHANNEL_0)
    ,m_ledcMode(LEDC_LOW_SPEED_MODE)
    ,m_ledcDutyBit(LEDC_TIMER_1_BIT)
{}

void Pwm::Initialize(const ledc_channel_t channelNo, const ledc_timer_t ledcTimer, const gpio_num_t gpioNo, const uint32_t frequency)
{
    m_channelNo = channelNo;
    m_ledcDutyBit = CalcFrequencyToBit(frequency);

    ESP_LOGI(TAG, "PWD INIT Channel:%d Timer:%d Bit:%d Freq:%d gpio:%d", m_channelNo, ledcTimer, m_ledcDutyBit, frequency, gpioNo);

    // Prepare and then apply the LEDC PWM timer configuration
    const ledc_timer_config_t ledc_timer = {
        .speed_mode       = m_ledcMode,
        .duty_resolution  = m_ledcDutyBit,
        .timer_num        = ledcTimer,
        .freq_hz          = frequency,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Prepare and then apply the LEDC PWM channel configuration
    const ledc_channel_config_t ledc_channel = {
        .gpio_num       = gpioNo,
        .speed_mode     = m_ledcMode,
        .channel        = m_channelNo,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = ledcTimer,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .flags {}
    };
    ledc_channel_config(&ledc_channel);
}

void Pwm::SetRate(const float rate)
{
    // rate to duty
    const int32_t ledc_duty = (std::pow(2, static_cast<int32_t>(m_ledcDutyBit)) - 1) * rate;

    ESP_LOGI(TAG, "PWD RATE rate:%0.2f bitrate:%d ledcbit:%d", rate, ledc_duty, m_ledcDutyBit);

    ledc_set_duty(m_ledcMode, m_channelNo, ledc_duty);
    ledc_update_duty(m_ledcMode, m_channelNo);
}

} // IrrigationSystem

// EOF

