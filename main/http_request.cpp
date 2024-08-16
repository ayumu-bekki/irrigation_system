// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "http_request.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>

#include "logger.h"

namespace IrrigationSystem {

HttpRequest::HttpRequest()
    : status_(STATUS_WAIT),
      url_(),
      response_body_(),
      server_root_cert_(nullptr) {}

void HttpRequest::Request(const std::string &url) {
  status_ = STATUS_WAIT;

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  esp_http_client_config_t config = {
      .url = url.c_str(),
      .event_handler = this->EventHandle,
      .user_data = this,
  };

  if (server_root_cert_) {
    config.cert_pem = server_root_cert_;
  }

  esp_http_client_handle_t client = esp_http_client_init(&config);
  const esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    ESP_LOGV(TAG, "HTTP Request Status = %d, content_length = %d",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
    if (HttpStatus_Ok == esp_http_client_get_status_code(client)) {
      status_ = STATUS_OK;
    } else {
      status_ = STATUS_NG;
    }
  } else {
    ESP_LOGW(TAG, "Failed HTTP Request");
    status_ = STATUS_NG;
  }

  esp_http_client_cleanup(client);
}

void HttpRequest::EnableTLS(const char *const cert) {
  server_root_cert_ = cert;
}

void HttpRequest::AddResponseBody(const size_t length, const void *data) {
  response_body_.insert(response_body_.end(), static_cast<const char *>(data),
                        static_cast<const char *>(data) + length);
}

const std::string HttpRequest::GetResponseBody() const {
  return std::string(response_body_.begin(), response_body_.end());
}

HttpRequest::Status HttpRequest::GetStatus() const { return status_; }

void HttpRequest::Event(esp_http_client_event_t *const event_data) {
  if (event_data->event_id == HTTP_EVENT_ERROR) {
    ESP_LOGW(TAG, "HTTP_EVENT_ERROR");
  } else if (event_data->event_id == HTTP_EVENT_ON_DATA) {
    if (!esp_http_client_is_chunked_response(event_data->client)) {
      AddResponseBody(event_data->data_len, event_data->data);
    }
  }
}

esp_err_t HttpRequest::EventHandle(esp_http_client_event_t *event_data) {
  if (!event_data->user_data) {
    ESP_LOGE(TAG, "UserData Is Null");
    return ESP_FAIL;
  }
  static_cast<HttpRequest *>(event_data->user_data)->Event(event_data);
  return ESP_OK;
}

}  // namespace IrrigationSystem

// EOF
