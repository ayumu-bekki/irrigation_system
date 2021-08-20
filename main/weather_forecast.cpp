// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "weather_forecast.h"

#include <cJSON.h>

#include <stdexcept>
#include <unordered_map>

#include "logger.h"
#include "http_request.h"

// Request Server Root Cert (PEM)
extern const uint8_t CERT_DigiCertGlobalRootCA_PEM[] asm("_binary_DigiCertGlobalRootCA_crt_pem_start");

namespace IrrigationSystem {

WeatherForecast::WeatherForecast()
    :m_IsGetSuccess(false)
    ,m_CurrentWeatherCode(0)
    ,m_CurrentMaxTemperature(0)
{}

/// Obtaining weather forecast information via the JMA API
bool WeatherForecast::Request()
{
    HttpRequest httpRequest;
    httpRequest.EnableTLS(reinterpret_cast<const char*>(CERT_DigiCertGlobalRootCA_PEM));
    httpRequest.Request("https://www.jma.go.jp/bosai/forecast/data/forecast/130000.json");
    if (httpRequest.GetStatus() == HttpRequest::STATUS_OK) {
        ESP_LOGI(TAG, "Request OK");
        return Parse(httpRequest.GetResponseBody());
    }
    ESP_LOGI(TAG, "Request NG");
    return false;
}


bool WeatherForecast::IsGetSuccess() const
{
    return m_IsGetSuccess;
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

bool WeatherForecast::Parse(const std::string& jsonStr)
{
    ESP_LOGI(TAG, "Deserializ JSON");
    
    m_IsGetSuccess = false;

    // Forecast JMA ID
    //  気象庁 気象警報・注意報等に用いる府県予報区、一次細分区域等のコード
    //  https://www.data.go.jp/data/dataset/mlit_20140919_0758/resource/a2081b13-5b3d-4ac1-9e4a-0a69aa7af0ef
    const int JMA_AREA_FORECAST_LOCAL_CODE = CONFIG_JMA_AREA_FORECAST_LOCAL_CODE; /// AreaInformationCity 一次細分区域等@code 
    const int JMA_AMEDAS_OBSERVATION_POINT_NUMBER = CONFIG_JMA_AMEDAS_OBSERVATION_POINT_NUMBER; /// アメダス観測所番号

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
        const cJSON *const pJsonForecast = cJSON_GetArrayItem(pJsonRoot, 0);
        if (!cJSON_IsObject(pJsonForecast)) {
            throw std::runtime_error("Illegal object type forecast.");
        }

        // timeSeries
        const cJSON *const pJsonTimeSeries = cJSON_GetObjectItemCaseSensitive(pJsonForecast, "timeSeries");
        if (!cJSON_IsArray(pJsonTimeSeries)) {
            throw std::runtime_error("Illegal object type timeSeries.");
        }
        const int timeSeriesArraySize = cJSON_GetArraySize(pJsonTimeSeries); 
        if (timeSeriesArraySize != 3) {
            throw std::runtime_error("Invalid Array Size timeSeries");
        }
        
        {
            // timeSeriesWeather
            const cJSON *const pJsonTimeSeriesWeather = cJSON_GetArrayItem(pJsonTimeSeries, 0);
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
                if (code == JMA_AREA_FORECAST_LOCAL_CODE) {
                    const cJSON *const pJsonWeatherCodeList = cJSON_GetObjectItemCaseSensitive(pJsonWeatherAreas, "weatherCodes");
                    if (!cJSON_IsArray(pJsonWeatherCodeList)) {
                        throw std::runtime_error("Illegal object type WeatherCodeList.");
                    }

                    const int weatherCodeArraySize = cJSON_GetArraySize(pJsonWeatherCodeList); 
                    if (timeDefinesArraySize != weatherCodeArraySize) {
                        throw std::runtime_error("Invalid Array Size weatherCodeList");
                    }

                    const cJSON *const pJsonWeatherCode = cJSON_GetArrayItem(pJsonWeatherCodeList, 1); // 次の日(予報が09:00に出るので、当日の予報にあたる)
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
            const cJSON *const pJsonTimeSeriesTemperature = cJSON_GetArrayItem(pJsonTimeSeries, 2);
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
                if (code == JMA_AMEDAS_OBSERVATION_POINT_NUMBER) {
                    const cJSON *const pJsonTemperatureList = cJSON_GetObjectItemCaseSensitive(pJsonTemperatureAreas, "temps");
                    if (!cJSON_IsArray(pJsonTemperatureList)) {
                        throw std::runtime_error("Illegal object type WeatherCodeList.");
                    }

                    const int temperatureLength = cJSON_GetArraySize(pJsonTemperatureList); 
                    if (temperatureLength != 2) {
                        throw std::runtime_error("Invalid Array Size temperatureList");
                    }

                    const cJSON *const pJsonTemperature = cJSON_GetArrayItem(pJsonTemperatureList, 1); // 0:min 1:max
                    if (!cJSON_IsString(pJsonTemperature)) {
                        throw std::runtime_error("Illegal object type temperature.");
                    }
                    
                    m_CurrentMaxTemperature = std::stoi(pJsonTemperature->valuestring);
                    break;
                }
            }
        }


        m_IsGetSuccess = true;
        ESP_LOGI(TAG, "Deserialize OK WeatherCode:%d Weather:%s MaxTemperature:%d ", m_CurrentWeatherCode, WeatherForecast::WeatherCodeToStr(m_CurrentWeatherCode), m_CurrentMaxTemperature);

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

    return m_IsGetSuccess;
}


const char* WeatherForecast::WeatherCodeToStr(const int weatherCode)
{
    static const std::unordered_map<int, std::string> WEATHER_CODE_TO_STR_MAP = {
        {100, "晴"}, {101, "晴時々曇"}, {102, "晴一時雨"}, {103, "晴時々雨"}, {104, "晴一時雪"}, {105, "晴時々雪"}, {106, "晴一時雨か雪"}, {107, "晴時々雨か雪"}, {108, "晴一時雨か雷雨"}, {110, "晴後時々曇"}, {111, "晴後曇"}, {112, "晴後一時雨"}, {113, "晴後時々雨"}, {114, "晴後雨"}, {115, "晴後一時雪"}, {116, "晴後時々雪"}, {117, "晴後雪"}, {118, "晴後雨か雪"}, {119, "晴後雨か雷雨"}, {120, "晴朝夕一時雨"}, {121, "晴朝の内一時雨"}, {122, "晴夕方一時雨"}, {123, "晴山沿い雷雨"}, {124, "晴山沿い雪"}, {125, "晴午後は雷雨"}, {126, "晴昼頃から雨"}, {127, "晴夕方から雨"}, {128, "晴夜は雨"}, {130, "朝の内霧後晴"}, {131, "晴明け方霧"}, {132, "晴朝夕曇"}, {140, "晴時々雨で雷を伴う"}, {160, "晴一時雪か雨"}, {170, "晴時々雪か雨"}, {181, "晴後雪か雨"},
        {200, "曇"}, {201, "曇時々晴"}, {202, "曇一時雨"}, {203, "曇時々雨"}, {204, "曇一時雪"}, {205, "曇時々雪"}, {206, "曇一時雨か雪"}, {207, "曇時々雨か雪"}, {208, "曇一時雨か雷雨"}, {209, "霧"}, {210, "曇後時々晴"}, {211, "曇後晴"}, {212, "曇後一時雨"}, {213, "曇後時々雨"}, {214, "曇後雨"}, {215, "曇後一時雪"}, {216, "曇後時々雪"}, {217, "曇後雪"}, {218, "曇後雨か雪"}, {219, "曇後雨か雷雨"}, {220, "曇朝夕一時雨"}, {221, "曇朝の内一時雨"}, {222, "曇夕方一時雨"}, {223, "曇日中時々晴"}, {224, "曇昼頃から雨"}, {225, "曇夕方から雨"}, {226, "曇夜は雨"}, {228, "曇昼頃から雪"}, {229, "曇夕方から雪"}, {230, "曇夜は雪"}, {231, "曇海上海岸は霧か霧雨"}, {240, "曇時々雨で雷を伴う"}, {250, "曇時々雪で雷を伴う"}, {260, "曇一時雪か雨"}, {270, "曇時々雪か雨"}, {281, "曇後雪か雨"},
        {300, "雨"}, {301, "雨時々晴"}, {302, "雨時々止む"}, {303, "雨時々雪"}, {304, "雨か雪"}, {306, "大雨"}, {308, "雨で暴風を伴う"}, {309, "雨一時雪"}, {311, "雨後晴"}, {313, "雨後曇"}, {314, "雨後時々雪"}, {315, "雨後雪"}, {316, "雨か雪後晴"}, {317, "雨か雪後曇"}, {320, "朝の内雨後晴"}, {321, "朝の内雨後曇"}, {322, "雨朝晩一時雪"}, {323, "雨昼頃から晴"}, {324, "雨夕方から晴"}, {325, "雨夜は晴"}, {326, "雨夕方から雪"}, {327, "雨夜は雪"}, {328, "雨一時強く降る"}, {329, "雨一時みぞれ"}, {340, "雪か雨"}, {350, "雨で雷を伴う"}, {361, "雪か雨後晴"}, {371, "雪か雨後曇"},
        {400, "雪"}, {401, "雪時々晴"}, {402, "雪時々止む"}, {403, "雪時々雨"}, {405, "大雪"}, {406, "風雪強い"}, {407, "暴風雪"}, {409, "雪一時雨"}, {411, "雪後晴"}, {413, "雪後曇"}, {414, "雪後雨"}, {420, "朝の内雪後晴"}, {421, "朝の内雪後曇"}, {422, "雪昼頃から雨"}, {423, "雪夕方から雨"},{425, "雪一時強く降る"}, {426, "雪後みぞれ"}, {427, "雪一時みぞれ"}, {450, "雪で雷を伴う"}
    };
    static constexpr char EMPTY[] = "";
    std::unordered_map<int, std::string>::const_iterator iter = WEATHER_CODE_TO_STR_MAP.find(weatherCode);
    if (iter == WEATHER_CODE_TO_STR_MAP.end()) {
        return EMPTY;;
    }
    return iter->second.c_str();
}


} // IrrigationSystem

// EOF
