#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <string>
#include <vector>

#include <esp_system.h>
#include <esp_http_client.h>

namespace IrrigationSystem {

/// HttpGetRequest (synchronous process)
class HttpRequest
{
public:
    enum Status
    {
        STATUS_WAIT,
        STATUS_OK,
        STATUS_NG,
    };

public:
    HttpRequest();
    
    /// Begin Request
    void Request(const std::string& url);

    void EnableTLS(const char *const pCert);

    const std::string GetResponseBody() const;
    
    Status GetStatus() const;

private:
    void Event(esp_http_client_event_t *const pEventData);

    void AddResponseBody(const size_t length, const void* data);

public:
    static esp_err_t EventHandle(esp_http_client_event_t *pEventData);


private:
    Status m_Status;
    std::string m_Url;
    std::vector<char> m_ResponseBody;
    const char* m_pServerRootCert;
};

} // IrrigationSystem

#endif // HTTP_REQUEST_H_
// EOF
