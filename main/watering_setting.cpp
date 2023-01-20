// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "watering_setting.h"

#include <stdexcept>
#include <sstream>
#include <algorithm>

#include <cJSON.h>

#include "logger.h"
#include "file_system.h"

namespace IrrigationSystem {

namespace {
    const std::string jsonWateringModeTable[WateringSetting::WATERING_MODE_MAX] = {
        "",         // WATERING_MODE_NONE
        "simple",   // WATERING_MODE_SIMPLE
        "advance",  // WATERING_MODE_ADVANCE
    };
}

WateringSetting::WateringSetting()
    :m_IsActive(false)
    ,m_WateringMode(WATERING_MODE_NONE)
    ,m_WateringSec(0)
    ,m_WateringHourList()
    ,m_JMAAreaPathCode(0)
    ,m_JMALocalCode(0)
    ,m_JMAAMeDAS(0)
    ,m_WateringTypeDict()
    ,m_TemperatureWateringList()
    ,m_MonthToTypeDict()
    ,m_BaseRate(0.0f)
    ,m_BaseVoltage(0.0f)
    ,m_VoltageRate(0.0f)
{}

bool WateringSetting::SetSettingData(const std::string& body)
{
    return Parse(body);
}

bool WateringSetting::IsActive() const
{
    return m_IsActive;
}

WateringSetting::WateringMode WateringSetting::GetWateringMode() const
{
    return m_WateringMode;
}

std::int32_t WateringSetting::GetWateringSec() const
{
    return m_WateringSec;
}

const std::vector<std::int32_t>& WateringSetting::GetWateringHourList() const
{
    return m_WateringHourList;
}

std::int32_t WateringSetting::GetJMAAreaPathCode() const
{
    return m_JMAAreaPathCode;
}

std::int32_t WateringSetting::GetJMALocalCode() const
{
    return m_JMALocalCode;
}

std::int32_t WateringSetting::GetJMAAMeDAS() const
{
    return m_JMAAMeDAS;
}

const WateringSetting::WateringTypeDict& WateringSetting::GetWateringTypeDict() const
{
    return m_WateringTypeDict;
}

const WateringSetting::TemperatureWateringList& WateringSetting::GetTemperatureWateringList() const
{
    return m_TemperatureWateringList;
}

const WateringSetting::MonthToTypeDict& WateringSetting::GetMonthToTypeDict() const
{
    return m_MonthToTypeDict;
}

bool WateringSetting::Parse(const std::string& body) noexcept
{
    // Initialize
    m_WateringMode = WATERING_MODE_NONE;

    cJSON* pJsonRoot = nullptr;
    try {
        pJsonRoot = cJSON_Parse(body.c_str());
        if (!pJsonRoot){
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr)
            {
                throw std::runtime_error(error_ptr);
            }
            throw std::runtime_error("Json Parse Error.");
        }
    
        // Get Watering Mode
        const cJSON *const pJsonWateringMode = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "watering_mode");
        if (!cJSON_IsString(pJsonWateringMode)) {
            throw std::runtime_error("Illegal object type watering_mode.");
        }
        const std::string wateringModeStr = pJsonWateringMode->valuestring;
    
        for (std::int32_t idx = WATERING_MODE_NONE; idx < WATERING_MODE_MAX; ++idx) {
            if (jsonWateringModeTable[idx] == wateringModeStr) {
                m_WateringMode = static_cast<WateringMode>(idx);
                break;
            }
        }

        if (m_WateringMode == WATERING_MODE_NONE) {
            throw std::runtime_error("Illegal type watering_mode.");
        } else if (m_WateringMode == WATERING_MODE_SIMPLE) {
            return ParseSimple(pJsonRoot);
        } else if (m_WateringMode == WATERING_MODE_ADVANCE) {
            return ParseAdvance(pJsonRoot);
        }
        
    } catch (const std::invalid_argument& e) {
        ESP_LOGW(TAG, "Catch Exception. Invalid Argument String to Number.");
        return false;
    } catch (const std::out_of_range& e) {
        ESP_LOGW(TAG, "Catch Exception. Out Of Range String to Number.");
        return false;
    } catch (const std::runtime_error& e) {
        ESP_LOGW(TAG, "Catch Exception. Runtime Exception message:%s", e.what());
        return false;
    } catch(...) {
        ESP_LOGW(TAG, "An error occurred.");
        return false;
    }

    return false;
}


bool WateringSetting::ParseSimple(cJSON* pJsonRoot) noexcept(false)
{
    ESP_LOGI(TAG, "Parse SimpleSetting");

    // Initialize
    m_WateringSec = 0;
    m_WateringHourList.clear();

    // Get Watering Sec
    const cJSON *const pJsonWateringSec = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "watering_sec");
    if (!cJSON_IsNumber(pJsonWateringSec)) {
        throw std::runtime_error("Illegal object type watering_sec.");
    }
    m_WateringSec = pJsonWateringSec->valueint;

    // Get WateringHour
    const cJSON *const pJsonWateringHourList = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "watering_hour");
    if (!cJSON_IsArray(pJsonWateringHourList)) {
        throw std::runtime_error("Illegal object type WateringHourList.");
    }
    const cJSON* pJsonWateringHour = nullptr;
    cJSON_ArrayForEach(pJsonWateringHour, pJsonWateringHourList) {
        if (!cJSON_IsNumber(pJsonWateringHour)) {
            throw std::runtime_error("Illegal object type weatherArea.");
        }
        m_WateringHourList.push_back(pJsonWateringHour->valueint);
    }

    m_IsActive = true;
    return true;
}

bool WateringSetting::ParseAdvance(cJSON* pJsonRoot) noexcept(false)
{
    ESP_LOGI(TAG, "Parse AdvanceSetting");

    // Initialize
    m_WateringSec = 0;
    m_JMAAreaPathCode = 0;
    m_JMALocalCode = 0;
    m_JMAAMeDAS = 0;
    m_WateringTypeDict.clear();
    m_TemperatureWateringList.clear();
    m_MonthToTypeDict.clear();

    // Get Watering Sec
    const cJSON *const pJsonWateringSec = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "watering_sec");
    if (!cJSON_IsNumber(pJsonWateringSec)) {
        throw std::runtime_error("Illegal object type watering_sec.");
    }
    m_WateringSec = pJsonWateringSec->valueint;

    {
        // Get Weather Forecast
        const cJSON *const pJsonWeatherForecast = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "wether_forecast");
        if (!cJSON_IsObject(pJsonWeatherForecast)) {
            throw std::runtime_error("Illegal object type weather_forecast.");
        }

        // Service
        const cJSON *const pJsonService = cJSON_GetObjectItemCaseSensitive(pJsonWeatherForecast, "service");
        if (!cJSON_IsString(pJsonService)) {
            throw std::runtime_error("Illegal object type weaarherAreaCode.");
        }
        const std::string serviceStr = pJsonService->valuestring;
        if (serviceStr != "jma") {
            throw std::runtime_error("Invalid Service.");
        }

        // AreaPathCode(jma)
        const cJSON *const pJsonAreaPathCode = cJSON_GetObjectItemCaseSensitive(pJsonWeatherForecast, "area_path_code");
        if (!cJSON_IsNumber(pJsonAreaPathCode)) {
            throw std::runtime_error("Illegal object type area_path_code.");
        }
        m_JMAAreaPathCode = pJsonAreaPathCode->valueint;

        // LocalCode(jma)
        const cJSON *const pJsonLocalCode = cJSON_GetObjectItemCaseSensitive(pJsonWeatherForecast, "local_code");
        if (!cJSON_IsNumber(pJsonLocalCode)) {
            throw std::runtime_error("Illegal object type local_code.");
        }
        m_JMALocalCode = pJsonLocalCode->valueint;

        // AMeDASObservationPointNumber(jma)
        const cJSON *const pJsonAMeDASObservationPointNumber = cJSON_GetObjectItemCaseSensitive(pJsonWeatherForecast, "amedas_observation_point_number");
        if (!cJSON_IsNumber(pJsonAMeDASObservationPointNumber)) {
            throw std::runtime_error("Illegal object type amedas_observation_point_number.");
        }
        m_JMAAMeDAS = pJsonAMeDASObservationPointNumber->valueint;
    }
    {
        // Get Watering Type
        const cJSON *const pJsonWateringTypeList = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "watering_type");
        if (!cJSON_IsArray(pJsonWateringTypeList)) {
            throw std::runtime_error("Illegal object type watering_type.");
        }

        const cJSON* pJsonWateringType = nullptr;
        cJSON_ArrayForEach(pJsonWateringType, pJsonWateringTypeList) {
            WateringType wateringType;

            // Watering type name
            const cJSON *const pJsonType = cJSON_GetObjectItemCaseSensitive(pJsonWateringType, "type");
            if (!cJSON_IsString(pJsonType)) {
                throw std::runtime_error("Illegal object type \"type\".");
            }
            wateringType.WateringType = pJsonType->valuestring;
            
            // Day Span
            const cJSON *const pJsonDaySpan = cJSON_GetObjectItemCaseSensitive(pJsonWateringType, "day_span");
            if (!cJSON_IsNumber(pJsonDaySpan)) {
                throw std::runtime_error("Illegal object type day_span.");
            }
            wateringType.DaySpan = pJsonDaySpan->valueint;
            
            // WateringHour
            std::vector<std::int32_t> wateringHours;
            const cJSON *const pJsonWateringHourList = cJSON_GetObjectItemCaseSensitive(pJsonWateringType, "watering_hour");
            if (!cJSON_IsArray(pJsonWateringHourList)) {
                throw std::runtime_error("Illegal object type WateringHourList.");
            }
            const cJSON* pJsonWateringHour = nullptr;
            cJSON_ArrayForEach(pJsonWateringHour, pJsonWateringHourList) {
                if (!cJSON_IsNumber(pJsonWateringHour)) {
                    throw std::runtime_error("Illegal object type weatherArea.");
                }
                wateringType.WateringHours.push_back(pJsonWateringHour->valueint);
            }

            m_WateringTypeDict.insert(std::make_pair(wateringType.WateringType, wateringType));
        }
    }
    {
        // Temperature Watering 
        const cJSON *const pJsonTemperatureWateringList = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "temperature_watering");
        if (!cJSON_IsArray(pJsonTemperatureWateringList)) {
            throw std::runtime_error("Illegal object type watering_type.");
        }

        const cJSON* pJsonTemparatureWatering = nullptr;
        cJSON_ArrayForEach(pJsonTemparatureWatering, pJsonTemperatureWateringList) {
            TemperatureWatering temperatureWatering;

            // UnderTemp
            const cJSON *const pJsonTemparature = cJSON_GetObjectItemCaseSensitive(pJsonTemparatureWatering, "temperature");
            if (!cJSON_IsNumber(pJsonTemparature)) {
                throw std::runtime_error("Illegal object type temperature.");
            }
            temperatureWatering.Temperature = pJsonTemparature->valueint;
            
            // NormalType
            const cJSON *const pJsonNormalType = cJSON_GetObjectItemCaseSensitive(pJsonTemparatureWatering, "normal_type");
            if (!cJSON_IsString(pJsonNormalType)) {
                throw std::runtime_error("Illegal object type normal_type.");
            }
            temperatureWatering.NormalType = pJsonNormalType->valuestring;

            // RainType
            const cJSON *const pJsonRainType = cJSON_GetObjectItemCaseSensitive(pJsonTemparatureWatering, "rain_type");
            if (!cJSON_IsString(pJsonRainType)) {
                throw std::runtime_error("Illegal object type rain_type.");
            }
            temperatureWatering.RainType = pJsonRainType->valuestring;

            m_TemperatureWateringList.push_back(temperatureWatering);
        }
        //Sort
        std::sort(m_TemperatureWateringList.begin(), m_TemperatureWateringList.end(), 
            [](const TemperatureWatering& left, const TemperatureWatering& right) {
                return left.Temperature < right.Temperature;
            }
        );
    }
    {
        // Month To Type (could not get Wether forecast)   
        const cJSON *const pJsonMonthToTypeList = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "month_to_type");
        if (!cJSON_IsArray(pJsonMonthToTypeList)) {
            throw std::runtime_error("Illegal object type month_to_type.");
        }

        const cJSON* pJsonMonthToTypeIter = nullptr;
        cJSON_ArrayForEach(pJsonMonthToTypeIter, pJsonMonthToTypeList) {
            if (!cJSON_IsObject(pJsonMonthToTypeIter)) {
                throw std::runtime_error("Illegal object type month_to_type.");
            }
            const cJSON* pJsonMonthToType = pJsonMonthToTypeIter->child;
            if (!pJsonMonthToType || !pJsonMonthToType->string) {
                throw std::runtime_error("Illegal object type month_to_type.");
            }
            m_MonthToTypeDict.insert(std::make_pair(pJsonMonthToType->string, pJsonMonthToType->valuestring));
        }
    }
    {
        // Valve Power Control
        const cJSON *const pJsonValvePowerControl = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "valve_power_control");
        if (!cJSON_IsObject(pJsonValvePowerControl)) {
            throw std::runtime_error("Illegal object type valve_power_control.");
        }

        // Base Voltage
        const cJSON *const pJsonBaseVoltage = cJSON_GetObjectItemCaseSensitive(pJsonValvePowerControl, "base_voltage");
        if (!cJSON_IsNumber(pJsonBaseVoltage)) {
            throw std::runtime_error("Illegal object type base_voltage.");
        }
        m_BaseVoltage = pJsonBaseVoltage->valuedouble;

        // BaseRate
        const cJSON *const pJsonBaseRate = cJSON_GetObjectItemCaseSensitive(pJsonValvePowerControl, "base_rate");
        if (!cJSON_IsNumber(pJsonBaseRate)) {
            throw std::runtime_error("Illegal object type base_rate.");
        }
        m_BaseRate = pJsonBaseRate->valuedouble;

        // Voltage Rate
        const cJSON *const pJsonVoltageRate = cJSON_GetObjectItemCaseSensitive(pJsonValvePowerControl, "voltage_rate");
        if (!cJSON_IsNumber(pJsonVoltageRate)) {
            throw std::runtime_error("Illegal object type voltage_rate.");
        }
        m_VoltageRate = pJsonVoltageRate->valuedouble;
    }

    m_IsActive = true;
    return true;
}



bool WateringSetting::Save(const std::string& body)
{
    ESP_LOGV(TAG, "SAVE");
    return FileSystem::Write(WateringSetting::SETTING_FILE_NAME, body);
}

bool WateringSetting::Load(std::string& body)
{
    body.clear();
    const bool isReadOk = FileSystem::Read(WateringSetting::SETTING_FILE_NAME, body);
    if (!isReadOk) {
        return false;
    }
    return true;
}

bool WateringSetting::Delete()
{
    return FileSystem::Delete(WateringSetting::SETTING_FILE_NAME);
}


} // IrrigationSystem

// EOF
