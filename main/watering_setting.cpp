// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "watering_setting.h"

#include <cJSON.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "file_system.h"
#include "logger.h"

namespace IrrigationSystem {

namespace {
const std::string JSON_WATERING_MODE_TABLE[WateringSetting::WATERING_MODE_MAX] = {
    "",         // WATERING_MODE_NONE
    "simple",   // WATERING_MODE_SIMPLE
    "advance",  // WATERING_MODE_ADVANCE
};
}

WateringSetting::WateringSetting()
    : is_active_(false),
      watering_mode_(WATERING_MODE_NONE),
      watering_sec_(0),
      watering_hour_list_(),
      jma_area_path_code_(0),
      jma_local_code_(0),
      jmaamedas_point_num_(0),
      watering_type_dict_(),
      temperature_watering_list_(),
      month_to_type_dict_(),
      base_rate_(0.0f),
      base_voltage_(0.0f),
      voltage_rate_(0.0f) {}

bool WateringSetting::SetSettingData(const std::string &body) {
  return Parse(body);
}

bool WateringSetting::IsActive() const { return is_active_; }

WateringSetting::WateringMode WateringSetting::GetWateringMode() const {
  return watering_mode_;
}

std::int32_t WateringSetting::GetWateringSec() const { return watering_sec_; }

const std::vector<std::int32_t> &WateringSetting::GetWateringHourList() const {
  return watering_hour_list_;
}

std::int32_t WateringSetting::GetJMAAreaPathCode() const {
  return jma_area_path_code_;
}

std::int32_t WateringSetting::GetJMALocalCode() const { return jma_local_code_; }

std::int32_t WateringSetting::GetJMAAMeDAS() const { return jmaamedas_point_num_; }

const WateringSetting::WateringTypeDict &WateringSetting::GetWateringTypeDict()
    const {
  return watering_type_dict_;
}

const WateringSetting::TemperatureWateringList &
WateringSetting::GetTemperatureWateringList() const {
  return temperature_watering_list_;
}

const WateringSetting::MonthToTypeDict &WateringSetting::GetMonthToTypeDict()
    const {
  return month_to_type_dict_;
}

float WateringSetting::GetValvePowerBaseRate() const { return base_rate_; }

float WateringSetting::GetValvePowerBaseVoltage() const {
  return base_voltage_;
}

float WateringSetting::GetValvePowerVoltageRate() const {
  return voltage_rate_;
}

bool WateringSetting::Parse(const std::string &body) noexcept {
  // Initialize
  watering_mode_ = WATERING_MODE_NONE;

  cJSON *json_root = nullptr;
  try {
    json_root = cJSON_Parse(body.c_str());
    if (!json_root) {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr) {
        throw std::runtime_error(error_ptr);
      }
      throw std::runtime_error("Json Parse Error.");
    }

    // Get Watering Mode
    const cJSON *const json_watering_mode =
        cJSON_GetObjectItemCaseSensitive(json_root, "watering_mode");
    if (!cJSON_IsString(json_watering_mode)) {
      throw std::runtime_error("Illegal object type watering_mode.");
    }
    const std::string watering_mode_str = json_watering_mode->valuestring;

    for (std::int32_t idx = WATERING_MODE_NONE; idx < WATERING_MODE_MAX;
         ++idx) {
      if (JSON_WATERING_MODE_TABLE[idx] == watering_mode_str) {
        watering_mode_ = static_cast<WateringMode>(idx);
        break;
      }
    }

    if (watering_mode_ == WATERING_MODE_NONE) {
      throw std::runtime_error("Illegal type watering_mode.");
    } else if (watering_mode_ == WATERING_MODE_SIMPLE) {
      return ParseSimple(json_root);
    } else if (watering_mode_ == WATERING_MODE_ADVANCE) {
      return ParseAdvance(json_root);
    }

  } catch (const std::invalid_argument &e) {
    ESP_LOGW(TAG, "Catch Exception. Invalid Argument String to Number.");
    return false;
  } catch (const std::out_of_range &e) {
    ESP_LOGW(TAG, "Catch Exception. Out Of Range String to Number.");
    return false;
  } catch (const std::runtime_error &e) {
    ESP_LOGW(TAG, "Catch Exception. Runtime Exception message:%s", e.what());
    return false;
  } catch (...) {
    ESP_LOGW(TAG, "An error occurred.");
    return false;
  }

  return false;
}

bool WateringSetting::ParseSimple(cJSON *json_root) noexcept(false) {
  ESP_LOGI(TAG, "Parse SimpleSetting");

  // Initialize
  watering_sec_ = 0;
  watering_hour_list_.clear();

  // Get Watering Sec
  const cJSON *const json_watering_sec =
      cJSON_GetObjectItemCaseSensitive(json_root, "watering_sec");
  if (!cJSON_IsNumber(json_watering_sec)) {
    throw std::runtime_error("Illegal object type watering_sec.");
  }
  watering_sec_ = json_watering_sec->valueint;

  // Get WateringHour
  const cJSON *const json_watering_hour_list =
      cJSON_GetObjectItemCaseSensitive(json_root, "watering_hour");
  if (!cJSON_IsArray(json_watering_hour_list)) {
    throw std::runtime_error("Illegal object type WateringHourList.");
  }
  const cJSON *json_watering_hour = nullptr;
  cJSON_ArrayForEach(json_watering_hour, json_watering_hour_list) {
    if (!cJSON_IsNumber(json_watering_hour)) {
      throw std::runtime_error("Illegal object type weatherArea.");
    }
    watering_hour_list_.push_back(json_watering_hour->valueint);
  }

  is_active_ = true;
  return true;
}

bool WateringSetting::ParseAdvance(cJSON *json_root) noexcept(false) {
  ESP_LOGI(TAG, "Parse AdvanceSetting");

  // Initialize
  watering_sec_ = 0;
  jma_area_path_code_ = 0;
  jma_local_code_ = 0;
  jmaamedas_point_num_ = 0;
  watering_type_dict_.clear();
  temperature_watering_list_.clear();
  month_to_type_dict_.clear();

  // Get Watering Sec
  const cJSON *const json_watering_sec =
      cJSON_GetObjectItemCaseSensitive(json_root, "watering_sec");
  if (!cJSON_IsNumber(json_watering_sec)) {
    throw std::runtime_error("Illegal object type watering_sec.");
  }
  watering_sec_ = json_watering_sec->valueint;

  {
    // Get Weather Forecast
    const cJSON *const json_weather_forecast =
        cJSON_GetObjectItemCaseSensitive(json_root, "wether_forecast");
    if (!cJSON_IsObject(json_weather_forecast)) {
      throw std::runtime_error("Illegal object type weather_forecast.");
    }

    // Service
    const cJSON *const json_service =
        cJSON_GetObjectItemCaseSensitive(json_weather_forecast, "service");
    if (!cJSON_IsString(json_service)) {
      throw std::runtime_error("Illegal object type weaarherAreaCode.");
    }
    const std::string service_str = json_service->valuestring;
    if (service_str != "jma") {
      throw std::runtime_error("Invalid Service.");
    }

    // AreaPathCode(jma)
    const cJSON *const json_area_path_code = cJSON_GetObjectItemCaseSensitive(
        json_weather_forecast, "area_path_code");
    if (!cJSON_IsNumber(json_area_path_code)) {
      throw std::runtime_error("Illegal object type area_path_code.");
    }
    jma_area_path_code_ = json_area_path_code->valueint;

    // LocalCode(jma)
    const cJSON *const json_local_code =
        cJSON_GetObjectItemCaseSensitive(json_weather_forecast, "local_code");
    if (!cJSON_IsNumber(json_local_code)) {
      throw std::runtime_error("Illegal object type local_code.");
    }
    jma_local_code_ = json_local_code->valueint;

    // AMeDASObservationPointNumber(jma)
    const cJSON *const json_amedas_observation_point_number =
        cJSON_GetObjectItemCaseSensitive(json_weather_forecast,
                                         "amedas_observation_point_number");
    if (!cJSON_IsNumber(json_amedas_observation_point_number)) {
      throw std::runtime_error(
          "Illegal object type amedas_observation_point_number.");
    }
    jmaamedas_point_num_ = json_amedas_observation_point_number->valueint;
  }
  {
    // Get Watering Type
    const cJSON *const json_watering_type_list =
        cJSON_GetObjectItemCaseSensitive(json_root, "watering_type");
    if (!cJSON_IsArray(json_watering_type_list)) {
      throw std::runtime_error("Illegal object type watering_type.");
    }

    const cJSON *json_watering_type = nullptr;
    cJSON_ArrayForEach(json_watering_type, json_watering_type_list) {
      WateringType wateringType;

      // Watering type name
      const cJSON *const json_type =
          cJSON_GetObjectItemCaseSensitive(json_watering_type, "type");
      if (!cJSON_IsString(json_type)) {
        throw std::runtime_error("Illegal object type \"type\".");
      }
      wateringType.WateringType = json_type->valuestring;

      // Day Span
      const cJSON *const json_day_span =
          cJSON_GetObjectItemCaseSensitive(json_watering_type, "day_span");
      if (!cJSON_IsNumber(json_day_span)) {
        throw std::runtime_error("Illegal object type day_span.");
      }
      wateringType.DaySpan = json_day_span->valueint;

      // WateringHour
      std::vector<std::int32_t> watering_hours;
      const cJSON *const json_watering_hour_list =
          cJSON_GetObjectItemCaseSensitive(json_watering_type, "watering_hour");
      if (!cJSON_IsArray(json_watering_hour_list)) {
        throw std::runtime_error("Illegal object type WateringHourList.");
      }
      const cJSON *json_watering_hour = nullptr;
      cJSON_ArrayForEach(json_watering_hour, json_watering_hour_list) {
        if (!cJSON_IsNumber(json_watering_hour)) {
          throw std::runtime_error("Illegal object type weatherArea.");
        }
        wateringType.WateringHours.push_back(json_watering_hour->valueint);
      }

      watering_type_dict_.insert(
          std::make_pair(wateringType.WateringType, wateringType));
    }
  }
  {
    // Temperature Watering
    const cJSON *const json_temperature_watering_list =
        cJSON_GetObjectItemCaseSensitive(json_root, "temperature_watering");
    if (!cJSON_IsArray(json_temperature_watering_list)) {
      throw std::runtime_error("Illegal object type watering_type.");
    }

    const cJSON *json_temperature_watering = nullptr;
    cJSON_ArrayForEach(json_temperature_watering, json_temperature_watering_list) {
      TemperatureWatering temperatureWatering;

      // UnderTemp
      const cJSON *const json_temperature = cJSON_GetObjectItemCaseSensitive(
          json_temperature_watering, "temperature");
      if (!cJSON_IsNumber(json_temperature)) {
        throw std::runtime_error("Illegal object type temperature.");
      }
      temperatureWatering.Temperature = json_temperature->valueint;

      // NormalType
      const cJSON *const json_normal_type = cJSON_GetObjectItemCaseSensitive(
          json_temperature_watering, "normal_type");
      if (!cJSON_IsString(json_normal_type)) {
        throw std::runtime_error("Illegal object type normal_type.");
      }
      temperatureWatering.NormalType = json_normal_type->valuestring;

      // RainType
      const cJSON *const json_rain_type = cJSON_GetObjectItemCaseSensitive(
          json_temperature_watering, "rain_type");
      if (!cJSON_IsString(json_rain_type)) {
        throw std::runtime_error("Illegal object type rain_type.");
      }
      temperatureWatering.RainType = json_rain_type->valuestring;

      temperature_watering_list_.push_back(temperatureWatering);
    }
    // Sort
    std::sort(
        temperature_watering_list_.begin(), temperature_watering_list_.end(),
        [](const TemperatureWatering &left, const TemperatureWatering &right) {
          return left.Temperature < right.Temperature;
        });
  }
  {
    // Month To Type (could not get Wether forecast)
    const cJSON *const json_month_to_type_list =
        cJSON_GetObjectItemCaseSensitive(json_root, "month_to_type");
    if (!cJSON_IsArray(json_month_to_type_list)) {
      throw std::runtime_error("Illegal object type month_to_type.");
    }

    const cJSON *json_month_to_type_iter = nullptr;
    cJSON_ArrayForEach(json_month_to_type_iter, json_month_to_type_list) {
      if (!cJSON_IsObject(json_month_to_type_iter)) {
        throw std::runtime_error("Illegal object type month_to_type.");
      }
      const cJSON *json_month_to_type = json_month_to_type_iter->child;
      if (!json_month_to_type || !json_month_to_type->string) {
        throw std::runtime_error("Illegal object type month_to_type.");
      }
      month_to_type_dict_.insert(std::make_pair(json_month_to_type->string,
                                              json_month_to_type->valuestring));
    }
  }
  {
    // Valve Power Control
    const cJSON *const json_valve_power_control =
        cJSON_GetObjectItemCaseSensitive(json_root, "valve_power_control");
    if (!cJSON_IsObject(json_valve_power_control)) {
      throw std::runtime_error("Illegal object type valve_power_control.");
    }

    // Base Voltage
    const cJSON *const json_base_voltage = cJSON_GetObjectItemCaseSensitive(
        json_valve_power_control, "base_voltage");
    if (!cJSON_IsNumber(json_base_voltage)) {
      throw std::runtime_error("Illegal object type base_voltage.");
    }
    base_voltage_ = json_base_voltage->valuedouble;

    // BaseRate
    const cJSON *const json_base_rate =
        cJSON_GetObjectItemCaseSensitive(json_valve_power_control, "base_rate");
    if (!cJSON_IsNumber(json_base_rate)) {
      throw std::runtime_error("Illegal object type base_rate.");
    }
    base_rate_ = json_base_rate->valuedouble;

    // Voltage Rate
    const cJSON *const json_voltage_rate = cJSON_GetObjectItemCaseSensitive(
        json_valve_power_control, "voltage_rate");
    if (!cJSON_IsNumber(json_voltage_rate)) {
      throw std::runtime_error("Illegal object type voltage_rate.");
    }
    voltage_rate_ = json_voltage_rate->valuedouble;
  }

  is_active_ = true;
  return true;
}

bool WateringSetting::Save(const std::string &body) {
  ESP_LOGV(TAG, "SAVE");
  return FileSystem::Write(WateringSetting::SETTING_FILE_NAME, body);
}

bool WateringSetting::Load(std::string &body) {
  body.clear();
  const bool is_read_ok =
      FileSystem::Read(WateringSetting::SETTING_FILE_NAME, body);
  if (!is_read_ok) {
    return false;
  }
  return true;
}

bool WateringSetting::Delete() {
  return FileSystem::Delete(WateringSetting::SETTING_FILE_NAME);
}

}  // namespace IrrigationSystem

// EOF
