#ifndef WEATHER_FORECAST_H_
#define WEATHER_FORECAST_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <string>


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

    void Request();

    void Reset();

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
};

} // IrrigationSystem

#endif // WEATHER_FORECAST_H_
// EOF
