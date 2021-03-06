// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "weather_forecast.h"

#include <cJSON.h>

#include <stdexcept>
#include <unordered_map>
#include <sstream>

#include "logger.h"
#include "http_request.h"

// Request Server Root Cert (PEM)
extern const uint8_t CERT_DigiCertGlobalRootCA_PEM[] asm("_binary_DigiCertGlobalRootCA_crt_pem_start");

namespace IrrigationSystem {

WeatherForecast::WeatherForecast()
    :m_RequestStatus(NOT_REQUEST)
    ,m_CurrentWeatherCode(0)
    ,m_CurrentMaxTemperature(0)
    ,m_JMAAreaPathCode(0)
    ,m_JMAAreaForecastLocalCode(0)
    ,m_JMAAMeDASObservationPointNumber(0)
{}

void WeatherForecast::Initialize()
{
    *this = WeatherForecast();
}

/// Set JMA Parameter
void WeatherForecast::SetJMAParamter(const std::int32_t areaPathCode, const std::int32_t localCode, const std::int32_t AMeDASPoint) 
{
    m_JMAAreaPathCode = areaPathCode;
    m_JMAAreaForecastLocalCode = localCode;
    m_JMAAMeDASObservationPointNumber = AMeDASPoint;
}

/// Obtaining weather forecast information via the JMA API
void WeatherForecast::Request()
{
    // Weather Forecast API by JMA
    std::stringstream requestUrl;
    requestUrl << "https://www.jma.go.jp/bosai/forecast/data/forecast/" 
               << m_JMAAreaPathCode << ".json";

    HttpRequest httpRequest;
    httpRequest.EnableTLS(reinterpret_cast<const char*>(CERT_DigiCertGlobalRootCA_PEM));
    httpRequest.Request(requestUrl.str());
    if (httpRequest.GetStatus() == HttpRequest::STATUS_OK) {
        Parse(httpRequest.GetResponseBody());
        return;
    }
    ESP_LOGW(TAG, "Request NG");
}

WeatherForecast::RequestStatus WeatherForecast::GetRequestStatus() const
{
    return m_RequestStatus;
}

int WeatherForecast::GetCurrentWeatherCode() const
{
    return m_CurrentWeatherCode;
}

int WeatherForecast::GetCurrentMaxTemperature() const
{
    return m_CurrentMaxTemperature;
}

bool WeatherForecast::IsRain() const
{
    static constexpr int WEATHER_TOP_CATEGORY_RAIN = 3;
    static constexpr int WEATHER_TOP_CATEGORY_SNOW = 4;
    static constexpr int WEATHER_TOP_CATEGORY_DIGITS = 100;
    const int weatherCodeTopCategory = (m_CurrentWeatherCode / WEATHER_TOP_CATEGORY_DIGITS);
    return (weatherCodeTopCategory == WEATHER_TOP_CATEGORY_RAIN ||
            weatherCodeTopCategory == WEATHER_TOP_CATEGORY_SNOW);
}

void WeatherForecast::Parse(const std::string& jsonStr)
{
    m_RequestStatus = FAILED;
    cJSON* pJsonRoot = nullptr;

    try {
        pJsonRoot = cJSON_Parse(jsonStr.c_str());
        if (!pJsonRoot){
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr)
            {
                throw std::runtime_error(error_ptr);
            }
            throw std::runtime_error("Json Parse Error.");
        }

        // JMA API Parse
        if (!cJSON_IsArray(pJsonRoot)) { 
            throw std::runtime_error("Illegal object type root.");
        }
        const int rootArraySize = cJSON_GetArraySize(pJsonRoot); 
        if (rootArraySize != 2) {
            throw std::runtime_error("Invalid Array Size root");
        }

        // Forecast
        static constexpr int JSON_FORECAST_NEAR_IDX = 0;
        //static constexpr int JSON_FORECAST_WEEK_IDX = 1;
        const cJSON *const pJsonForecast = cJSON_GetArrayItem(pJsonRoot, JSON_FORECAST_NEAR_IDX);
        if (!cJSON_IsObject(pJsonForecast)) {
            throw std::runtime_error("Illegal object type forecast.");
        }

        // timeSeries
        const cJSON *const pJsonTimeSeries = cJSON_GetObjectItemCaseSensitive(pJsonForecast, "timeSeries");
        if (!cJSON_IsArray(pJsonTimeSeries)) {
            throw std::runtime_error("Illegal object type timeSeries.");
        }
        static const int TIME_SERIES_WEATHER_NEWAR_LENGTH = 3;
        const int timeSeriesArraySize = cJSON_GetArraySize(pJsonTimeSeries); 
        if (timeSeriesArraySize != TIME_SERIES_WEATHER_NEWAR_LENGTH) {
            throw std::runtime_error("Invalid Array Size timeSeries");
        }
        
        {
            // timeSeriesWeather
            static constexpr int TIME_SERIES_WEATHER_INDEX = 0;
            const cJSON *const pJsonTimeSeriesWeather = cJSON_GetArrayItem(pJsonTimeSeries, TIME_SERIES_WEATHER_INDEX);
            if (!cJSON_IsObject(pJsonTimeSeriesWeather)) {
                throw std::runtime_error("Illegal object type timeSeriesWeather.");
            }

            // timeDefines
            const cJSON *const pJsonTimeDefinesList = cJSON_GetObjectItemCaseSensitive(pJsonTimeSeriesWeather, "timeDefines");
            if (!cJSON_IsArray(pJsonTimeDefinesList)) {
                throw std::runtime_error("Illegal object type timeDefinesList.");
            }
            const int timeDefinesArraySize = cJSON_GetArraySize(pJsonTimeDefinesList); 

            // weatherArea
            const cJSON *const pJsonWeatherAreaList = cJSON_GetObjectItemCaseSensitive(pJsonTimeSeriesWeather, "areas");
            if (!cJSON_IsArray(pJsonWeatherAreaList)) {
                throw std::runtime_error("Illegal object type waatherAreaList.");
            }
            const cJSON* pJsonWeatherAreas = nullptr;
            cJSON_ArrayForEach(pJsonWeatherAreas, pJsonWeatherAreaList) {
                const cJSON *const pJsonArea = cJSON_GetObjectItemCaseSensitive(pJsonWeatherAreas, "area");
                if (!cJSON_IsObject(pJsonArea)) {
                    throw std::runtime_error("Illegal object type weatherArea.");
                }

                const cJSON *const pJsonCode = cJSON_GetObjectItemCaseSensitive(pJsonArea, "code");
                if (!cJSON_IsString(pJsonCode)) {
                    throw std::runtime_error("Illegal object type weaarherAreaCode.");
                }

                const int code = std::stoi(pJsonCode->valuestring);
                if (code == m_JMAAreaForecastLocalCode) {
                    const cJSON *const pJsonWeatherCodeList = cJSON_GetObjectItemCaseSensitive(pJsonWeatherAreas, "weatherCodes");
                    if (!cJSON_IsArray(pJsonWeatherCodeList)) {
                        throw std::runtime_error("Illegal object type WeatherCodeList.");
                    }

                    const int weatherCodeArraySize = cJSON_GetArraySize(pJsonWeatherCodeList); 
                    if (timeDefinesArraySize != weatherCodeArraySize) {
                        throw std::runtime_error("Invalid Array Size weatherCodeList");
                    }
                    
                    static constexpr int FORECAST_TOMMORROW_INDEX = 1; // The next day (the forecast comes out at 09:00, so it corresponds to the day's forecast)
                    const cJSON *const pJsonWeatherCode = cJSON_GetArrayItem(pJsonWeatherCodeList, FORECAST_TOMMORROW_INDEX);
                    if (!cJSON_IsString(pJsonWeatherCode)) {
                        throw std::runtime_error("Illegal object type TimeSeriesWeather.");
                    }
                    
                    m_CurrentWeatherCode = std::stoi(pJsonWeatherCode->valuestring);

                    break;
                }
            }
        }

        {
            // timeSeriesTemperature
            static constexpr int TIME_SERIES_TEMPERATURE_INDEX = 2;
            const cJSON *const pJsonTimeSeriesTemperature = cJSON_GetArrayItem(pJsonTimeSeries, TIME_SERIES_TEMPERATURE_INDEX);
            if (!cJSON_IsObject(pJsonTimeSeriesTemperature)) {
                throw std::runtime_error("Illegal object type timeSeriesTemperature.");
            }

            // temperatureArea
            const cJSON *const pJsonTemperatureAreaList = cJSON_GetObjectItemCaseSensitive(pJsonTimeSeriesTemperature, "areas");
            if (!cJSON_IsArray(pJsonTemperatureAreaList)) {
                throw std::runtime_error("Illegal object type temperatureAreaList.");
            }
            const cJSON* pJsonTemperatureAreas = nullptr;
            cJSON_ArrayForEach(pJsonTemperatureAreas, pJsonTemperatureAreaList) {
                const cJSON *const pJsonArea = cJSON_GetObjectItemCaseSensitive(pJsonTemperatureAreas, "area");
                if (!cJSON_IsObject(pJsonArea)) {
                    throw std::runtime_error("Illegal object type weatherArea.");
                }

                const cJSON *const pJsonCode = cJSON_GetObjectItemCaseSensitive(pJsonArea, "code");
                if (!cJSON_IsString(pJsonCode)) {
                    throw std::runtime_error("Illegal object type weaarherAreaCode.");
                }

                const int code = std::stoi(pJsonCode->valuestring);
                if (code == m_JMAAMeDASObservationPointNumber) {
                    const cJSON *const pJsonTemperatureList = cJSON_GetObjectItemCaseSensitive(pJsonTemperatureAreas, "temps");
                    if (!cJSON_IsArray(pJsonTemperatureList)) {
                        throw std::runtime_error("Illegal object type WeatherCodeList.");
                    }

                    const int temperatureLength = cJSON_GetArraySize(pJsonTemperatureList); 
                    if (temperatureLength != 2) {
                        throw std::runtime_error("Invalid Array Size temperatureList");
                    }
                    
                    //static constexpr int TEMPERATURE_MIN_INDEX = 0;
                    static constexpr int TEMPERATURE_MAX_INDEX = 1;
                    const cJSON *const pJsonTemperature = cJSON_GetArrayItem(pJsonTemperatureList, TEMPERATURE_MAX_INDEX);
                    if (!cJSON_IsString(pJsonTemperature)) {
                        throw std::runtime_error("Illegal object type temperature.");
                    }
                    
                    m_CurrentMaxTemperature = std::stoi(pJsonTemperature->valuestring);
                    break;
                }
            }
        }

        m_RequestStatus = ACQUIRED;
        ESP_LOGD(TAG, "WeatherForecast Parse OK.");

    } catch (const std::invalid_argument& e) {
        ESP_LOGW(TAG, "Catch Exception. Invalid Argument String to Number.");
    } catch (const std::out_of_range& e) {
        ESP_LOGW(TAG, "Catch Exception. Out Of Range String to Number.");
    } catch (const std::runtime_error& e) {
        ESP_LOGW(TAG, "Catch Exception. Runtime Exception message:%s", e.what());
    } catch(...) {
        ESP_LOGW(TAG, "An error occurred.");
    }

    cJSON_Delete(pJsonRoot);
}


const char* WeatherForecast::WeatherCodeToStr(const int weatherCode)
{
    static const std::unordered_map<int, std::string> WEATHER_CODE_TO_STR_MAP = {
        {100, "CLEAR"}, {101, "PARTLY CLOUDY"}, {102, "CLEAR, OCCASIONAL SCATTERED SHOWERS"}, {103, "CLEAR, FREQUENT SCATTERED SHOWERS"}, {104, "CLEAR, SNOW FLURRIES"}, {105, "CLEAR, FREQUENT SNOW FLURRIES"}, {106, "CLEAR, OCCASIONAL SCATTERED SHOWERS OR SNOW FLURRIES"}, {107, "CLEAR, FREQUENT SCATTERED SHOWERS OR SNOW FLURRIES"}, {108, "CLEAR, OCCASIONAL SCATTERED SHOWERS AND/OR THUNDER"}, {110, "CLEAR, PARTLY CLOUDY LATER"}, {111, "CLEAR, CLOUDY LATER"}, {112, "CLEAR, OCCASIONAL SCATTERED SHOWERS LATER"}, {113, "CLEAR, FREQUENT SCATTERED SHOWERS LATER"}, {114, "CLEAR,RAIN LATER"}, {115, "CLEAR, OCCASIONAL SNOW FLURRIES LATER"}, {116, "CLEAR, FREQUENT SNOW FLURRIES LATER"}, {117, "CLEAR,SNOW LATER"}, {118, "CLEAR, RAIN OR SNOW LATER"}, {119, "CLEAR, RAIN AND/OR THUNDER LATER"}, {120, "OCCASIONAL SCATTERED SHOWERS IN THE MORNING AND EVENING, CLEAR DURING THE DAY"}, {121, "OCCASIONAL SCATTERED SHOWERS IN THE MORNING, CLEAR DURING THE DAY"}, {122, "CLEAR, OCCASIONAL SCATTERED SHOWERS IN THE EVENING"}, {123, "CLEAR IN THE PLAINS, RAIN AND THUNDER NEAR MOUTAINOUS AREAS"}, {124, "CLEAR IN THE PLAINS, SNOW NEAR MOUTAINOUS AREAS"}, {125, "CLEAR, RAIN AND THUNDER IN THE AFTERNOON"}, {126, "CLEAR, RAIN IN THE AFTERNOON"}, {127, "CLEAR, RAIN IN THE EVENING"}, {128, "CLEAR, RAIN IN THE NIGHT"}, {130, "FOG IN THE MORNING, CLEAR LATER"}, {131, "FOG AROUND DAWN, CLEAR LATER"}, {132, "CLOUDY IN THE MORNING AND EVENING, CLEAR DURING THE DAY"}, {140, "CLEAR, FREQUENT SCATTERED SHOWERS AND THUNDER"}, {160, "CLEAR, SNOW FLURRIES OR OCCASIONAL SCATTERED SHOWERS"}, {170, "CLEAR, FREQUENT SNOW FLURRIES OR SCATTERED SHOWERS"}, {181, "CLEAR, SNOW OR RAIN LATER"},
        {200, "CLOUDY"}, {201, "MOSTLY CLOUDY"}, {202, "CLOUDY, OCCASIONAL SCATTERED SHOWERS"}, {203, "CLOUDY, FREQUENT SCATTERED SHOWERS"}, {204, "CLOUDY, OCCASIONAL SNOW FLURRIES"}, {205, "CLOUDY FREQUENT SNOW FLURRIES"}, {206, "CLOUDY, OCCASIONAL SCATTERED SHOWERS OR SNOW FLURRIES"}, {207, "CLOUDY, FREQUENT SCCATERED SHOWERS OR SNOW FLURRIES"}, {208, "CLOUDY, OCCASIONAL SCATTERED SHOWERS AND/OR THUNDER"}, {209, "FOG"}, {210, "CLOUDY, PARTLY CLOUDY LATER"}, {211, "CLOUDY, CLEAR LATER"}, {212, "CLOUDY, OCCASIONAL SCATTERED SHOWERS LATER"}, {213, "CLOUDY, FREQUENT SCATTERED SHOWERS LATER"}, {214, "CLOUDY, RAIN LATER"}, {215, "CLOUDY, SNOW FLURRIES LATER"}, {216, "CLOUDY, FREQUENT SNOW FLURRIES LATER"}, {217, "CLOUDY, SNOW LATER"}, {218, "CLOUDY, RAIN OR SNOW LATER"}, {219, "CLOUDY, RAIN AND/OR THUNDER LATER"}, {220, "OCCASIONAL SCCATERED SHOWERS IN THE MORNING AND EVENING, CLOUDY DURING THE DAY"}, {221, "CLOUDY OCCASIONAL SCCATERED SHOWERS IN THE MORNING"}, {222, "CLOUDY, OCCASIONAL SCCATERED SHOWERS IN THE EVENING"}, {223, "CLOUDY IN THE MORNING AND EVENING, PARTLY CLOUDY DURING THE DAY,"}, {224, "CLOUDY, RAIN IN THE AFTERNOON"}, {225, "CLOUDY, RAIN IN THE EVENING"}, {226, "CLOUDY, RAIN IN THE NIGHT"}, {228, "CLOUDY, SNOW IN THE AFTERNOON"}, {229, "CLOUDY, SNOW IN THE EVENING"}, {230, "CLOUDY, SNOW IN THE NIGHT"}, {231, "CLOUDY, FOG OR DRIZZLING ON THE SEA AND NEAR SEASHORE"}, {240, "CLOUDY, FREQUENT SCCATERED SHOWERS AND THUNDER"}, {250, "CLOUDY, FREQUENT SNOW AND THUNDER"}, {260, "CLOUDY, SNOW FLURRIES OR OCCASIONAL SCATTERED SHOWERS"}, {270, "CLOUDY, FREQUENT SNOW FLURRIES OR SCATTERED SHOWERS"}, {281, "CLOUDY, SNOW OR RAIN LATER"},
        {300, "RAIN"}, {301, "RAIN, PARTLY CLOUDY"}, {302, "SHOWERS THROUGHOUT THE DAY"}, {303, "RAIN,FREQUENT SNOW FLURRIES"}, {304, "RAINORSNOW"}, {306, "HEAVYRAIN"}, {308, "RAINSTORM"}, {309, "RAIN,OCCASIONAL SNOW"}, {311, "RAIN,CLEAR LATER"}, {313, "RAIN,CLOUDY LATER"}, {314, "RAIN, FREQUENT SNOW FLURRIES LATER"}, {315, "RAIN,SNOW LATER"}, {316, "RAIN OR SNOW, CLEAR LATER"}, {317, "RAIN OR SNOW, CLOUDY LATER"}, {320, "RAIN IN THE MORNING, CLEAR LATER"}, {321, "RAIN IN THE MORNING, CLOUDY LATER"}, {322, "OCCASIONAL SNOW IN THE MORNING AND EVENING, RAIN DURING THE DAY"}, {323, "RAIN, CLEAR IN THE AFTERNOON"}, {324, "RAIN, CLEAR IN THE EVENING"}, {325, "RAIN, CLEAR IN THE NIGHT"}, {326, "RAIN, SNOW IN THE EVENING"}, {327, "RAIN,SNOW IN THE NIGHT"}, {328, "RAIN, EXPECT OCCASIONAL HEAVY RAINFALL"}, {329, "RAIN, OCCASIONAL SLEET"}, {340, "SNOWORRAIN"}, {350, "RAIN AND THUNDER"}, {361, "SNOW OR RAIN, CLEAR LATER"}, {371, "SNOW OR RAIN, CLOUDY LATER"},
        {400, "SNOW"}, {401, "SNOW, FREQUENT CLEAR"}, {402, "SNOWTHROUGHOUT THE DAY"}, {403, "SNOW,FREQUENT SCCATERED SHOWERS"}, {405, "HEAVYSNOW"}, {406, "SNOWSTORM"}, {407, "HEAVYSNOWSTORM"}, {409, "SNOW, OCCASIONAL SCCATERED SHOWERS"}, {411, "SNOW,CLEAR LATER"}, {413, "SNOW,CLOUDY LATER"}, {414, "SNOW,RAIN LATER"}, {420, "SNOW IN THE MORNING, CLEAR LATER"}, {421, "SNOW IN THE MORNING, CLOUDY LATER"}, {422, "SNOW, RAIN IN THE AFTERNOON"}, {423, "SNOW, RAIN IN THE EVENING"}, {425, "SNOW, EXPECT OCCASIONAL HEAVY SNOWFALL"}, {426, "SNOW, SLEET LATER"}, {427, "SNOW, OCCASIONAL SLEET"}, {450, "SNOW AND THUNDER"},
    };
/*
    // JP
    static const std::unordered_map<int, std::string> WEATHER_CODE_TO_STR_MAP = {
        {100, "???"}, {101, "????????????"}, {102, "????????????"}, {103, "????????????"}, {104, "????????????"}, {105, "????????????"}, {106, "??????????????????"}, {107, "??????????????????"}, {108, "?????????????????????"}, {110, "???????????????"}, {111, "?????????"}, {112, "???????????????"}, {113, "???????????????"}, {114, "?????????"}, {115, "???????????????"}, {116, "???????????????"}, {117, "?????????"}, {118, "???????????????"}, {119, "??????????????????"}, {120, "??????????????????"}, {121, "?????????????????????"}, {122, "??????????????????"}, {123, "??????????????????"}, {124, "???????????????"}, {125, "??????????????????"}, {126, "??????????????????"}, {127, "??????????????????"}, {128, "????????????"}, {130, "??????????????????"}, {131, "???????????????"}, {132, "????????????"}, {140, "???????????????????????????"}, {160, "??????????????????"}, {170, "??????????????????"}, {181, "???????????????"},
        {200, "???"}, {201, "????????????"}, {202, "????????????"}, {203, "????????????"}, {204, "????????????"}, {205, "????????????"}, {206, "??????????????????"}, {207, "??????????????????"}, {208, "?????????????????????"}, {209, "???"}, {210, "???????????????"}, {211, "?????????"}, {212, "???????????????"}, {213, "???????????????"}, {214, "?????????"}, {215, "???????????????"}, {216, "???????????????"}, {217, "?????????"}, {218, "???????????????"}, {219, "??????????????????"}, {220, "??????????????????"}, {221, "?????????????????????"}, {222, "??????????????????"}, {223, "??????????????????"}, {224, "??????????????????"}, {225, "??????????????????"}, {226, "????????????"}, {228, "??????????????????"}, {229, "??????????????????"}, {230, "????????????"}, {231, "??????????????????????????????"}, {240, "???????????????????????????"}, {250, "???????????????????????????"}, {260, "??????????????????"}, {270, "??????????????????"}, {281, "???????????????"},
        {300, "???"}, {301, "????????????"}, {302, "???????????????"}, {303, "????????????"}, {304, "?????????"}, {306, "??????"}, {308, "?????????????????????"}, {309, "????????????"}, {311, "?????????"}, {313, "?????????"}, {314, "???????????????"}, {315, "?????????"}, {316, "???????????????"}, {317, "???????????????"}, {320, "??????????????????"}, {321, "??????????????????"}, {322, "??????????????????"}, {323, "??????????????????"}, {324, "??????????????????"}, {325, "????????????"}, {326, "??????????????????"}, {327, "????????????"}, {328, "?????????????????????"}, {329, "??????????????????"}, {340, "?????????"}, {350, "??????????????????"}, {361, "???????????????"}, {371, "???????????????"},
        {400, "???"}, {401, "????????????"}, {402, "???????????????"}, {403, "????????????"}, {405, "??????"}, {406, "????????????"}, {407, "?????????"}, {409, "????????????"}, {411, "?????????"}, {413, "?????????"}, {414, "?????????"}, {420, "??????????????????"}, {421, "??????????????????"}, {422, "??????????????????"}, {423, "??????????????????"},{425, "?????????????????????"}, {426, "???????????????"}, {427, "??????????????????"}, {450, "??????????????????"}
    };
*/

    static constexpr char EMPTY[] = "";
    std::unordered_map<int, std::string>::const_iterator iter = WEATHER_CODE_TO_STR_MAP.find(weatherCode);
    if (iter == WEATHER_CODE_TO_STR_MAP.end()) {
        return EMPTY;;
    }
    return iter->second.c_str();
}


} // IrrigationSystem

// EOF
