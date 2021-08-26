// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "httpd_server_task.h"

#include <sstream>
#include <string>
#include <iomanip>
#include <algorithm>

#include "logger.h"
#include "util.h"
#include "schedule_manager.h"
#include "schedule_base.h"
#include "weather_forecast.h"


namespace IrrigationSystem {

static constexpr int WEB_RELAY_OPEN_MAX_SECOND = 60;

HttpdServerTask::HttpdServerTask(IrrigationInterface *const pIrrigationInterface)
    :Task(TASK_NAME, PRIORITY, CORE_ID)
    ,m_pIrrigationInterface(pIrrigationInterface)
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

    // Post "/emergency_stop" handle
    const httpd_uri_t routingEmergencyStopyUriHandler = {
        .uri       = "/emergency_stop",
        .method    = HTTP_POST,
        .handler   = this->EmergencyStopHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingEmergencyStopyUriHandler);
 
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
    ESP_LOGV(TAG, "WebServer Request Recv. Get:Root");

    if (!pHttpRequestData->user_ctx) {
        ESP_LOGE(TAG, "Failed user_ctx is null");
        return ESP_FAIL;
    }
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);

    IrrigationInterface *const pIrrigationInterface = pHttpdServerTask->m_pIrrigationInterface;
    if (!pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed irrigationInterface is null");
        return ESP_FAIL;
    }
    const WeatherForecast &weatherForecast = pIrrigationInterface->GetWeatherForecast();
    const ScheduleManager& scheduleManager = pIrrigationInterface->GetScheduleManager();
    const ScheduleManager::ScheduleBaseList& scheduleList = scheduleManager.GetScheduleList();
    const std::time_t relayCloseEpoch = pIrrigationInterface->RelayCloseEpoch();

#if CONFIG_DEBUG != 0
    static const std::string title = "Irrigation System (DEBUG)";
    static const std::string bodyStyle = "body {background-color:lightgray;}";
#else
    static const std::string title = "Irrigation System";
    static const std::string bodyStyle = "body {background-color:lightskyblue;}";
#endif

    
    std::stringstream weatherInfo;
    if (weatherForecast.GetRequestStatus() == WeatherForecast::NOT_REQUEST) {   
        weatherInfo << " Not yet acquired.";
    } else if (weatherForecast.GetRequestStatus() == WeatherForecast::ACQUIRED) {   
        weatherInfo << " Weather(" << WeatherForecast::WeatherCodeToStr(weatherForecast.GetCurrentWeatherCode())
                    << ") MaxTemp(" << weatherForecast.GetCurrentMaxTemperature() << "°C)";
    } else {
        weatherInfo << " <span style=\"background-color: yellow;\">Failed to retrieve data</span>";
    }

    std::stringstream responseBody;
    responseBody 
        << "<!doctype html><head>"
        << "<meta charset=\"utf-8\"/>"
        << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        << "<meta http-equiv=\"refresh\" content=\"3600\">"
        << "<title>" << title << "</title>"
        << "<style>" 
        << "*{box-sizing:border-box;margin:0;padding:0;}"
        << "html{font-size: 16px}"
        << "h1, h2 {margin: 14px}"
        << "hr {margin:0px 6px}"
        << "p, form {margin: 8px 20px}"
        << "table {margin: 16px 20px}"
        << "input {border-style:none; padding: 5px}"
        << bodyStyle
        << "hr {height:0;border:0;overflow:visible;border-top:3px dotted white;}"
        << "table {border-collapse: collapse;border-spacing: 0;background-color:aliceblue;border:solid 1px steelblue;}"
        << "table th {text-align:center;padding: 10px;background: steelblue;color: white;}"
        << "table td {padding: 10px; border-bottom: solid 1px steelblue; }"
        << ".schedule_disable { background-color: silver;}"
        << ".schedule_executable { background-color: greenyellow;}"
        << "</style></head>"
        << "<body><h1>" << title << "</h1>"
        << "<hr><h2>Schedule</h2>"
        << "<p>Current Date : " << scheduleManager.GetCurrentMonth() << "/" << scheduleManager.GetCurrentDay() << "</p>"
        << "<p>System Time : " << Util::GetNowTimeStr() << " TZ:" << CONFIG_LOCAL_TIME_ZONE << "</p>";

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
        << "<hr><h2>Manual Operation</h2>"
        << "<form action=\"/open_relay\" method=\"post\">"
        << "Watering time (sec) : <input type=\"number\" name=\"second\" value=\"10\" min=\"1\" max=\"" << WEB_RELAY_OPEN_MAX_SECOND << "\"> "
        << "<input type=\"submit\" value=\"Start\">"
        << "</form>"
        << "<form action=\"/emergency_stop\" method=\"post\">"
        << "Emergency Stop : <input type=\"submit\" value=\"Stop\">"
        << "</form>"
        << "<hr><h2>Information</h2>"
        << "<p>Relay Status : ";
    if (relayCloseEpoch == 0) {
        responseBody << "Close</p>";
    } else {
        responseBody 
            << "<span style=\"background:coral;\">Open</span> > Close At(" 
            << Util::TimeToStr(Util::EpochToLocalTime(relayCloseEpoch)) << ")</p>";
    }
    responseBody
        << "<p>Weather Forecast : " << weatherInfo.str() << "<p>"
        << "<p>Version : " << GIT_VERSION << "</p>"
        << "</body></html>";

    httpd_resp_send(pHttpRequestData, responseBody.str().c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t HttpdServerTask::OpenRelayHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGV(TAG, "WebServer Request Recv. Post:OpenRelay");

    if (!pHttpRequestData->user_ctx) {
        ESP_LOGE(TAG, "Failed user_ctx is null");
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

    ESP_LOGV(TAG, " Recv Data Length:%d Data:%s", total_len, buf);
    
    // Parse
    int relayOpenSecond = 0;
    std::vector<std::string> elements = Util::SplitString(buf, '=');   
    if (elements.size() == 2) {
        if (elements.at(0) == "second") {
            relayOpenSecond = std::max(1, std::min(WEB_RELAY_OPEN_MAX_SECOND, static_cast<int>(std::stol(elements.at(1)))));
        }
    }

    // Relay Open
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    pHttpdServerTask->m_pIrrigationInterface->RelayAddOpenSecond(relayOpenSecond);

    // Redirect
    httpd_resp_set_status(pHttpRequestData, "303 See Other");
    httpd_resp_set_hdr(pHttpRequestData, "Location", "/");
    httpd_resp_send(pHttpRequestData, NULL, 0);
    return ESP_OK;
}

esp_err_t HttpdServerTask::EmergencyStopHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGV(TAG, "WebServer Request Recv. Post:EmergencyStop");

    if (!pHttpRequestData->user_ctx) {
        ESP_LOGE(TAG, "Failed user_ctx is null");
        return ESP_FAIL;
    }

    // Relay Open
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    pHttpdServerTask->m_pIrrigationInterface->RelayResetTimer();

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
    if (strcmp("/emergency_stop", pHttpRequestData->uri) == 0) {
        return ESP_OK;
    }
    httpd_resp_send_err(pHttpRequestData, HTTPD_404_NOT_FOUND, "HTTP Status 404 Not Found");
    return ESP_FAIL;
}


} // IrrigationSystem

// EOF
