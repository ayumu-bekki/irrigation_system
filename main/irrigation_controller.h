#ifndef IRRIGATION_CONTROLLER_H_
#define IRRIGATION_CONTROLLER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_interface.h"
#include "wifi_manager.h"
#include "valve_task.h"
#include "schedule_manager.h"
#include "weather_forecast.h"
#include "watering_setting.h"
#include "watering_record.h"
#include "voltage_check_task.h"

namespace IrrigationSystem {

/// IrrigationController
class IrrigationController final : public IrrigationInterface
{
public:
    IrrigationController();

    /// Start
    void Start();

public:
    /// (IrrigationInterface:override)
    void ValveAddOpenSecond(const int second) override;

    /// (IrrigationInterface:override)
    void ValveResetTimer() override;

    /// (IrrigationInterface:override)
    void ValveForce(const bool isOpen) override;

    /// (IrrigationInterface:override)
    std::time_t ValveCloseEpoch() const override;

    /// (IrrigationInterface:override)
    ScheduleManager& GetScheduleManager() override;

    /// (IrrigationInterface:override)
    WeatherForecast& GetWeatherForecast() override;

    /// (IrrigationInterface:override)
    WateringSetting& GetWateringSetting() override;

    /// (IrrigationInterface:override)
    void SaveLastWateringEpoch(const std::time_t wateringEpoch);

    /// (IrrigationInterface:override)
    std::time_t GetLastWateringEpoch() const;

    /// (IrrigationInterface:override)
    float GetMainVoltage() const;

private:
    WifiManager m_WifiManager;
    ValveTask m_ValveTask;
    ScheduleManager m_ScheduleManager;
    WeatherForecast m_WeatherForecast;
    WateringSetting m_WateringSetting;
    WateringRecord m_WateringRecord;

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
    VoltageCheckTask m_VoltageCheckTask;
#endif
};

} // IrrigationSystem

#endif // IRRIGATION_CONTROLLER_H_
// EOF
