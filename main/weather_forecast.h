#ifndef WEATHER_FORECAST_H_
#define WEATHER_FORECAST_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <string>
#include <cstdint>

namespace IrrigationSystem {

class WeatherForecast final
{
public:
    enum RequestStatus {
        NOT_REQUEST,
        ACQUIRED,
        FAILED,
    };

public:
    WeatherForecast();

    void Initialize();

    void SetJMAParamter(const std::int32_t areaPathCode, const std::int32_t localCode, const std::int32_t AMeDASPoint);

    /// Obtaining weather forecast information via the JMA API
    void Request();

    RequestStatus GetRequestStatus() const;
    int GetCurrentWeatherCode() const;
    int GetCurrentMaxTemperature() const;
    bool IsRain() const;

private:
    void Parse(const std::string& jsonStr);

public:
    static const char* WeatherCodeToStr(const int weatherCode);

private:
    RequestStatus m_RequestStatus;
    int m_CurrentWeatherCode;
    int m_CurrentMaxTemperature;

    /// Area path code for weather forecast determination. Tokyo:130010 http://www.jma.go.jp/bosai/common/const/area.json
    std::int32_t m_JMAAreaPathCode;
    /// Area number for weather forecast determination. Tokyo:130010 気象庁 気象警報・注意報等に用いる府県予報区、一次細分区域等のコード https://www.data.go.jp/data/dataset/mlit_20140919_0758/resource/a2081b13-5b3d-4ac1-9e4a-0a69aa7af0ef
    std::int32_t m_JMAAreaForecastLocalCode;
    /// AMeDAS observation point number for weather forecast determination. Tokyo:44132
    std::int32_t m_JMAAMeDASObservationPointNumber;
};

} // IrrigationSystem

#endif // WEATHER_FORECAST_H_
// EOF
