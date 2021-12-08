#ifndef IRRIGATION_INTERFACE_H_
#define IRRIGATION_INTERFACE_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <chrono>

namespace IrrigationSystem {

class ScheduleManager;
class WeatherForecast;
class WateringSetting;

class IrrigationInterface
{
public:
    virtual ~IrrigationInterface() {}
    
    virtual void ValveAddOpenSecond(const int second) = 0;
    virtual void ValveResetTimer() = 0;
    virtual void ValveForce(const bool isOpen) = 0;
    virtual std::time_t ValveCloseEpoch() const = 0;

    virtual ScheduleManager& GetScheduleManager() = 0;
    virtual WeatherForecast& GetWeatherForecast() = 0;
    virtual WateringSetting& GetWateringSetting() = 0;
    virtual void SaveLastWateringEpoch(const std::time_t wateringEpoch) = 0;
    virtual std::time_t GetLastWateringEpoch() const = 0;

};

} // IrrigationSystem

#endif // IRRIGATION_INTERFACE_H_
// EOF
