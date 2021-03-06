// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_controller.h"

#include <esp_system.h>
#include <nvs_flash.h>

#include "logger.h"
#include "util.h"
#include "management_task.h"
#include "httpd_server_task.h"
#include "watering_button_task.h"
#include "gpio_control.h"
#include "file_system.h"


namespace IrrigationSystem {

IrrigationController::IrrigationController()
    :m_WifiManager()
    ,m_ValveTask()
    ,m_ScheduleManager(this)
    ,m_WeatherForecast()
    ,m_WateringSetting()
    ,m_WateringRecord()
{}

void IrrigationController::Start()
{
    // Initialize Log
    Logger::InitializeLogLevel();

    ESP_LOGI(TAG, "Startup Irrigation System. Version:%s", GIT_VERSION);

    // Initialize GPIO
    GPIO::InitOutput(CONFIG_MONITORING_OUTPUT_GPIO_NO, 1);
    GPIO::InitOutput(CONFIG_WATERING_OUTPUT_GPIO_NO);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_LOGE(TAG, "NVS Flash Error");
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // WiFi
    m_WifiManager.Connect(); 
    
    // Timezone init
    Util::InitTimeZone();

    // Sync NTP
    Util::SyncSntpObtainTime();

    // Normal Operation OK
    GPIO::SetLevel(CONFIG_MONITORING_OUTPUT_GPIO_NO, 0);

    // Mount File System
    FileSystem::Mount();

    // Read Setting Data
    std::string rawSettingData;
    if (WateringSetting::Load(rawSettingData)) {
        if (!m_WateringSetting.SetSettingData(rawSettingData)) {
            ESP_LOGE(TAG, "Invlaid Setting data");
        }
    } else {
        ESP_LOGI(TAG, "Failed Load Setting File");
    }

    // Read Last Watering Date
    m_WateringRecord.Load();

    // MainTask
    ManagementTask managementTask(this);
    HttpdServerTask httpdServerTask(this);
    WateringButtonTask wateringButtonTask(this);

    managementTask.Start();
    httpdServerTask.Start();
    wateringButtonTask.Start();
    m_ValveTask.Start();
    m_VoltageCheckTask.Start();

    ESP_LOGI(TAG, "Activation Complete Irrigation System.");

    // vTaskStartSchedule() is already called by ESP-IDF before app_main. Infinite loop thereafter.
    while(1) {
        Util::SleepMillisecond(1000);
    }
}

void IrrigationController::ValveAddOpenSecond(const int second)
{
    m_ValveTask.AddOpenSecond(second);   
}

void IrrigationController::ValveResetTimer()
{
    m_ValveTask.ResetTimer();   
}

void IrrigationController::ValveForce(const bool isOpen)
{
    m_ValveTask.Force(isOpen);   
}

std::time_t IrrigationController::ValveCloseEpoch() const 
{
    return m_ValveTask.GetCloseEpoch();
}

ScheduleManager& IrrigationController::GetScheduleManager() 
{
    return m_ScheduleManager;
}

WeatherForecast& IrrigationController::GetWeatherForecast()
{
    return m_WeatherForecast;
}

WateringSetting& IrrigationController::GetWateringSetting()
{
    return m_WateringSetting;
}

void IrrigationController::SaveLastWateringEpoch(const std::time_t wateringEpoch)
{
    m_WateringRecord.SetLastWateringEpoch(wateringEpoch);
    m_WateringRecord.Save();
}

std::time_t IrrigationController::GetLastWateringEpoch() const
{
    return m_WateringRecord.GetLastWateringEpoch();
}

float IrrigationController::GetBatteryVoltage() const
{
    return m_VoltageCheckTask.GetVoltage();
}


} // IrrigationSystem

// EOF
