#ifndef IRRIGATION_INTERFACE_H_
#define IRRIGATION_INTERFACE_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <chrono>

namespace IrrigationSystem {

class ScheduleManager;

class IrrigationInterface
{
public:
    virtual ~IrrigationInterface() {}
    
    virtual void RelayAddOpenSecond(const int second) = 0;
    virtual void RelayForceClose() = 0;
    virtual std::time_t RelayCloseEpoch() const = 0;

    virtual ScheduleManager& GetScheduleManager() = 0;
};

} // IrrigationSystem

#endif // IRRIGATION_INTERFACE_H_
// EOF
