// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_controller.h"

#include <esp_system.h>
#include <nvs_flash.h>
#include <memory>

#include "logger.h"
#include "util.h"
#include "management_task.h"
#include "httpd_server_task.h"
#include "watering_button_task.h"
#include "gpio_control.h"
#include "file_system.h"
#include "version.h"


namespace IrrigationSystem {

IrrigationController::IrrigationController()
    :m_WifiManager()
    ,m_ValveTask()
    ,m_ScheduleManager()
    ,m_WeatherForecast()
    ,m_WateringSetting()
    ,m_WateringRecord()
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
    ,m_VoltageCheckTask()
#endif
#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
    ,m_WaterLevelChecker()
#endif
{}


IrrigationController::~IrrigationController() = default;

void IrrigationController::Start()
{
    // Initialize Log
    Logger::InitializeLogLevel();

    ESP_LOGI(TAG, "Startup Irrigation System. Version:%s", GIT_VERSION);

    // Monitoring LED Init And ON
    GPIO::InitOutput(CONFIG_MONITORING_OUTPUT_GPIO_NO, 1);

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
    // VoltageCheck GPIO Init
    GPIO::InitOutput(CONFIG_VAOLTAGE_CHECK_OUTPUT_GPIO_NO, 0);
#endif

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
    ManagementTask managementTask(weak_from_this());
    HttpdServerTask httpdServerTask(weak_from_this());
    WateringButtonTask wateringButtonTask(weak_from_this());
    m_ScheduleManager = std::make_shared<ScheduleManager>(weak_from_this());
    m_ValveTask = std::make_unique<ValveTask>(weak_from_this());

    managementTask.Start();
    httpdServerTask.Start();
    wateringButtonTask.Start();

    if (m_ValveTask) {
        m_ValveTask->Start();
    }

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
    m_VoltageCheckTask.Start();
#endif

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
    m_WaterLevelChecker.Start();
#endif

    // Monitoring LED Off
    GPIO::SetLevel(CONFIG_MONITORING_OUTPUT_GPIO_NO, 0);

    ESP_LOGI(TAG, "Activation Complete Irrigation System.");

    // vTaskStartSchedule() is already called by ESP-IDF before app_main. Infinite loop thereafter.
    while (true) {
        Util::SleepMillisecond(1000);
    }
}

void IrrigationController::ValveAddOpenSecond(const int second)
{
    if (m_ValveTask) {
        m_ValveTask->AddOpenSecond(second);   
    }
}

void IrrigationController::ValveResetTimer()
{
    if (m_ValveTask) {
        m_ValveTask->ResetTimer();
    }
}

void IrrigationController::ValveForce(const bool isOpen)
{
    if (m_ValveTask) {
        m_ValveTask->Force(isOpen);
    }
}

std::time_t IrrigationController::ValveCloseEpoch() const 
{
    if (m_ValveTask) {
        return m_ValveTask->GetCloseEpoch();
    }
    return 0;
}

const ScheduleManagerWeakPtr IrrigationController::GetScheduleManager() 
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

const WateringSetting& IrrigationController::GetWateringSetting() const
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

float IrrigationController::GetMainVoltage() const
{
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
    return m_VoltageCheckTask.GetVoltage();
#else
    return 0.0f;
#endif
}

void IrrigationController::CheckWaterLevel()
{
#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
    m_WaterLevelChecker.Check();
#endif
}

float IrrigationController::GetWaterLevel() const
{
#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
    return m_WaterLevelChecker.GetWaterLevel();
#else
    return 0.0f;
#endif
}

} // IrrigationSystem

// EOF
