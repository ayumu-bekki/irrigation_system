// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "management_task.h"

#include <esp_log.h>

#include "define.h"
#include "util.h"
#include "irrigation_controller.h"
#include "http_request.h"

namespace IrrigationSystem {

ManagementTask::ManagementTask(IrrigationInterface *const pIrricationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrricationInterface(pIrricationInterface)
    ,m_TargetDate(0)
{
    m_TargetDate = time(nullptr) + 60 * 60;

    // 初期タスクの構築
}

void ManagementTask::Update()
{
    if (!m_pIrricationInterface) {
        return;
    }

    // タスクのチェック
    
    // タスクが実行できる場合は実行

    /*
    // HttpRequest Test
    HttpRequest httpRequest;
    httpRequest.Request("http://example.net/");
    if (httpRequest.GetStatus() == HttpRequest::STATUS_OK) {
        ESP_LOGI(TAG, "Request OK");
    } else if (httpRequest.GetStatus() == HttpRequest::STATUS_NG) {
        ESP_LOGI(TAG, "Request NG");
    }
    */

    const time_t now = time(nullptr);
    if (m_TargetDate <= now) {
        m_pIrricationInterface->RequestRelayOpen(5);
        m_TargetDate = now + 60 * 60;
        ESP_LOGI(TAG, "Task Open Relay Next:%ld Now:%ld", m_TargetDate, now);
    }

    Util::SleepMillisecond(10 * 1000);
}

} // IrrigationSystem

// EOF
