// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "httpd_server_task.h"

#include <esp_log.h>

#include <sstream>
#include <string>
#include <iomanip>
#include <algorithm>

#include "schedule_manager.h"
#include "schedule_base.h"
#include "define.h"
#include "util.h"

namespace IrrigationSystem {

HttpdServerTask::HttpdServerTask(IrrigationInterface *const pIrricationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrricationInterface(pIrricationInterface)
    ,m_HttpdHandle(NULL)
{}

void HttpdServerTask::Initialize()
{
    StopWebServer();
    m_HttpdHandle = StartWebServer();
}

httpd_handle_t HttpdServerTask::StartWebServer()
{
    ESP_LOGI(TAG, "Starting HTTP Server");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t httpdServerHandle = NULL;
    if (httpd_start(&httpdServerHandle, &config) != ESP_OK) {
        return NULL;
    }

    // Get "/" Handle
    const httpd_uri_t routingRootUriHandler = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = this->RootHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingRootUriHandler);

    // Post "/open_relay" handle
    const httpd_uri_t routingOpenRelayUriHandler = {
        .uri       = "/open_relay",
        .method    = HTTP_POST,
        .handler   = this->OpenRelayHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingOpenRelayUriHandler);
 
    // Not Found Handle
    httpd_register_err_handler(httpdServerHandle, HTTPD_404_NOT_FOUND, this->ErrorNotFoundHandler);
    
    return httpdServerHandle;
}

void HttpdServerTask::StopWebServer()
{
    if (m_HttpdHandle) {
        ESP_LOGI(TAG, "Stop HTTP Server");
        httpd_stop(m_HttpdHandle);
        m_HttpdHandle = nullptr;
    }
}

void HttpdServerTask::Update()
{
    Util::SleepMillisecond(10 * 1000);
}

esp_err_t HttpdServerTask::RootHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGI(TAG, "WebServer Request Recv. Get:Root");

    if (!pHttpRequestData->user_ctx) {
        ESP_LOGI(TAG, "Failed user_ctx is null");
        return ESP_FAIL;
    }
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    ScheduleManager& scheduleManager = pHttpdServerTask->m_pIrricationInterface->GetScheduleManager();
    const ScheduleManager::ScheduleBaseList& scheduleList = scheduleManager.GetScheduleList();

    std::stringstream responseBody;
    responseBody 
        << "<!doctype html><head>"
        << "<title>Irrigation System</title><style>" 
        << "body {background-color:lightskyblue;}"
        << "hr {height:0;margin:0;padding:0;border:0;overflow:visible;border-top:3px dotted white;}"
        << "table {border-collapse: collapse;border-spacing: 0;margin-bottom:1.5em;background-color:aliceblue;}"
        << "table th {text-align:center;padding: 10px;background: steelblue;color: white;}"
        << "table td {padding: 10px; border-bottom: solid 1px steelblue; }"
        << ".schedule_disable { background-color: silver;}"
        << ".schedule_executable { background-color: greenyellow;}"
        << "</style></head>"
        << "<body><h1>Irrigation System</h1>"
        << "<hr><h2>Status</h2>"
        << "<p>System Time : " << Util::GetNowTimeStr() << " TZ:" << CONFIG_LOCAL_TIME_ZONE << "</p>"
        << "<p>Version : " << GIT_VERSION << "</p>"
        << "<hr><h2>Schedule</h2>"
        << "<p>Current Date : " << scheduleManager.GetCurrentMonth() << "/" << scheduleManager.GetCurrentDay() << "</p>";

    // Create Schedule Table
    responseBody << "<table><thead><tr><th>ScheduleName</th><th>Time</th><th>Status</th></tr></thead><tbody>";

    if (std::any_of(scheduleList.begin(), scheduleList.end(), [](const ScheduleBase::UniquePtr& item){ return item->IsVisible(); })) {
        // Found Visible Schedule Item
        for (const auto& pScheduleItem : scheduleList) {
            if (pScheduleItem->IsVisible()) {
                responseBody 
                    << std::setfill('0')
                    << "<tr class=\"" << ScheduleBase::StatusToRecordStyle(pScheduleItem->GetStatus()) << "\">"
                    << "<td>" << pScheduleItem->GetName() << "</td>"
                    << "<td>" 
                    << std::setw(2) << pScheduleItem->GetHour() << ":"
                    << std::setw(2) << pScheduleItem->GetMinute()
                    << "</td>"
                    << "<td>" << ScheduleBase::StatusToStr(pScheduleItem->GetStatus()) << "</td>"
                    << "</tr>";
            }
        }
    } else {
        // Not Found Visible Schedule Item
        responseBody << "<tr><td colspan=\"3\">Empty</td></tr>";
    }
   
    responseBody
        << "</tbody></table>"
        << "<hr><h2>Manual Watering</h2>"
        << "<form action=\"/open_relay\" method=\"post\">"
        << "Watering time (sec) : <input type=\"number\" name=\"second\" value=\"10\" min=\"1\" max=\"60\">"
        << "<input type=\"submit\" value=\"Start\">"
        << "</form>"
        << "</body></html>";

    httpd_resp_send(pHttpRequestData, responseBody.str().c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HttpdServerTask::OpenRelayHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGI(TAG, "WebServer Request Recv. Post:OpenRelay");

    if (!pHttpRequestData->user_ctx) {
        ESP_LOGI(TAG, "Failed user_ctx is null");
        return ESP_FAIL;
    }

    // Receive Post Data
    static constexpr size_t SCRATCH_BUFSIZE = 256;
    const int total_len = pHttpRequestData->content_len;

    if (total_len >= SCRATCH_BUFSIZE) {
        httpd_resp_send_err(pHttpRequestData, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    char buf[SCRATCH_BUFSIZE] = {};
    int cur_len = 0;
    int received = 0;
    while (cur_len < total_len) {
        received = httpd_req_recv(pHttpRequestData, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(pHttpRequestData, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    //ESP_LOGI(TAG, " Recv Data Length:%d Data:%s", total_len, buf);
    
    // Parse
    int relayOpenSecond = 0;
    std::vector<std::string> elements = Util::SplitString(buf, '=');   
    if (elements.size() == 2) {
        if (elements.at(0) == "second") {
            relayOpenSecond = std::max(1, std::min(60, static_cast<int>(std::stol(elements.at(1)))));
        }
    }

    // Relay Open
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    pHttpdServerTask->m_pIrricationInterface->RequestRelayOpen(relayOpenSecond);

    // Redirect
    httpd_resp_set_status(pHttpRequestData, "303 See Other");
    httpd_resp_set_hdr(pHttpRequestData, "Location", "/");
    httpd_resp_send(pHttpRequestData, NULL, 0);
    return ESP_OK;
}

esp_err_t HttpdServerTask::ErrorNotFoundHandler(httpd_req_t *pHttpRequestData, httpd_err_code_t errCode)
{
    if (strcmp("/", pHttpRequestData->uri) == 0) {
        return ESP_OK;
    }
    if (strcmp("/open_relay", pHttpRequestData->uri) == 0) {
        return ESP_OK;
    }
    httpd_resp_send_err(pHttpRequestData, HTTPD_404_NOT_FOUND, "HTTP Status 404 Not Found");
    return ESP_FAIL;
}


} // IrrigationSystem

// EOF
