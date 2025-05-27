// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "watering_record.h"

#include <cJSON.h>

#include <sstream>
#include <stdexcept>

#include "file_system.h"
#include "logger.h"
#include "util.h"

namespace IrrigationSystem {

WateringRecord::WateringRecord() : last_watering_epoch_(0) {}

bool WateringRecord::Save() const {
  // Write History
  std::stringstream historyBody;
  historyBody << "{\"last_watering_date\":\""
              << Util::TimeToStr(Util::EpochToLocalTime(last_watering_epoch_))
              << "\"}";
  return FileSystem::Write(WateringRecord::RECORD_FILE_NAME, historyBody.str());
}

bool WateringRecord::Load() noexcept {
  last_watering_epoch_ = 0;

  std::string recordBody;
  const bool is_read_ok =
      FileSystem::Read(WateringRecord::RECORD_FILE_NAME, recordBody);
  if (!is_read_ok) {
    ESP_LOGE(TAG, "Failed Read File.");
    return false;
  }
  ESP_LOGV(TAG, "Log Body:%s", recordBody.c_str());

  // Parse
  cJSON* json_root = nullptr;
  try {
    json_root = cJSON_Parse(recordBody.c_str());
    if (!json_root) {
      const char* error_ptr = cJSON_GetErrorPtr();
      if (error_ptr) {
        throw std::runtime_error(error_ptr);
      }
      throw std::runtime_error("Json Parse Error.");
    }

    const cJSON* const json_last_watering_date =
        cJSON_GetObjectItemCaseSensitive(json_root, "last_watering_date");
    if (!cJSON_IsString(json_last_watering_date)) {
      throw std::runtime_error("Illegal object type weatherAreaCode.");
    }
    const std::string last_watering_date_str =
        json_last_watering_date->valuestring;

    tm timeInfo;
    strptime(last_watering_date_str.c_str(), "%Y/%m/%d %H:%M:%S", &timeInfo);
    last_watering_epoch_ = mktime(&timeInfo);

#if CONFIG_DEBUG != 0
    // show read date
    char buf[255] = {};
    strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M", &timeInfo);
    ESP_LOGV(TAG, "Read Date:%s", buf);

    // std::cout << "Y:" << (timeInfo.tm_year + 1900) << std::endl;
    // std::cout << "M:" << (timeInfo.tm_mon + 1) << std::endl;
    // std::cout << "D:" << timeInfo.tm_mday << std::endl;
#endif  // CONFIG_DEBUG != 0

  } catch (const std::invalid_argument& e) {
    ESP_LOGW(TAG, "Catch Exception. Invalid Argument String to Number.");
    return false;
  } catch (const std::out_of_range& e) {
    ESP_LOGW(TAG, "Catch Exception. Out Of Range String to Number.");
    return false;
  } catch (const std::runtime_error& e) {
    ESP_LOGW(TAG, "Catch Exception. Runtime Exception message:%s", e.what());
    return false;
  } catch (...) {
    ESP_LOGW(TAG, "An error occurred.");
    return false;
  }

  return true;
}

#if CONFIG_DEBUG != 0
bool WateringRecord::Delete() {
  if (!FileSystem::Delete(WateringRecord::RECORD_FILE_NAME)) {
    ESP_LOGE(TAG, "Failed Delete File.");
    return false;
  }
  ESP_LOGE(TAG, "Deleted File.");
  return true;
}
#endif  // CONFIG_DEBUG != 0

void WateringRecord::SetLastWateringEpoch(const std::time_t watering_epoch) {
  last_watering_epoch_ = watering_epoch;
}

time_t WateringRecord::GetLastWateringEpoch() const {
  return last_watering_epoch_;
}

}  // namespace IrrigationSystem

// EOF
