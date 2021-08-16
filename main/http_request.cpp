// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "http_request.h"

#include <iterator>
#include <algorithm>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_http_client.h>

#include "define.h"

namespace IrrigationSystem {

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    if (!evt->user_data) {
        ESP_LOGI(TAG, "UserData Is Null");
        return ESP_OK;
    }

    HttpRequest *const pHttpRequest = static_cast<HttpRequest*>(evt->user_data);

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                pHttpRequest->AddResponseBody(evt->data_len, evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}


HttpRequest::HttpRequest() 
    :m_Status(STATUS_WAIT)
    ,m_Url()
    ,m_ResponseBody()
{}

void HttpRequest::Request(const std::string& url)
{
    m_Status = STATUS_WAIT;

    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    esp_http_client_config_t config = {
            .url = url.c_str(),
            .event_handler = _http_event_handle,
            .user_data = this,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    const esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Status = %d, content_length = %d",
           esp_http_client_get_status_code(client),
           esp_http_client_get_content_length(client));
        if (HttpStatus_Ok == esp_http_client_get_status_code(client)) {
            m_Status = STATUS_OK;
        } else {
            m_Status = STATUS_NG;
        }
    } else {
        ESP_LOGI(TAG, "HTTP ESP NG");
        m_Status = STATUS_NG;
    }

    esp_http_client_cleanup(client);
}

void HttpRequest::AddResponseBody(const size_t length, const void* data)
{
    std::copy(static_cast<const char*>(data), static_cast<const char*>(data) + length, std::back_inserter(m_ResponseBody));
}

const std::string HttpRequest::GetResponseBody() const
{
    return std::string(m_ResponseBody.data());
}

HttpRequest::Status HttpRequest::GetStatus() const
{
    return m_Status;
}


} // IrrigationSystem

// EOF
