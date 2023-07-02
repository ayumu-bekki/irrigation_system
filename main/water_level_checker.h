#ifndef WATER_LEVEL_CHECKER_H_
#define WATER_LEVEL_CHECKER_H_
// ESP32 Irrigation system
// (C)2023 bekki.jp

// Include ----------------------
#include <soc/soc.h>

#include "pwm.h"

namespace IrrigationSystem {

class WaterLevelChecker final
{
public:
    WaterLevelChecker();

    void Initialize();
    void Check();

    float GetWaterLevel() const;

private:
    float m_WaterLevel;
    Pwm m_pwm;
};

} // IrrigationSystem


#endif // WATER_LEVEL_CHECKER_H_
// EOF
