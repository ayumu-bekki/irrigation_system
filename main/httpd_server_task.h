#ifndef HTTPD_SERVER_TASK_H_
#define HTTPD_SERVER_TASK_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <soc/soc.h>
#include <esp_http_server.h>

#include "task.h"
#include "irrigation_interface.h"

namespace IrrigationSystem {

class HttpdServerTask : public Task
{
public:
    static constexpr char *const TASK_NAME = (char*)"HttpdServerTask";
    static constexpr int PRIORITY = Task::PRIORITY_LOW;
    static constexpr int CORE_ID = APP_CPU_NUM;

public:
    explicit HttpdServerTask(IrrigationInterface *const pParent);
   
    void Initialize() override;

    void Update() override;

private:
    httpd_handle_t StartWebServer();
    void StopWebServer();

private:
    static esp_err_t RootHandler(httpd_req_t *pHttpRequestData);
    static esp_err_t OpenRelayHandler(httpd_req_t *pHttpRequestData);
    static esp_err_t ErrorNotFoundHandler(httpd_req_t *pHttpRequestData, httpd_err_code_t errCode);

private:
    IrrigationInterface* m_pIrricationInterface;
    httpd_handle_t m_HttpdHandle;
};

} // IrrigationSystem

#endif // HTTPD_SERVER_TASK_H_
// EOF
