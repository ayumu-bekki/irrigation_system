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


namespace IrrigationSystem {

IrrigationController::IrrigationController()
    :m_WifiManager()
    ,m_RelayTask()
    ,m_ScheduleManager(this)
    ,m_WeatherForecast()
{}

void IrrigationController::Start()
{
    // Initialize Log
    Logger::InitializeLogLevel();

    ESP_LOGI(TAG, "Startup Irrigation System. Version:%s", GIT_VERSION);

    // Initialize GPIO
    GPIO::InitOutput(CONFIG_MONITORING_OUTPUT_GPIO_NO, 1);
    GPIO::InitOutput(CONFIG_WATERING_OUTPUT_GPIO_NO);

    //Initialize NVS
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

    // MainTask
    ManagementTask managementTask(this);
    HttpdServerTask httpdServerTask(this);
    WateringButtonTask wateringButtonTask(this);

    managementTask.Start();
    httpdServerTask.Start();
    wateringButtonTask.Start();
    m_RelayTask.Start();

    ESP_LOGI(TAG, "Activation Complete Irrigation System.");

    // vTaskStartSchedule() is already called by ESP-IDF before app_main. Infinite loop thereafter.
    while(1) {
        Util::SleepMillisecond(1000);
    }
}

void IrrigationController::RelayAddOpenSecond(const int second)
{
    m_RelayTask.AddOpenSecond(second);   
}

void IrrigationController::RelayResetTimer()
{
    m_RelayTask.ResetTimer();   
}

void IrrigationController::RelayForce(const bool isOpen)
{
    m_RelayTask.Force(isOpen);   
}

std::time_t IrrigationController::RelayCloseEpoch() const 
{
    return m_RelayTask.GetCloseEpoch();
}

ScheduleManager& IrrigationController::GetScheduleManager() 
{
    return m_ScheduleManager;
}

WeatherForecast& IrrigationController::GetWeatherForecast()
{
    return m_WeatherForecast;
}


} // IrrigationSystem

// EOF
