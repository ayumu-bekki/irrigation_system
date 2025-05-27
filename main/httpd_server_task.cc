// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "httpd_server_task.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "logger.h"
#include "schedule_base.h"
#include "schedule_manager.h"
#include "schedule_manual.h"
#include "schedule_watering.h"
#include "util.h"
#include "valve_executor.h"
#include "version.h"
#include "water_flow_sensor.h"
#include "watering_setting.h"
#include "weather_forecast.h"

namespace {
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
std::string voltageToColorName(const float voltage) {
  if (12.5f <= voltage) {
    return "lime";
  } else if (12.0f <= voltage) {
    return "chartreuse";
  } else if (11.8f <= voltage) {
    return "yellow";
  } else if (11.5f <= voltage) {
    return "coral";
  }
  return "darkgray";
}
#endif
#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
std::string water_levelToColorName(const int water_level) {
  if (60 <= water_level) {
    return "steelblue";
  } else if (25 <= water_level) {
    return "lightseagreen";
  }
  return "yellow";
}
#endif
}  // namespace

namespace IrrigationSystem {

static constexpr int WEB_RELAY_OPEN_MAX_SECOND = 180;

HttpdServerTask::HttpdServerTask(
    const IrrigationInterfaceWeakPtr irrigation_interface)
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      irrigation_interface_(irrigation_interface),
      httpd_handle_(NULL) {}

void HttpdServerTask::Initialize() {
  StopWebServer();
  httpd_handle_ = StartWebServer();
}

httpd_handle_t HttpdServerTask::StartWebServer() {
  ESP_LOGI(TAG, "Starting HTTP Server");

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 10;
  httpd_handle_t httpd_server_handle = NULL;
  if (httpd_start(&httpd_server_handle, &config) != ESP_OK) {
    return NULL;
  }

  // Get "/" Handle
  const httpd_uri_t routing_root_uri_handler = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = this->RootHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle, &routing_root_uri_handler);

  // Post "/manual_watering" handle
  const httpd_uri_t routing_manual_watering_uri_handler = {
      .uri = "/manual_watering",
      .method = HTTP_POST,
      .handler = this->ManualWateringHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_manual_watering_uri_handler);

  // Post "/emergency_stop" handle
  const httpd_uri_t routing_emergency_stop_uri_handler = {
      .uri = "/emergency_stop",
      .method = HTTP_POST,
      .handler = this->EmergencyStopHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_emergency_stop_uri_handler);

  // Post "/upload_setting" handle
  const httpd_uri_t routing_upload_setting_uri_handler = {
      .uri = "/upload_setting",
      .method = HTTP_POST,
      .handler = this->UploadSettingHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_upload_setting_uri_handler);

  // Get "/download_setting" handle
  const httpd_uri_t routing_download_setting_uri_handler = {
      .uri = "/download_setting",
      .method = HTTP_GET,
      .handler = this->DownloadSettingHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_download_setting_uri_handler);

  // Post "/delete_setting" handle
  const httpd_uri_t routing_delete_setting_uri_handler = {
      .uri = "/delete_setting",
      .method = HTTP_POST,
      .handler = this->DeleteSettingHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_delete_setting_uri_handler);

  // Get "/voltage" handle
  const httpd_uri_t routing_get_voltage_uri_handler = {
      .uri = "/voltage",
      .method = HTTP_GET,
      .handler = this->GetVoltageHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_get_voltage_uri_handler);

  // Get "/water_level" handle
  const httpd_uri_t routing_get_water_level_uri_handler = {
      .uri = "/water_level",
      .method = HTTP_GET,
      .handler = this->GetWaterLevelHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_get_water_level_uri_handler);

  // Post "/system_restart" handle
  const httpd_uri_t routing_restart_system_uri_handler = {
      .uri = "/system_restart",
      .method = HTTP_POST,
      .handler = this->RestartSystemHandler,
      .user_ctx = this,
  };
  httpd_register_uri_handler(httpd_server_handle,
                             &routing_restart_system_uri_handler);

  // Not Found Handle
  httpd_register_err_handler(httpd_server_handle, HTTPD_404_NOT_FOUND,
                             this->ErrorNotFoundHandler);

  return httpd_server_handle;
}

void HttpdServerTask::StopWebServer() {
  if (httpd_handle_) {
    ESP_LOGI(TAG, "Stop HTTP Server");
    httpd_stop(httpd_handle_);
    httpd_handle_ = nullptr;
  }
}

void HttpdServerTask::Update() { Util::SleepMillisecond(10 * 1000); }

esp_err_t HttpdServerTask::RootHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Get:Root");

  if (!request_data->user_ctx) {
    ESP_LOGE(TAG, "Failed user_ctx is null");
    return ESP_FAIL;
  }

  HttpdServerTask *const httpd_server_task =
      static_cast<HttpdServerTask *>(request_data->user_ctx);
  if (!httpd_server_task) {
    ESP_LOGE(TAG, "Failed HttpdServerTask is null");
    return ESP_FAIL;
  }
  const IrrigationInterfaceSharedPtr irrigation_interface =
      httpd_server_task->irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return ESP_FAIL;
  }
  const WeatherForecast &weather_forecast =
      irrigation_interface->GetWeatherForecast();
  const WateringSetting &weather_setting =
      irrigation_interface->GetWateringSetting();
  const ScheduleManagerSharedPtr schedule_manager =
      irrigation_interface->GetScheduleManager().lock();
  if (!schedule_manager) {
    ESP_LOGE(TAG, "Failed Schedule Manager is null");
    return ESP_FAIL;
  }
  const ScheduleManager::ScheduleBaseList &schedule_list =
      schedule_manager->GetScheduleList();
#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  const float batteryVoltage = irrigation_interface->GetMainVoltage();
  const int32_t voltageGuage =
      ((batteryVoltage - 10.0f) / (15.0 - 10.0f)) * 100.0f;
#endif

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  const int32_t water_level = irrigation_interface->GetWaterLevel() * 100.0f;
#endif

#if CONFIG_DEBUG != 0
  static const std::string title = "Irrigation System (DEBUG)";
  static const std::string body_style = "body {background-color:lightgray;}";
#else
  static const std::string title = "Irrigation System";
  static const std::string body_style = "body {background-color:lightskyblue;}";
#endif

  std::stringstream weatherInfo;
  if (weather_forecast.GetRequestStatus() == WeatherForecast::NOT_REQUEST) {
    weatherInfo << " Not yet acquired.";
  } else if (weather_forecast.GetRequestStatus() == WeatherForecast::ACQUIRED) {
    weatherInfo << " Weather("
                << WeatherForecast::WeatherCodeToStr(
                       weather_forecast.GetCurrentWeatherCode())
                << ") MaxTemp(" << weather_forecast.GetCurrentMaxTemperature()
                << "°C)";
  } else {
    weatherInfo << " <span style=\"background-color: yellow;\">Failed to "
                   "retrieve data</span>";
  }

  std::stringstream response_body;
  response_body
      << "<!doctype html><head>"
      << "<meta charset=\"utf-8\"/>"
      << "<meta name=\"viewport\" "
         "content=\"width=device-width,initial-scale=1\">"
      << "<meta http-equiv=\"refresh\" content=\"3600\">"
      << "<title>" << title << "</title>"
      << "<style>"
      << "*{box-sizing:border-box;margin:0;padding:0;}"
      << "h1 {margin: 10px 12px; font-size: 1.3em;}"
      << "h2 {margin: 10px 12px; font-size: 1.2em;}"
      << "h3 {margin: 8px 12px; font-size: 1.0em;}"
      << "hr {margin:0px 6px}"
      << "p, form {margin: 4px 12px; font-size: 1.0em;}"
      << "table {margin: 10px 20px}"
      << "input {border-style:none; padding: 5px}" << body_style
      << "hr {height:0;border:0;overflow:visible;border-top:3px dotted white;}"
      << "table {border-collapse: collapse;border-spacing: "
         "0;background-color:aliceblue;border:solid 1px steelblue;}"
      << "table th {text-align:center;padding: 10px;background: "
         "steelblue;color: white;}"
      << "table td {padding: 10px; border-bottom: solid 1px steelblue; }"
      << ".schedule_disable { background-color: silver;}"
      << ".schedule_executable { background-color: greenyellow;}"
      << ".schedule_manual { background-color: greenyellow;}"
      << ".gauge{ position: relative; border:solid 1px steelblue; "
         "background-color:lightgray; width: 300px; margin: 6px 20px; }"
      << "div#inner { height: 20px; }"
      << "div#num { position: absolute; top: 0px; left: 0px; line-height: "
         "20px; text-align: center; width: 300px;}"
      << "</style>"
      << "<script>var checkSubmit = function(msg) { return confirm(msg); "
         "};</script>"
      << "</head>";

  httpd_resp_sendstr_chunk(request_data, response_body.str().c_str());
  response_body.str("");
  response_body.clear(std::stringstream::goodbit);

  response_body << "<body><h1>" << title << "</h1><hr>";

  if (weather_setting.IsActive()) {
    response_body << "<h2>Schedule (" << std::setfill('0') << std::setw(2)
                  << schedule_manager->GetCurrentMonth() << "/" << std::setw(2)
                  << schedule_manager->GetCurrentDay() << ")</h2>";

    // Create Schedule Table
    response_body << "<table><thead><tr>"
                  << "<th>ScheduleName</th>"
                  << "<th>Time</th>"
                  << "<th>Status</th>"
#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
                  << "<th>Amount Of Water</th>"
#endif
                  << "</tr></thead><tbody>";

    if (std::any_of(schedule_list.begin(), schedule_list.end(),
                    [](const ScheduleBaseUniquePtr &item) {
                      return item->IsVisible();
                    })) {
      // Found Visible Schedule Item
      for (const auto &schedule_item : schedule_list) {
        if (schedule_item->IsVisible()) {
          response_body << std::setfill('0') << "<tr class=\""
                        << ScheduleBase::StatusToRecordStyle(
                               schedule_item->GetStatus())
                        << "\">"
                        << "<td>" << schedule_item->GetName() << "</td>"
                        << "<td>" << std::setw(2) << schedule_item->GetHour()
                        << ":" << std::setw(2) << schedule_item->GetMinute()
                        << "</td>"
                        << "<td>"
                        << ScheduleBase::StatusToStr(schedule_item->GetStatus())
                        << "</td>"
#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
                        << "<td>";
          if (schedule_item->GetWaterFlow() < 0) {
            response_body << "-";
          } else {
            response_body << std::fixed << std::setprecision(2)
                          << (WaterFlowSensor::CountToCubicDecimeters(
                                  schedule_item->GetWaterFlow()) /
                              60.0f)
                          << "L";
          }
          response_body << "</td>"
#endif
                        << "</tr>";
        }
      }
    } else {
      // Not Found Visible Schedule Item
      response_body << "<tr><td colspan=\"3\">Empty</td></tr>";
    }

    response_body << "</tbody></table>";
  } else {
    response_body << "<p><span style=\"background-color:yellow;\">No settings "
                     "have been made.<span></p>";
  }

  httpd_resp_sendstr_chunk(request_data, response_body.str().c_str());
  response_body.str("");
  response_body.clear(std::stringstream::goodbit);

  // -- Status -----
  response_body << "<hr><h2>Status</h2>";
  response_body << "<h3>Valve Status</h3>";

  ValveExecutorSharedPtr executor =
      irrigation_interface->GetCurrentValveExecutor();
  if (executor) {
    response_body << "<p><span style=\"background:coral;\">Open</span>";
    if (executor->GetStatus() ==
        ValveExecutor::ExecutorStatus::EXECUTOR_SCHEDULE) {
      response_body << " &gt; Close At("
                    << Util::TimeToStr(
                           Util::EpochToLocalTime(executor->GetCloseEpoch()))
                    << ")";
    } else if (executor->GetStatus() ==
               ValveExecutor::ExecutorStatus::EXECUTOR_MANUAL_START) {
      response_body << " &gt; Manual";
    }
    response_body << "</p>";

#if CONFIG_IS_ENABLE_WATER_FLOW_SENSOR
    response_body << "<p>"
                  << "Water Flow : " << std::fixed << std::setprecision(2)
                  << WaterFlowSensor::CountToCubicDecimeters(
                         irrigation_interface->GetWaterFlowHz())
                  << "L/min"
                  << "</p>";
#endif
  } else {
    response_body << "<p>Close</p>";
  }

  response_body << "<h3>Weather Forecast</h3>"
                << "<p>" << weatherInfo.str() << "</p>";

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  response_body << "<h3>Warter Level</h3>"
                << "<div class=\"gauge\"><div id=\"inner\" style=\"width:"
                << water_level << "%;  background-color:"
                << ::water_levelToColorName(water_level)
                << ";\"></div><div id=\"num\">" << water_level
                << "%</div></div>";
#endif

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  response_body << "<h3>Battery Voltage</h3>"
                << "<div class=\"gauge\"><div id=\"inner\" style=\"width:"
                << voltageGuage << "%; background-color:"
                << ::voltageToColorName(batteryVoltage)
                << ";\"></div><div id=\"num\">" << std::setfill('0')
                << std::fixed << std::setprecision(2) << batteryVoltage
                << "[V]</div></div>";
#endif

#if CONFIG_IS_ENABLE_MQTT_PUBLISH
  response_body << "<h3>MQTT Broker</h3>";
  if (irrigation_interface->IsConnectedMQTTBroker()) {
    response_body << "<p>Connected</p>";
  } else {
    response_body << "<p>Disconnected</p>";
  }
#endif

  response_body << "<h3>Last Watering Date</h3><p>"
                << Util::TimeToDayStr(Util::EpochToLocalTime(
                       irrigation_interface->GetLastWateringEpoch()))
                << "</p>";

  response_body << "<h3>System Time</h3><p>" << Util::GetNowTimeStr() << " ("
                << CONFIG_LOCAL_TIME_ZONE << ")</p>";
  response_body << "<h3>System Boot Time</h3><p>"
                << Util::TimeToStr(Util::EpochToLocalTime(
                       irrigation_interface->GetSystemBootTime()))
                << "</p>";

  httpd_resp_sendstr_chunk(request_data, response_body.str().c_str());
  response_body.str("");
  response_body.clear(std::stringstream::goodbit);

  // -- Operation -----
  response_body
      << "<hr><h2>Operation</h2>"
      << "<p><form action=\"/manual_watering\" method=\"post\">"
      << "Manual Watering. time (sec) : <input type=\"number\" name=\"second\" "
         "value=\"10\" min=\"1\" max=\""
      << WEB_RELAY_OPEN_MAX_SECOND << "\"> "
      << "<input type=\"submit\" value=\"Start\">"
      << "</form></p>"
      << "<p><form action=\"/emergency_stop\" method=\"post\">"
      << "Emergency Stop : <input type=\"submit\" value=\"Stop\">"
      << "</form></p>"
      << "<p><form action=\"/upload_setting\" enctype=\"multipart/form-data\" "
         "method=\"post\" style=\"display:inline;\">"
      << "Watering Setting File : <input type=\"file\" "
         "name=\"setting_file\"><input type=\"submit\" value=\"Upload\">"
      << "</form>";

  if (weather_setting.IsActive()) {
    response_body
        << ":<form action=\"/download_setting\" method=\"get\" "
           "style=\"display:inline;\"><input type=\"submit\" "
           "value=\"Download\"></form>"
        << ":<form action=\"/delete_setting\" method=\"post\" "
           "style=\"display:inline;\" onsubmit=\"return checkSubmit('Are you "
           "sure you want to delete setting?');\">"
        << "<input type=\"submit\" value=\"Delete\"></form>"
        << "</p>";
  }

  response_body
      << "<p><form action=\"/system_restart\" method=\"post\" "
         "style=\"display:inline;\" onsubmit=\"return checkSubmit('Are you "
         "sure you want to restart the system?');\">"
      << "System Restart : <input type=\"submit\" "
         "value=\"Restart\"></form></p>";

  httpd_resp_sendstr_chunk(request_data, response_body.str().c_str());
  response_body.str("");
  response_body.clear(std::stringstream::goodbit);

  // -- Information -----
  response_body << "<hr>"
                << "<p>Version : " << GIT_VERSION << "</p>"
                << "</body></html>";

  httpd_resp_sendstr_chunk(request_data, response_body.str().c_str());
  response_body.str("");
  response_body.clear(std::stringstream::goodbit);

  httpd_resp_sendstr_chunk(request_data, nullptr);
  return ESP_OK;
}

esp_err_t HttpdServerTask::ManualWateringHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Post:ManualWatering");

  if (!request_data->user_ctx) {
    ESP_LOGE(TAG, "Failed user_ctx is null");
    return ESP_FAIL;
  }

  // Receive Post Data
  static constexpr size_t SCRATCH_BUFSIZE = 256;
  const int total_len = request_data->content_len;

  if (SCRATCH_BUFSIZE <= total_len) {
    httpd_resp_send_err(request_data, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "content too long");
    return ESP_FAIL;
  }
  char buf[SCRATCH_BUFSIZE] = {};
  int cur_len = 0;
  int received = 0;
  while (cur_len < total_len) {
    received = httpd_req_recv(request_data, buf + cur_len, total_len);
    if (received <= 0) {
      httpd_resp_send_err(request_data, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  ESP_LOGV(TAG, " Recv Data Length:%d Data:%s", total_len, buf);

  // Parse
  int valve_open_second = 0;
  std::vector<std::string> elements = Util::SplitString(buf, '=');
  if (elements.size() == 2) {
    if (elements.at(0) == "second") {
      valve_open_second =
          std::max(1, std::min(WEB_RELAY_OPEN_MAX_SECOND,
                               static_cast<int>(std::stol(elements.at(1)))));
    }
  }

  // Valve Open
  HttpdServerTask *const httpd_server_task =
      static_cast<HttpdServerTask *>(request_data->user_ctx);
  if (!httpd_server_task) {
    ESP_LOGE(TAG, "Failed HttpdServerTask is null");
    return ESP_FAIL;
  }
  const IrrigationInterfaceSharedPtr irrigation_interface =
      httpd_server_task->irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Add Open");
  std::tm now = Util::GetLocalTime();
  const ScheduleManagerSharedPtr schedule_manager =
      irrigation_interface->GetScheduleManager().lock();
  if (!schedule_manager) {
    ESP_LOGE(TAG, "Failed Schedule Manager is null");
    return ESP_FAIL;
  }
  schedule_manager->AddSchedule(std::make_unique<ScheduleWatering>(
      irrigation_interface, now.tm_hour, now.tm_min, valve_open_second));
  schedule_manager->SortScheduleTime();

  // Redirect
  httpd_resp_set_status(request_data, "303 See Other");
  httpd_resp_set_hdr(request_data, "Location", "/");
  httpd_resp_send(request_data, NULL, 0);
  return ESP_OK;
}

esp_err_t HttpdServerTask::EmergencyStopHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Post:EmergencyStop");

  if (!request_data->user_ctx) {
    ESP_LOGE(TAG, "Failed user_ctx is null");
    return ESP_FAIL;
  }

  // Valve Open
  HttpdServerTask *const httpd_server_task =
      static_cast<HttpdServerTask *>(request_data->user_ctx);
  if (!httpd_server_task) {
    ESP_LOGE(TAG, "Failed HttpdServerTask is null");
    return ESP_FAIL;
  }
  const IrrigationInterfaceSharedPtr irrigation_interface =
      httpd_server_task->irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return ESP_FAIL;
  }
  irrigation_interface->ForceStopValve();

  // Redirect
  httpd_resp_set_status(request_data, "303 See Other");
  httpd_resp_set_hdr(request_data, "Location", "/");
  httpd_resp_send(request_data, NULL, 0);
  return ESP_OK;
}

esp_err_t HttpdServerTask::UploadSettingHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Post:UploadSetting");

  // Check
  if (!request_data->user_ctx) {
    ESP_LOGE(TAG, "Failed user_ctx is null");
    return ESP_FAIL;
  }
  HttpdServerTask *const httpd_server_task =
      static_cast<HttpdServerTask *>(request_data->user_ctx);
  if (!httpd_server_task) {
    ESP_LOGE(TAG, "Failed HttpdServerTask is null");
    return ESP_FAIL;
  }
  const IrrigationInterfaceSharedPtr irrigation_interface =
      httpd_server_task->irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return ESP_FAIL;
  }

  // Receive Header (get Multipart boundary)
  static constexpr char *const HTTP_HEADER_CONTENT_TYPE =
      (char *)"Content-Type";
  const size_t content_typeHeaderLen =
      httpd_req_get_hdr_value_len(request_data, HTTP_HEADER_CONTENT_TYPE);
  if (content_typeHeaderLen == 0) {
    ESP_LOGE(TAG, "Not Found Rqeust Header Empty: %s",
             HTTP_HEADER_CONTENT_TYPE);
    return ESP_FAIL;
  }
  std::string content_type;
  content_type.resize(content_typeHeaderLen);
  if (httpd_req_get_hdr_value_str(request_data, HTTP_HEADER_CONTENT_TYPE,
                                  &content_type.at(0),
                                  content_typeHeaderLen + 1) != ESP_OK) {
    ESP_LOGE(TAG, "Not Found Rqeust Header : %s", HTTP_HEADER_CONTENT_TYPE);
    return ESP_FAIL;
  }
  // ESP_LOGV(TAG, "Found header => %s: %s",HTTP_HEADER_CONTENT_TYPE,
  // content_type.c_str());

  // Get Boundary String
  const std::string BOUNDARY_STR = "boundary=";
  std::string::size_type pos = content_type.find(BOUNDARY_STR);
  if (pos == std::string::npos) {
    ESP_LOGE(TAG, "Failed Get Rqeust Header : %s", HTTP_HEADER_CONTENT_TYPE);
    return ESP_FAIL;
  }
  const std::string boundaryStr =
      content_type.substr(pos + BOUNDARY_STR.length());
  const std::string::size_type boundaryStrLength = boundaryStr.length();
  // ESP_LOGV(TAG, "Boundary => %s", boundaryStr.c_str());

  // Receive Post Data
  static constexpr size_t MAX_FILE_SIZE = 10240;  // 10 KB
  const int total_len = request_data->content_len;
  if (MAX_FILE_SIZE <= total_len) {
    httpd_resp_send_err(request_data, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "content too long");
    return ESP_FAIL;
  }

  ESP_LOGV(TAG, "Receive post data length:%d", total_len);
  std::string body;
  body.resize(total_len + 1);
  int cur_len = 0;
  int received = 0;
  while (cur_len < total_len) {
    received = httpd_req_recv(request_data, &body.at(0) + cur_len, total_len);
    if (received <= 0) {
      httpd_resp_send_err(request_data, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  // ESP_LOGE(TAG, "Recv File \n-------\n%s", body.c_str());

  // Split multipart data
  std::vector<std::string> list;
  if (boundaryStrLength == 0) {
    list.push_back(body);
  } else {
    std::string::size_type offset = std::string::size_type(0);
    while (true) {
      std::string::size_type pos =
          body.find(std::string("--") + boundaryStr, offset);
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
  std::string payload_json_data;
  for (std::vector<std::string>::const_iterator iter = list.begin();
       iter != list.end(); ++iter) {
    // ESP_LOGV(TAG, "Split\n%s", (*iter).c_str());
    if (iter->find("name=\"setting_file\"") != std::string::npos) {
      std::string::size_type begin = iter->find("\r\n\r\n") + 4;
      std::string::size_type end = iter->rfind("\r\n");
      if ((end - begin) <= 0) {
        break;
      }
      payload_json_data = iter->substr(begin, end - begin);
      ESP_LOGV(TAG, "OK Payload-------\n%s\n-----", payload_json_data.c_str());
      break;
    }
  }

  // Parse
  WateringSetting &weather_setting = irrigation_interface->GetWateringSetting();
  if (!weather_setting.SetSettingData(payload_json_data)) {
    httpd_resp_send_err(request_data, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Invalid Data");
    return ESP_FAIL;
  }

  // Save
  if (!WateringSetting::Save(payload_json_data)) {
    httpd_resp_send_err(request_data, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed save");
    return ESP_FAIL;
  }

  // Init Schedule
  const ScheduleManagerSharedPtr schedule_manager =
      irrigation_interface->GetScheduleManager().lock();
  if (!schedule_manager) {
    ESP_LOGE(TAG, "Failed Schedule Manager is null");
    return ESP_FAIL;
  }
  const std::tm now_data = Util::GetLocalTime();
  schedule_manager->InitializeNewDay(now_data);

  // Redirect
  httpd_resp_set_status(request_data, "303 See Other");
  httpd_resp_set_hdr(request_data, "Location", "/");
  httpd_resp_send(request_data, NULL, 0);
  return ESP_OK;
}

esp_err_t HttpdServerTask::DownloadSettingHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Post:DownloadSetting");

  std::string raw_Setting_data;
  if (!WateringSetting::Load(raw_Setting_data)) {
    ESP_LOGE(TAG, "Failed Load Setting File");
    return ESP_FAIL;
  }

  httpd_resp_set_type(request_data, "application/json");
  httpd_resp_send(request_data, raw_Setting_data.c_str(),
                  raw_Setting_data.length());
  return ESP_OK;
}

esp_err_t HttpdServerTask::DeleteSettingHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Post:DeleteSetting");

  if (!request_data->user_ctx) {
    ESP_LOGE(TAG, "Failed user_ctx is null");
    return ESP_FAIL;
  }

  HttpdServerTask *const httpd_server_task =
      static_cast<HttpdServerTask *>(request_data->user_ctx);
  if (!httpd_server_task) {
    ESP_LOGE(TAG, "Failed HttpdServerTask is null");
    return ESP_FAIL;
  }
  const IrrigationInterfaceSharedPtr irrigation_interface =
      httpd_server_task->irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return ESP_FAIL;
  }

  // Delete
  if (!WateringSetting::Delete()) {
    httpd_resp_send_err(request_data, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed delete");
    return ESP_FAIL;
  }

  // WateringSetting init
  WateringSetting &watering_settings =
      irrigation_interface->GetWateringSetting();
  watering_settings = WateringSetting();

  // Init Schedule
  const ScheduleManagerSharedPtr schedule_manager =
      irrigation_interface->GetScheduleManager().lock();
  if (!schedule_manager) {
    ESP_LOGE(TAG, "Failed Schedule Manager is null");
    return ESP_FAIL;
  }
  const std::tm now_data = Util::GetLocalTime();
  schedule_manager->InitializeNewDay(now_data);

  // Redirect
  httpd_resp_set_status(request_data, "303 See Other");
  httpd_resp_set_hdr(request_data, "Location", "/");
  httpd_resp_send(request_data, NULL, 0);
  return ESP_OK;
}

esp_err_t HttpdServerTask::GetVoltageHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Get:GetVoltage");

#if CONFIG_IS_ENABLE_VOLTAGE_CHECK
  const float voltage = Util::GetVoltage();

  // Generate Response
  std::stringstream response_body;
  response_body << "{\"voltage\":" << std::setfill('0') << std::fixed
                << std::setprecision(2) << voltage << "}";
#else
  std::stringstream response_body;
  response_body << "{\"voltage\": 0}";

#endif
  httpd_resp_set_type(request_data, "application/json");
  httpd_resp_send(request_data, response_body.str().c_str(),
                  response_body.str().length());
  return ESP_OK;
}

esp_err_t HttpdServerTask::GetWaterLevelHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Get:Getwater_level");

#if CONFIG_IS_ENABLE_WATER_LEVEL_CHECK
  HttpdServerTask *const httpd_server_task =
      static_cast<HttpdServerTask *>(request_data->user_ctx);
  if (!httpd_server_task) {
    ESP_LOGE(TAG, "Failed HttpdServerTask is null");
    return ESP_FAIL;
  }
  const IrrigationInterfaceSharedPtr irrigation_interface =
      httpd_server_task->irrigation_interface_.lock();
  if (!irrigation_interface) {
    ESP_LOGE(TAG, "Failed IrrigationInterface is null");
    return ESP_FAIL;
  }

  const float water_level = irrigation_interface->GetWaterLevel();

  // Generate Response
  std::stringstream response_body;
  response_body << "{\"water_level\":" << std::setfill('0') << std::fixed
                << std::setprecision(2) << water_level << "}";
#else
  ESP_LOGI(TAG, "WATER LEVEL CHECK 5 ");
  std::stringstream response_body;
  response_body << "{\"water_level\": 0}";

#endif
  httpd_resp_set_type(request_data, "application/json");
  httpd_resp_send(request_data, response_body.str().c_str(),
                  response_body.str().length());
  return ESP_OK;
}

esp_err_t HttpdServerTask::RestartSystemHandler(httpd_req_t *request_data) {
  ESP_LOGV(TAG, "WebServer Request Recv. Post:RestartSystem");
  // httpd_resp_send(request_data, NULL, 0);

  esp_restart();
  return ESP_OK;
}

esp_err_t HttpdServerTask::ErrorNotFoundHandler(httpd_req_t *request_data,
                                                httpd_err_code_t errCode) {
  httpd_resp_send_err(request_data, HTTPD_404_NOT_FOUND,
                      "HTTP Status 404 Not Found");
  return ESP_FAIL;
}

}  // namespace IrrigationSystem

// EOF
