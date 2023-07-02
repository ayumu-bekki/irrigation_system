#ifndef IRRIGATION_INTERFACE_H_
#define IRRIGATION_INTERFACE_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <chrono>
#include <memory>

namespace IrrigationSystem {

class ScheduleManager;
using ScheduleManagerWeakPtr = std::weak_ptr<ScheduleManager>;
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

    virtual const ScheduleManagerWeakPtr GetScheduleManager() = 0;
    virtual WeatherForecast& GetWeatherForecast() = 0;
    virtual WateringSetting& GetWateringSetting() = 0;
    virtual const WateringSetting& GetWateringSetting() const = 0;
    virtual void SaveLastWateringEpoch(const std::time_t wateringEpoch) = 0;
    virtual std::time_t GetLastWateringEpoch() const = 0;
    virtual float GetMainVoltage() const = 0;
    virtual void CheckWaterLevel() = 0;
    virtual float GetWaterLevel() const = 0;
};

using IrrigationInterfaceSharedPtr = std::shared_ptr<IrrigationInterface>;
using IrrigationInterfaceConstSharedPtr = std::shared_ptr<const IrrigationInterface>;
using IrrigationInterfaceWeakPtr = std::weak_ptr<IrrigationInterface>;
using IrrigationInterfaceConstWeakPtr = std::weak_ptr<const IrrigationInterface>;

} // IrrigationSystem

#endif // IRRIGATION_INTERFACE_H_
// EOF
