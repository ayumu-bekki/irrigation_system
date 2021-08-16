#ifndef DEFINE_H_
#define DEFINE_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <driver/gpio.h>


namespace IrrigationSystem {

// Define

/// Tag for Log
static constexpr char TAG[] = "water";

/// TimeZone
static constexpr char LOCAL_TIME_ZONE[] = "JST-9";

/// NtpServer
static constexpr char NTP_SERVER_ADDRESS[] = "ntp.nict.jp";

/// GPIO No
static constexpr gpio_num_t NORMAL_OPERATION_SIGNAL_GPIO = GPIO_NUM_14;
static constexpr gpio_num_t RELAY_SIGNAL_GPIO = GPIO_NUM_12;

} // IrrigationSystem

#endif // DEFINE_H_
// EOF
