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
        {100, "晴"}, {101, "晴時々曇"}, {102, "晴一時雨"}, {103, "晴時々雨"}, {104, "晴一時雪"}, {105, "晴時々雪"}, {106, "晴一時雨か雪"}, {107, "晴時々雨か雪"}, {108, "晴一時雨か雷雨"}, {110, "晴後時々曇"}, {111, "晴後曇"}, {112, "晴後一時雨"}, {113, "晴後時々雨"}, {114, "晴後雨"}, {115, "晴後一時雪"}, {116, "晴後時々雪"}, {117, "晴後雪"}, {118, "晴後雨か雪"}, {119, "晴後雨か雷雨"}, {120, "晴朝夕一時雨"}, {121, "晴朝の内一時雨"}, {122, "晴夕方一時雨"}, {123, "晴山沿い雷雨"}, {124, "晴山沿い雪"}, {125, "晴午後は雷雨"}, {126, "晴昼頃から雨"}, {127, "晴夕方から雨"}, {128, "晴夜は雨"}, {130, "朝の内霧後晴"}, {131, "晴明け方霧"}, {132, "晴朝夕曇"}, {140, "晴時々雨で雷を伴う"}, {160, "晴一時雪か雨"}, {170, "晴時々雪か雨"}, {181, "晴後雪か雨"},
        {200, "曇"}, {201, "曇時々晴"}, {202, "曇一時雨"}, {203, "曇時々雨"}, {204, "曇一時雪"}, {205, "曇時々雪"}, {206, "曇一時雨か雪"}, {207, "曇時々雨か雪"}, {208, "曇一時雨か雷雨"}, {209, "霧"}, {210, "曇後時々晴"}, {211, "曇後晴"}, {212, "曇後一時雨"}, {213, "曇後時々雨"}, {214, "曇後雨"}, {215, "曇後一時雪"}, {216, "曇後時々雪"}, {217, "曇後雪"}, {218, "曇後雨か雪"}, {219, "曇後雨か雷雨"}, {220, "曇朝夕一時雨"}, {221, "曇朝の内一時雨"}, {222, "曇夕方一時雨"}, {223, "曇日中時々晴"}, {224, "曇昼頃から雨"}, {225, "曇夕方から雨"}, {226, "曇夜は雨"}, {228, "曇昼頃から雪"}, {229, "曇夕方から雪"}, {230, "曇夜は雪"}, {231, "曇海上海岸は霧か霧雨"}, {240, "曇時々雨で雷を伴う"}, {250, "曇時々雪で雷を伴う"}, {260, "曇一時雪か雨"}, {270, "曇時々雪か雨"}, {281, "曇後雪か雨"},
        {300, "雨"}, {301, "雨時々晴"}, {302, "雨時々止む"}, {303, "雨時々雪"}, {304, "雨か雪"}, {306, "大雨"}, {308, "雨で暴風を伴う"}, {309, "雨一時雪"}, {311, "雨後晴"}, {313, "雨後曇"}, {314, "雨後時々雪"}, {315, "雨後雪"}, {316, "雨か雪後晴"}, {317, "雨か雪後曇"}, {320, "朝の内雨後晴"}, {321, "朝の内雨後曇"}, {322, "雨朝晩一時雪"}, {323, "雨昼頃から晴"}, {324, "雨夕方から晴"}, {325, "雨夜は晴"}, {326, "雨夕方から雪"}, {327, "雨夜は雪"}, {328, "雨一時強く降る"}, {329, "雨一時みぞれ"}, {340, "雪か雨"}, {350, "雨で雷を伴う"}, {361, "雪か雨後晴"}, {371, "雪か雨後曇"},
        {400, "雪"}, {401, "雪時々晴"}, {402, "雪時々止む"}, {403, "雪時々雨"}, {405, "大雪"}, {406, "風雪強い"}, {407, "暴風雪"}, {409, "雪一時雨"}, {411, "雪後晴"}, {413, "雪後曇"}, {414, "雪後雨"}, {420, "朝の内雪後晴"}, {421, "朝の内雪後曇"}, {422, "雪昼頃から雨"}, {423, "雪夕方から雨"},{425, "雪一時強く降る"}, {426, "雪後みぞれ"}, {427, "雪一時みぞれ"}, {450, "雪で雷を伴う"}
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
