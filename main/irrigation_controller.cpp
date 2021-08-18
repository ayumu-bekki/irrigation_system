// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_controller.h"

#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_log.h>

#include "define.h"
#include "util.h"
#include "management_task.h"
#include "httpd_server_task.h"
#include "gpio_control.h"


namespace IrrigationSystem {

IrrigationController::IrrigationController()
    :m_WifiManager()
    ,m_RelayTask()
    ,m_ScheduleManager(this)
{}

void IrrigationController::Start()
{
    ESP_LOGI(TAG, "Startup Irrigation System. Version:%s", GIT_VERSION);

    // Initialize GPIO
    GPIO::InitOutput(CONFIG_MONITORING_SIGNAL_GPIO_NO, 1);
    GPIO::InitOutput(CONFIG_RELAY_SIGNAL_GPIO_NO);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_LOGI(TAG, "NVS Flash Error");
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
    GPIO::SetLevel(CONFIG_MONITORING_SIGNAL_GPIO_NO, 0);

    // MainTask
    ManagementTask managementTask(this);
    HttpdServerTask httpdServerTask(this);

    managementTask.Start();
    httpdServerTask.Start();
    m_RelayTask.Start();

    ESP_LOGI(TAG, "Activation Complete Irrigation System.");

    // vTaskStartSchedule() is already called by ESP-IDF before app_main. Infinite loop thereafter.
    while(1) {
        Util::SleepMillisecond(1000);
    }
}

void IrrigationController::RequestRelayOpen(const int second)
{
    m_RelayTask.AddOpenSecond(second);   
}


ScheduleManager& IrrigationController::GetScheduleManager() 
{
    return m_ScheduleManager;
}

} // IrrigationSystem

// EOF
