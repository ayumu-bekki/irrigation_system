// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "http_request.h"

#include <iterator>
#include <algorithm>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.h"

namespace IrrigationSystem {

HttpRequest::HttpRequest() 
    :m_Status(STATUS_WAIT)
    ,m_Url()
    ,m_ResponseBody()
    ,m_pServerRootCert(nullptr)
{}

void HttpRequest::Request(const std::string& url)
{
    m_Status = STATUS_WAIT;

    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    esp_http_client_config_t config = {
        .url = url.c_str(),
        .event_handler = this->EventHandle,
        .user_data = this,
    };

    if (m_pServerRootCert) {
        config.cert_pem = m_pServerRootCert;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);
    const esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGV(TAG, "Status = %d, content_length = %d",
           esp_http_client_get_status_code(client),
           esp_http_client_get_content_length(client));
        if (HttpStatus_Ok == esp_http_client_get_status_code(client)) {
            m_Status = STATUS_OK;
        } else {
            m_Status = STATUS_NG;
        }
    } else {
        ESP_LOGW(TAG, "HTTP ESP NG");
        m_Status = STATUS_NG;
    }

    esp_http_client_cleanup(client);
}

void HttpRequest::EnableTLS(const char *const pCert)
{
    m_pServerRootCert = pCert;
}

void HttpRequest::AddResponseBody(const size_t length, const void* data)
{
    m_ResponseBody.insert(m_ResponseBody.end(), static_cast<const char*>(data), static_cast<const char*>(data) + length);
}

const std::string HttpRequest::GetResponseBody() const
{
    return std::string(m_ResponseBody.begin(), m_ResponseBody.end());
}

HttpRequest::Status HttpRequest::GetStatus() const
{
    return m_Status;
}

void HttpRequest::Event(esp_http_client_event_t *const pEventData)
{
    if (pEventData->event_id == HTTP_EVENT_ERROR) {
        ESP_LOGW(TAG, "HTTP_EVENT_ERROR");
    } else if (pEventData->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGV(TAG, "HTTP_EVENT_ON_DATA, len=%d", pEventData->data_len);
        if (!esp_http_client_is_chunked_response(pEventData->client)) {
            AddResponseBody(pEventData->data_len, pEventData->data);
        }
    }
}

esp_err_t HttpRequest::EventHandle(esp_http_client_event_t *pEventData)
{
    if (!pEventData->user_data) {
        ESP_LOGE(TAG, "UserData Is Null");
        return ESP_FAIL;
    }
    static_cast<HttpRequest*>(pEventData->user_data)->Event(pEventData);
    return ESP_OK;
}


} // IrrigationSystem

// EOF
