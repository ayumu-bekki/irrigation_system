#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <esp_http_client.h>
#include <esp_system.h>

#include <string>
#include <vector>

namespace IrrigationSystem {

/// HttpGetRequest (synchronous process)
class HttpRequest {
 public:
  enum Status {
    STATUS_WAIT,
    STATUS_OK,
    STATUS_NG,
  };

 public:
  HttpRequest();

  /// Begin Request
  void Request(const std::string& url);

  void EnableTLS(const char* const cert);

  const std::string GetResponseBody() const;

  Status GetStatus() const;

 private:
  void Event(esp_http_client_event_t* const event_data);

  void AddResponseBody(const size_t length, const void* data);

 public:
  static esp_err_t EventHandle(esp_http_client_event_t* event_data);

 private:
  Status status_;
  std::string url_;
  std::vector<char> response_body_;
  const char* server_root_cert_;
};

}  // namespace IrrigationSystem

#endif  // HTTP_REQUEST_H_
// EOF
