#ifndef WEATHER_FORECAST_H_
#define WEATHER_FORECAST_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include <cstdint>
#include <string>

namespace IrrigationSystem {

class WeatherForecast final {
 public:
  enum RequestStatus {
    NOT_REQUEST,
    ACQUIRED,
    FAILED,
  };

 public:
  WeatherForecast();

  void Initialize();

  void SetJMAParamter(const std::int32_t area_path_code,
                      const std::int32_t local_code,
                      const std::int32_t amedas_point);

  /// Obtaining weather forecast information via the JMA API
  void Request();

  RequestStatus GetRequestStatus() const;
  int GetCurrentWeatherCode() const;
  int GetCurrentMaxTemperature() const;
  bool IsRain() const;

 private:
  void Parse(const std::string& json_str);

 public:
  static const char* WeatherCodeToStr(const int weather_code);

 private:
  RequestStatus request_status_;
  int current_weather_code_;
  int current_max_temperature_;

  /// Area path code for weather forecast determination. Tokyo:130010
  /// http://www.jma.go.jp/bosai/common/const/area.json
  std::int32_t jma_area_path_code_;
  /// Area number for weather forecast determination. Tokyo:130010 気象庁
  /// 気象警報・注意報等に用いる府県予報区、一次細分区域等のコード
  /// https://www.data.go.jp/data/dataset/mlit_20140919_0758/resource/a2081b13-5b3d-4ac1-9e4a-0a69aa7af0ef
  std::int32_t jma_area_forecast_local_code_;
  /// AMeDAS observation point number for weather forecast determination.
  /// Tokyo:44132
  std::int32_t jma_amedas_observation_point_number_;
};

}  // namespace IrrigationSystem

#endif  // WEATHER_FORECAST_H_
// EOF
