#ifndef IRRIGATION_CONTROLLER_H_
#define IRRIGATION_CONTROLLER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <cstdint>
#include <memory>

#include "irrigation_interface.h"
#include "schedule_manager.h"
#include "valve_executor.h"
#include "valve_task.h"
#include "voltage_check_task.h"
#include "water_flow_sensor.h"
#include "water_level_check_task.h"
#include "watering_record.h"
#include "watering_setting.h"
#include "weather_forecast.h"
#include "wifi_manager.h"
#include "mqtt.h"
#include "mqtt_publish_status_task.h"

namespace IrrigationSystem {

/// IrrigationController
class IrrigationController final
    : public IrrigationInterface,
      public std::enable_shared_from_this<IrrigationController> {
 public:
  IrrigationController();
  ~IrrigationController();

  /// Start
  void Start();

 public:
  /// (IrrigationInterface:override)
  void AddValveExecutor(ValveExecutorSharedPtr executor) override;

  /// (IrrigationInterface:override)
  ValveExecutorSharedPtr GetCurrentValveExecutor() override;

  /// (IrrigationInterface:override)
  void ForceStopValve() override;

  /// (IrrigationInterface:override)
  const ScheduleManagerWeakPtr GetScheduleManager() override;

  /// (IrrigationInterface:override)
  WeatherForecast& GetWeatherForecast() override;

  /// (IrrigationInterface:override)
  WateringSetting& GetWateringSetting() override;

  /// (IrrigationInterface:override)
  const WateringSetting& GetWateringSetting() const override;

  /// (IrrigationInterface:override)
  void SaveLastWateringEpoch(const std::time_t watering_epoch) override;

  /// (IrrigationInterface:override)
  std::time_t GetLastWateringEpoch() const override;

  /// (IrrigationInterface:override)
  float GetMainVoltage() const override;

  /// (IrrigationInterface:override)
  void CheckWaterLevel() override;

  /// (IrrigationInterface:override)
  float GetWaterLevel() const override;

  /// (IrrigationInterface:override)
  void StartWaterFlowMeasurement() override;

  /// (IrrigationInterface:override)
  int32_t FinishWaterFlowMeasurement() override;

  /// (IrrigationInterface:override)
  int32_t GetWaterFlowHz() override;

  /// (IrrigationInterface:override)
  std::time_t GetSystemBootTime() override;
 
  /// (IrrigationInterface:override)
  void PublishMQTTMessage(const std::string& topic, const std::string& data) override;

 private:
#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  void EventMQTTConnect();
  void EventMQTTDisconnect();
#endif
 
 private:
  WifiManager wifi_manager_;
  ValveTaskUniquePtr valve_task_;
  ScheduleManagerSharedPtr schedule_manager_;
  WeatherForecast weather_forecast_;
  WateringSetting watering_setting_;
  WateringRecord watering_record_;

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  VoltageCheckTaskUniquePtr voltage_check_task_;
#endif

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  WaterLevelCheckTaskUniquePtr water_level_check_task_;
#endif

#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  WaterFlowSensor water_flow_sensor_;
#endif

#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  MQTTClient mqtt_client_;
  MQTTPublishStatusTaskUniquePtr mqtt_publish_status_task_;
#endif

  std::time_t system_boot_time_;
};

}  // namespace IrrigationSystem

#endif  // IRRIGATION_CONTROLLER_H_
// EOF
