#ifndef WATERING_RECORD_H_
#define WATERING_RECORD_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <ctime>


namespace IrrigationSystem {

class WateringRecord final
{
private:
    static constexpr char *const RECORD_FILE_NAME = (char*)"watering_record.json";

public:
    WateringRecord();

    bool Save() const;
    bool Load() noexcept;

    time_t GetLastWateringEpoch() const;
    
private:
    time_t m_LastWateringEpoch;
    
};

} // IrrigationSystem


#endif // WATERING_RECORD_H_
// EOF
