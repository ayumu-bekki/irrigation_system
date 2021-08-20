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
    WeatherForecast();

    bool Request();

    bool IsGetSuccess() const;
    int GetCurrentWeatherCode() const;
    int GetCurrentMaxTemperature() const;
    bool IsRain() const;

private:
    bool Parse(const std::string& jsonStr);

public:
    static const char* WeatherCodeToStr(const int weatherCode);

private:
    bool m_IsGetSuccess;
    int m_CurrentWeatherCode;
    int m_CurrentMaxTemperature;
};

} // IrrigationSystem

#endif // WEATHER_FORECAST_H_
// EOF
