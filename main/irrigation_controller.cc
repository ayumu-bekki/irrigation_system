// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_controller.h"

#include <esp_system.h>
#include <nvs_flash.h>

#include <memory>

#include "file_system.h"
#include "gpio_control.h"
#include "httpd_server_task.h"
#include "logger.h"
#include "management_task.h"
#include "util.h"
#include "version.h"
#include "watering_button_task.h"

namespace IrrigationSystem {

IrrigationController::IrrigationController()
    : wifi_manager_(),
      valve_task_(),
      schedule_manager_(),
      weather_forecast_(),
      watering_setting_(),
      watering_record_()
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
      ,
      voltage_check_task_()
#endif
#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
      ,
      water_level_check_task_()
#endif
#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
      ,
      water_flow_sensor_()
#endif
#if CONFIG_IS_ENABLE_MQTT_PUBLISH
      ,
      mqtt_client_(),
      mqtt_publish_status_task_()
#endif
      ,
      system_boot_time_(0) {
}

IrrigationController::~IrrigationController() = default;

void IrrigationController::Start() {
  // Initialize Log
  Logger::InitializeLogLevel();

  ESP_LOGI(TAG, "Startup Irrigation System. Version:%s", GIT_VERSION);

  // Monitoring LED Init And ON
  GPIO::InitOutput(CONFIG_MONITORING_OUTPUT_GPIO_NO, 1);

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  // VoltageCheck GPIO Init
  GPIO::InitOutput(CONFIG_VAOLTAGE_CHECK_OUTPUT_GPIO_NO, 0);
#endif

#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  // WaterFlow GPIO Init
  GPIO::InitOutput(CONFIG_WATER_FLOW_OUTPUT_GPIO_NO, 0);
#endif

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGE(TAG, "NVS Flash Error");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // WiFi
  wifi_manager_.Connect();

  // Timezone init
  Util::InitTimeZone();

  // Sync NTP
  Util::SyncSntpObtainTime();

  // Mount File System
  FileSystem::Mount();

  // Set System Boot Time
  system_boot_time_ = Util::GetEpoch();

  // Read Setting Data
  std::string raw_setting_data;
  if (WateringSetting::Load(raw_setting_data)) {
    if (!watering_setting_.SetSettingData(raw_setting_data)) {
      ESP_LOGE(TAG, "Invlaid Setting data");
    }
  } else {
    ESP_LOGI(TAG, "Failed Load Setting File");
  }

  // Read Last Watering Date
  watering_record_.Load();

  // MainTask
  ManagementTask management_task(weak_from_this());
  HttpdServerTask httpd_server_task(weak_from_this());
  WateringButtonTask watering_button_task(weak_from_this());
  schedule_manager_ = std::make_shared<ScheduleManager>(weak_from_this());
  valve_task_ = std::make_unique<ValveTask>(weak_from_this());

  management_task.Start();
  httpd_server_task.Start();
  watering_button_task.Start();

  if (valve_task_) {
    valve_task_->Start();
  }

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  voltage_check_task_ = std::make_unique<VoltageCheckTask>(weak_from_this());
  if (voltage_check_task_) {
    voltage_check_task_->Start();
  }
#endif

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  water_level_check_task_ =
      std::make_unique<WaterLevelCheckTask>(weak_from_this());
  if (water_level_check_task_) {
    water_level_check_task_->Start();
  }
#endif

#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  water_flow_sensor_.Start();
#endif

#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  mqtt_client_.SetBrokerHost(CONFIG_MQTT_BROKER_HOST);
  mqtt_client_.SetConnectEvent(
      std::bind(&IrrigationController::EventMQTTConnect, this));
  mqtt_client_.SetDisconnectEvent(
      std::bind(&IrrigationController::EventMQTTDisconnect, this));
#if 0
  mqtt_client_.SetWillMessage("irrigation_system/" CONFIG_MQTT_DEVICE_TOPIC_NAME
                              "/status",
                              "{\"status\":\"close\"}");
#endif
  mqtt_client_.Start();

  mqtt_publish_status_task_ =
      std::make_unique<MQTTPublishStatusTask>(weak_from_this());
  mqtt_publish_status_task_->Start();
#endif

  // Monitoring LED Off
  GPIO::SetLevel(CONFIG_MONITORING_OUTPUT_GPIO_NO, 0);

  ESP_LOGI(TAG, "Activation Complete Irrigation System.");

  // vTaskStartSchedule() is already called by ESP-IDF before app_main. Infinite
  // loop thereafter.
  while (true) {
    Util::SleepMillisecond(1000);
  }
}

void IrrigationController::AddValveExecutor(ValveExecutorSharedPtr executor) {
  if (valve_task_) {
    valve_task_->AddExecutor(std::move(executor));
  }
}

ValveExecutorSharedPtr IrrigationController::GetCurrentValveExecutor() {
  if (valve_task_) {
    return valve_task_->GetCurrentExecutor();
  }
  return nullptr;
}

void IrrigationController::ForceStopValve() {
  if (valve_task_) {
    valve_task_->ForceStop();
  }
}

const ScheduleManagerWeakPtr IrrigationController::GetScheduleManager() {
  return schedule_manager_;
}

WeatherForecast& IrrigationController::GetWeatherForecast() {
  return weather_forecast_;
}

WateringSetting& IrrigationController::GetWateringSetting() {
  return watering_setting_;
}

const WateringSetting& IrrigationController::GetWateringSetting() const {
  return watering_setting_;
}

void IrrigationController::SaveLastWateringEpoch(
    const std::time_t watering_epoch) {
  watering_record_.SetLastWateringEpoch(watering_epoch);
  watering_record_.Save();
}

std::time_t IrrigationController::GetLastWateringEpoch() const {
  return watering_record_.GetLastWateringEpoch();
}

float IrrigationController::GetMainVoltage() const {
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  if (voltage_check_task_) {
    return voltage_check_task_->GetVoltage();
  }
#endif
  return 0.0f;
}

void IrrigationController::CheckWaterLevel() {
#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  if (water_level_check_task_) {
    water_level_check_task_->Check();
  }
#endif
}

float IrrigationController::GetWaterLevel() const {
#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  if (water_level_check_task_) {
    return water_level_check_task_->GetWaterLevel();
  }
  return 0.0f;
#else
  return -1.0f;
#endif
}

void IrrigationController::StartWaterFlowMeasurement() {
#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  water_flow_sensor_.StartMeasurement();
#endif
}

int32_t IrrigationController::FinishWaterFlowMeasurement() {
#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  return water_flow_sensor_.FinishMeasurement();
#else
  return 0;
#endif
}

int32_t IrrigationController::GetWaterFlowHz() {
#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
  return water_flow_sensor_.GetSensorHz();
#else
  return 0;
#endif
}

std::time_t IrrigationController::GetSystemBootTime() {
  return system_boot_time_;
}

bool IrrigationController::IsConnectedMQTTBroker() const {
#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  return mqtt_client_.IsConnected();
#else
  return false;
#endif
}

void IrrigationController::PublishMQTTMessage(const std::string& topic,
                                              const std::string& data) {
#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  mqtt_client_.Publish(topic, data);
#endif
}

#if CONFIG_IS_ENABLE_MQTT_PUBLISH
void IrrigationController::EventMQTTConnect() {
  GPIO::SetLevel(CONFIG_MONITORING_OUTPUT_GPIO_NO, 0);
}

void IrrigationController::EventMQTTDisconnect() {
  GPIO::SetLevel(CONFIG_MONITORING_OUTPUT_GPIO_NO, 1);
}
#endif

}  // namespace IrrigationSystem

// EOF
