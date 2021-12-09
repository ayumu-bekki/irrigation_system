// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "httpd_server_task.h"

#include <sstream>
#include <string>
#include <iomanip>
#include <algorithm>

#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "logger.h"
#include "util.h"
#include "schedule_manager.h"
#include "schedule_base.h"
#include "weather_forecast.h"
#include "watering_setting.h"


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

    // Post "/manual_watering" handle
    const httpd_uri_t routingManualWateringUriHandler = {
        .uri       = "/manual_watering",
        .method    = HTTP_POST,
        .handler   = this->ManualWateringHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingManualWateringUriHandler);

    // Post "/emergency_stop" handle
    const httpd_uri_t routingEmergencyStopyUriHandler = {
        .uri       = "/emergency_stop",
        .method    = HTTP_POST,
        .handler   = this->EmergencyStopHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingEmergencyStopyUriHandler);

    // Post "/upload_setting" handle
    const httpd_uri_t routingUploadSettingUriHandler = {
        .uri       = "/upload_setting",
        .method    = HTTP_POST,
        .handler   = this->UploadSettingHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingUploadSettingUriHandler);

    // Post "/download_setting" handle
    const httpd_uri_t routingDownloadSettingUriHandler = {
        .uri       = "/download_setting",
        .method    = HTTP_GET,
        .handler   = this->DownloadSettingHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingDownloadSettingUriHandler);

    // Post "/delete_setting" handle
    const httpd_uri_t routingDeleteSettingUriHandler = {
        .uri       = "/delete_setting",
        .method    = HTTP_POST,
        .handler   = this->DeleteSettingHandler,
        .user_ctx  = this,
    };
    httpd_register_uri_handler(httpdServerHandle, &routingDeleteSettingUriHandler);

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
    if (!pHttpdServerTask) {
        ESP_LOGE(TAG, "Failed HttpdServerTask is null");
        return ESP_FAIL;
    }

    IrrigationInterface *const pIrrigationInterface = pHttpdServerTask->m_pIrrigationInterface;
    if (!pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed irrigationInterface is null");
        return ESP_FAIL;
    }
    const WeatherForecast& weatherForecast = pIrrigationInterface->GetWeatherForecast();
    const WateringSetting& weatherSetting = pIrrigationInterface->GetWateringSetting();
    const ScheduleManager& scheduleManager = pIrrigationInterface->GetScheduleManager();
    const ScheduleManager::ScheduleBaseList& scheduleList = scheduleManager.GetScheduleList();
    const std::time_t valveCloseEpoch = pIrrigationInterface->ValveCloseEpoch();

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
        << "p, form {margin: 8px 10px}"
        << "table {margin: 16px 20px}"
        << "input {border-style:none; padding: 5px}"
        << bodyStyle
        << "hr {height:0;border:0;overflow:visible;border-top:3px dotted white;}"
        << "table {border-collapse: collapse;border-spacing: 0;background-color:aliceblue;border:solid 1px steelblue;}"
        << "table th {text-align:center;padding: 10px;background: steelblue;color: white;}"
        << "table td {padding: 10px; border-bottom: solid 1px steelblue; }"
        << ".schedule_disable { background-color: silver;}"
        << ".schedule_executable { background-color: greenyellow;}"
        << "</style>"
        << "<script>var checkSubmit = function(msg) { return confirm(msg); };</script>"
        << "</head>";

    httpd_resp_sendstr_chunk(pHttpRequestData, responseBody.str().c_str());
    responseBody.str("");
    responseBody.clear(std::stringstream::goodbit);

    responseBody
        << "<body><h1>" << title << "</h1>"
        << "<hr><h2>Schedule</h2>";

    if (weatherSetting.IsActive()) {
        const std::tm wateringTm = Util::EpochToLocalTime(pHttpdServerTask->m_pIrrigationInterface->GetLastWateringEpoch());

        responseBody
            << "<p>System Time : " << Util::GetNowTimeStr() << " TZ:" << CONFIG_LOCAL_TIME_ZONE << "</p>"
            << std::setfill('0')
            << "<p>Current Date : " 
            << std::setw(2) << scheduleManager.GetCurrentMonth() << "/" 
            << std::setw(2) << scheduleManager.GetCurrentDay()
            << "&nbsp;&nbsp; Last Watering Date : "
            << std::setw(2) << (wateringTm.tm_mon + 1) << "/" 
            << std::setw(2) << wateringTm.tm_mday
            << "</p>";

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
           
        responseBody << "</tbody></table>";
    } else {
        responseBody << "<p><span style=\"background-color:yellow;\">No settings have been made.<span></p>";
    }

    httpd_resp_sendstr_chunk(pHttpRequestData, responseBody.str().c_str());
    responseBody.str("");
    responseBody.clear(std::stringstream::goodbit);

    responseBody
        << "<hr><h2>Operation</h2>"
        << "<form action=\"/manual_watering\" method=\"post\">"
        << "Manual Watering. time (sec) : <input type=\"number\" name=\"second\" value=\"10\" min=\"1\" max=\"" << WEB_RELAY_OPEN_MAX_SECOND << "\"> "
        << "<input type=\"submit\" value=\"Start\">"
        << "</form>"
        << "<form action=\"/emergency_stop\" method=\"post\">"
        << "Emergency Stop : <input type=\"submit\" value=\"Stop\">"
        << "</form>"
        << "<p><form action=\"/upload_setting\" enctype=\"multipart/form-data\" method=\"post\" style=\"display:inline;\">"
        << "Watering Setting File : <input type=\"file\" name=\"setting_file\"><input type=\"submit\" value=\"Upload\">"
        << "</form>";

    if (weatherSetting.IsActive()) {
        responseBody
            << ":<form action=\"/download_setting\" method=\"get\" style=\"display:inline;\"><input type=\"submit\" value=\"Download\"></form>"
            << ":<form action=\"/delete_setting\" method=\"post\" style=\"display:inline;\" onsubmit=\"return checkSubmit('Are you sure you want to delete setting?');\">"
            << "<input type=\"submit\" value=\"Delete\"></form>"
            << "</p>";
    }

    httpd_resp_sendstr_chunk(pHttpRequestData, responseBody.str().c_str());
    responseBody.str("");
    responseBody.clear(std::stringstream::goodbit);

    responseBody
        << "<hr><h2>Information</h2>"
        << "<p>Valve Status : ";
    if (valveCloseEpoch == 0) {
        responseBody << "Close</p>";
    } else {
        responseBody 
            << "<span style=\"background:coral;\">Open</span> &gt; Close At(" 
            << Util::TimeToStr(Util::EpochToLocalTime(valveCloseEpoch)) << ")</p>";
    }
    responseBody
        << "<p>Weather Forecast : " << weatherInfo.str() << "</p>"
        << "<p>Version : " << GIT_VERSION << "</p>"
        << "</body></html>";

    httpd_resp_sendstr_chunk(pHttpRequestData, responseBody.str().c_str());
    responseBody.str("");
    responseBody.clear(std::stringstream::goodbit);

    httpd_resp_sendstr_chunk(pHttpRequestData, nullptr);
    return ESP_OK;
}

esp_err_t HttpdServerTask::ManualWateringHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGV(TAG, "WebServer Request Recv. Post:ManualWatering");

    if (!pHttpRequestData->user_ctx) {
        ESP_LOGE(TAG, "Failed user_ctx is null");
        return ESP_FAIL;
    }

    // Receive Post Data
    static constexpr size_t SCRATCH_BUFSIZE = 256;
    const int total_len = pHttpRequestData->content_len;

    if (SCRATCH_BUFSIZE <= total_len) {
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
    int valveOpenSecond = 0;
    std::vector<std::string> elements = Util::SplitString(buf, '=');   
    if (elements.size() == 2) {
        if (elements.at(0) == "second") {
            valveOpenSecond = std::max(1, std::min(WEB_RELAY_OPEN_MAX_SECOND, static_cast<int>(std::stol(elements.at(1)))));
        }
    }

    // Valve Open
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    if (!pHttpdServerTask) {
        ESP_LOGE(TAG, "Failed HttpdServerTask is null");
        return ESP_FAIL;
    }
    pHttpdServerTask->m_pIrrigationInterface->ValveAddOpenSecond(valveOpenSecond);

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

    // Valve Open
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    if (!pHttpdServerTask) {
        ESP_LOGE(TAG, "Failed HttpdServerTask is null");
        return ESP_FAIL;
    }
    pHttpdServerTask->m_pIrrigationInterface->ValveResetTimer();

    // Redirect
    httpd_resp_set_status(pHttpRequestData, "303 See Other");
    httpd_resp_set_hdr(pHttpRequestData, "Location", "/");
    httpd_resp_send(pHttpRequestData, NULL, 0);
    return ESP_OK;
}


esp_err_t HttpdServerTask::UploadSettingHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGV(TAG, "WebServer Request Recv. Post:UploadSetting");

    // Check
    if (!pHttpRequestData->user_ctx) {
        ESP_LOGE(TAG, "Failed user_ctx is null");
        return ESP_FAIL;
    }
    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    if (!pHttpdServerTask) {
        ESP_LOGE(TAG, "Failed HttpdServerTask is null");
        return ESP_FAIL;
    }

    IrrigationInterface *const pIrrigationInterface = pHttpdServerTask->m_pIrrigationInterface;
    if (!pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed irrigationInterface is null");
        return ESP_FAIL;
    }

    // Receive Header (get Multipart boundary)
    static constexpr char *const HTTP_HEADER_CONTENT_TYPE = (char*)"Content-Type";
    const size_t contentTypeHeaderLen = httpd_req_get_hdr_value_len(pHttpRequestData, HTTP_HEADER_CONTENT_TYPE);
    if (contentTypeHeaderLen == 0) {
        ESP_LOGE(TAG, "Not Found Rqeust Header Empty: %s", HTTP_HEADER_CONTENT_TYPE);
        return ESP_FAIL;
    }
    std::string contentType;
    contentType.resize(contentTypeHeaderLen);
    if (httpd_req_get_hdr_value_str(pHttpRequestData, HTTP_HEADER_CONTENT_TYPE, &contentType.at(0), contentTypeHeaderLen + 1) != ESP_OK) {
        ESP_LOGE(TAG, "Not Found Rqeust Header : %s", HTTP_HEADER_CONTENT_TYPE);
        return ESP_FAIL;
    }
    //ESP_LOGV(TAG, "Found header => %s: %s",HTTP_HEADER_CONTENT_TYPE, contentType.c_str());
   
    // Get Boundary String
    const std::string BOUNDARY_STR = "boundary=";
    std::string::size_type pos = contentType.find(BOUNDARY_STR); 
    if (pos == std::string::npos) {
        ESP_LOGE(TAG, "Failed Get Rqeust Header : %s", HTTP_HEADER_CONTENT_TYPE);
        return ESP_FAIL;
    }
    const std::string boundaryStr = contentType.substr(pos + BOUNDARY_STR.length());
    const std::string::size_type boundaryStrLength = boundaryStr.length();
    //ESP_LOGV(TAG, "Boundary => %s", boundaryStr.c_str());


    // Receive Post Data
    static constexpr size_t MAX_FILE_SIZE = 10240; // 10 KB
    const int total_len = pHttpRequestData->content_len;
    if (MAX_FILE_SIZE <= total_len) {
        httpd_resp_send_err(pHttpRequestData, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    ESP_LOGV(TAG, "Receive post data length:%d", total_len);
    std::string body;
    body.resize(total_len + 1);
    int cur_len = 0;
    int received = 0;
    while (cur_len < total_len) {
        received = httpd_req_recv(pHttpRequestData, &body.at(0) + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(pHttpRequestData, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    //ESP_LOGE(TAG, "Recv File \n-------\n%s", body.c_str());

    // Split multipart data
    std::vector<std::string> list;
    if (boundaryStrLength == 0) {
        list.push_back(body);
    } else {
        std::string::size_type offset = std::string::size_type(0);
        while (true) {
            std::string::size_type pos = body.find(std::string("--") + boundaryStr, offset);
            if (pos == std::string::npos) {
                std::string splitStr = body.substr(offset);
                list.push_back(splitStr);
                break;
            }
            std::string splitStr = body.substr(offset, pos - offset);
            list.push_back(splitStr);
            offset = pos + boundaryStrLength + 2;
        }
    }

    // Get Json Data
    std::string payloadJsonData;
    for (std::vector<std::string>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
        //ESP_LOGV(TAG, "Split\n%s", (*iter).c_str());
        if (iter->find("name=\"setting_file\"") != std::string::npos) {
            std::string::size_type begin = iter->find("\r\n\r\n") + 4;
            std::string::size_type end = iter->rfind("\r\n");
            if ((end - begin) <= 0) {
                break;
            }
            payloadJsonData = iter->substr(begin, end - begin);
            ESP_LOGV(TAG, "OK Payload-------\n%s\n-----", payloadJsonData.c_str());
            break;
        }
    }
    
    // Parse
    WateringSetting& weatherSetting = pHttpdServerTask->m_pIrrigationInterface->GetWateringSetting();
    if (!weatherSetting.SetSettingData(payloadJsonData)) {
        httpd_resp_send_err(pHttpRequestData, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid Data");
        return ESP_FAIL;
    }
    
    // Save
    if (!WateringSetting::Save(payloadJsonData)) {
        httpd_resp_send_err(pHttpRequestData, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed save");
        return ESP_FAIL;
    }

    // Init Schedule
    ScheduleManager& scheduleManager = pIrrigationInterface->GetScheduleManager();
    const std::tm nowTimeInfo = Util::GetLocalTime();
    scheduleManager.InitializeNewDay(nowTimeInfo);

    // Redirect
    httpd_resp_set_status(pHttpRequestData, "303 See Other");
    httpd_resp_set_hdr(pHttpRequestData, "Location", "/");
    httpd_resp_send(pHttpRequestData, NULL, 0);
    return ESP_OK;
}

esp_err_t HttpdServerTask::DownloadSettingHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGV(TAG, "WebServer Request Recv. Post:DownloadSetting");

    std::string rawSettingData;
    if (!WateringSetting::Load(rawSettingData)) {
        ESP_LOGE(TAG, "Failed Load Setting File");
        return ESP_FAIL;
    }

    httpd_resp_set_type(pHttpRequestData, "application/json");
    httpd_resp_send(pHttpRequestData, rawSettingData.c_str(), rawSettingData.length());
    return ESP_OK;
}
 
esp_err_t HttpdServerTask::DeleteSettingHandler(httpd_req_t *pHttpRequestData)
{
    ESP_LOGV(TAG, "WebServer Request Recv. Post:DeleteSetting");

    if (!pHttpRequestData->user_ctx) {
        ESP_LOGE(TAG, "Failed user_ctx is null");
        return ESP_FAIL;
    }

    HttpdServerTask *const pHttpdServerTask = static_cast<HttpdServerTask*>(pHttpRequestData->user_ctx);
    if (!pHttpdServerTask) {
        ESP_LOGE(TAG, "Failed HttpdServerTask is null");
        return ESP_FAIL;
    }
    IrrigationInterface *const pIrrigationInterface = pHttpdServerTask->m_pIrrigationInterface;
    if (!pIrrigationInterface) {
        ESP_LOGE(TAG, "Failed irrigationInterface is null");
        return ESP_FAIL;
    }

    // Delete
    if (!WateringSetting::Delete()) {
        httpd_resp_send_err(pHttpRequestData, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed delete");
        return ESP_FAIL;
    }

    // WateringSetting init
    WateringSetting& wateringSetting = pHttpdServerTask->m_pIrrigationInterface->GetWateringSetting();
    wateringSetting = WateringSetting();

    // Init Schedule
    ScheduleManager& scheduleManager = pIrrigationInterface->GetScheduleManager();
    const std::tm nowTimeInfo = Util::GetLocalTime();
    scheduleManager.InitializeNewDay(nowTimeInfo);

    // Redirect
    httpd_resp_set_status(pHttpRequestData, "303 See Other");
    httpd_resp_set_hdr(pHttpRequestData, "Location", "/");
    httpd_resp_send(pHttpRequestData, NULL, 0);
    return ESP_OK;
}

esp_err_t HttpdServerTask::ErrorNotFoundHandler(httpd_req_t *pHttpRequestData, httpd_err_code_t errCode)
{
    httpd_resp_send_err(pHttpRequestData, HTTPD_404_NOT_FOUND, "HTTP Status 404 Not Found");
    return ESP_FAIL;
}


} // IrrigationSystem

// EOF
