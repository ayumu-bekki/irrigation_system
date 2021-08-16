#ifndef IRRIGATION_CONTROLLER_H_
#define IRRIGATION_CONTROLLER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_interface.h"
#include "wifi_manager.h"
#include "relay_task.h"


namespace IrrigationSystem {

/// IrrigationController
class IrrigationController : public IrrigationInterface
{
public:
    IrrigationController();

    /// Start
    void Start();

    void RequestRelayOpen(const int second) override;

private:
    WifiManager m_WifiManager;
    RelayTask m_RelayTask;
};

} // IrrigationSystem

#endif // IRRIGATION_CONTROLLER_H_
// EOF
