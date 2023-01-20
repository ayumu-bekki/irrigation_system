#ifndef WATERING_SETTING_H_
#define WATERING_SETTING_H_
// ESP32 Irrigation system
// (C)2021 bekki.jp

// Include ----------------------
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

#include <cJSON.h>


namespace IrrigationSystem {

class WateringSetting final
{
private:
    static constexpr char *const SETTING_FILE_NAME = (char*)"watering_setting.json";

public:
    using WateringHourList = std::vector<std::int32_t>;

    enum WateringMode : std::int32_t
    {
        WATERING_MODE_NONE,
        WATERING_MODE_SIMPLE,  // simple
        WATERING_MODE_ADVANCE, // advance
        WATERING_MODE_MAX,
    };

    struct WateringType
    {
        std::string WateringType;
        std::int32_t DaySpan;
        std::vector<std::int32_t> WateringHours;
    };
    using WateringTypeDict = std::unordered_map<std::string, WateringType>;

    struct TemperatureWatering
    {
        std::int32_t Temperature;
        std::string NormalType;
        std::string RainType;
    };
    using TemperatureWateringList = std::vector<TemperatureWatering>;

    using MonthToTypeDict = std::unordered_map<std::string, std::string>;

public:
    WateringSetting();

    bool SetSettingData(const std::string& body);
    
    bool IsActive() const;

    WateringMode GetWateringMode() const;
    std::int32_t GetWateringSec() const;
    const WateringHourList& GetWateringHourList() const;
    std::int32_t GetJMAAreaPathCode() const;
    std::int32_t GetJMALocalCode() const;
    std::int32_t GetJMAAMeDAS() const;
    const WateringTypeDict& GetWateringTypeDict() const;
    const TemperatureWateringList& GetTemperatureWateringList() const;
    const MonthToTypeDict& GetMonthToTypeDict() const;
    float GetValvePowerBaseRate() const;
    float GetValvePowerBaseVoltage() const;
    float GetValvePowerVoltageRate() const;

private:
    bool Parse(const std::string& body) noexcept;

    bool ParseSimple(cJSON* pJsonRoot) noexcept(false);
    bool ParseAdvance(cJSON* pJsonRoot) noexcept(false);

public:
    static bool Save(const std::string& body);
    static bool Load(std::string& body);
    static bool Delete();
    
private:
    /// Is Read Setting
    bool m_IsActive;
    /// Current Watering Mode
    WateringMode m_WateringMode;
    
    // Share --------------------------
    /// Watering Second
    std::int32_t m_WateringSec;

    // Simple --------------------------
    /// Watering Hour List
    WateringHourList m_WateringHourList;
    
    // Advanced --------------------------
    /// Area path code for weather forecast determination.
    std::int32_t m_JMAAreaPathCode;
    /// Area number for weather forecast determination. 
    std::int32_t m_JMALocalCode;
    /// AMeDAS observation point number for weather forecast determination.
    std::int32_t m_JMAAMeDAS;
    /// Watering Type Dictionary
    WateringTypeDict m_WateringTypeDict;
    /// Watering Tempature List
    TemperatureWateringList m_TemperatureWateringList;
    /// Month To Type Dict
    MonthToTypeDict m_MonthToTypeDict;
    /// ValvePowerBaseRate
    float m_BaseRate;
    /// ValvePowerBaseVoltage
    float m_BaseVoltage;
    /// ValvePowerVoltageRate
    float m_VoltageRate;
};

} // IrrigationSystem


#endif // WATERING_SETTING_H_
// EOF
