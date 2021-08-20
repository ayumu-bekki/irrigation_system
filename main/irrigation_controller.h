#ifndef IRRIGATION_CONTROLLER_H_
#define IRRIGATION_CONTROLLER_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_interface.h"
#include "wifi_manager.h"
#include "relay_task.h"
#include "schedule_manager.h"


namespace IrrigationSystem {

/// IrrigationController
class IrrigationController final : public IrrigationInterface
{
public:
    IrrigationController();

    /// Start
    void Start();

public:
    /// (IrrigationInterface:override)
    void RelayAddOpenSecond(const int second) override;

    /// (IrrigationInterface:override)
    void RelayForceClose() override;

    /// (IrrigationInterface:override)
    std::time_t RelayCloseEpoch() const override;

    /// (IrrigationInterface:override)
    ScheduleManager& GetScheduleManager() override;

private:
    WifiManager m_WifiManager;
    RelayTask m_RelayTask;
    ScheduleManager m_ScheduleManager;
};

} // IrrigationSystem

#endif // IRRIGATION_CONTROLLER_H_
// EOF
