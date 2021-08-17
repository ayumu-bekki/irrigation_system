#ifndef IRRIGATION_INTERFACE_H_
#define IRRIGATION_INTERFACE_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

namespace IrrigationSystem {

class ScheduleManager;

class IrrigationInterface
{
public:
    virtual ~IrrigationInterface() {}
    
    virtual void RequestRelayOpen(const int second) = 0;

    virtual ScheduleManager& GetScheduleManager() = 0;
};

} // IrrigationSystem

#endif // IRRIGATION_INTERFACE_H_
// EOF
