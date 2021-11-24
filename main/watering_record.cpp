// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "watering_record.h"

#include <stdexcept>
#include <sstream>

#include <cJSON.h>

#include "logger.h"
#include "util.h"
#include "file_system.h"

namespace IrrigationSystem {

WateringRecord::WateringRecord()
    :m_LastWateringEpoch(0)
{}

bool WateringRecord::Save() const
{
    // Write History
    std::stringstream historyBody;
    historyBody << "{\"last_watering_date\":\""
                << Util::GetNowTimeStr()
                << "\"}";
    return FileSystem::Write(WateringRecord::RECORD_FILE_NAME, historyBody.str());
}

bool WateringRecord::Load() noexcept
{
    m_LastWateringEpoch = 0;

    std::string recordBody;
    const bool isReadOk = FileSystem::Read(WateringRecord::RECORD_FILE_NAME, recordBody);
    if (!isReadOk) {
        ESP_LOGE(TAG, "Failed Read File.");
    }
    ESP_LOGV(TAG, "Log Body:%s", recordBody.c_str());
 
    // Parse 
    cJSON* pJsonRoot = nullptr;
    try {
        pJsonRoot = cJSON_Parse(recordBody.c_str());
        if (!pJsonRoot){
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr)
            {
                throw std::runtime_error(error_ptr);
            }
            throw std::runtime_error("Json Parse Error.");
        }

        const cJSON *const pJsonLastWateringDate = cJSON_GetObjectItemCaseSensitive(pJsonRoot, "last_watering_date");
        if (!cJSON_IsString(pJsonLastWateringDate)) {
            throw std::runtime_error("Illegal object type weaarherAreaCode.");
        }
        const std::string lastWateringDateStr = pJsonLastWateringDate->valuestring;

        tm timeInfo;
        strptime(lastWateringDateStr.c_str(), "%Y/%m/%d %H:%M:%S", &timeInfo);
        m_LastWateringEpoch = mktime(&timeInfo);
        
#if CONFIG_DEBUG != 0
        // show read date
        char buf[255] = {};
        strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M", &timeInfo);
        ESP_LOGV(TAG, "Read Date:%s", buf);

        //std::cout << "Y:" << (timeInfo.tm_year + 1900) << std::endl;
        //std::cout << "M:" << (timeInfo.tm_mon + 1) << std::endl;
        //std::cout << "D:" << timeInfo.tm_mday << std::endl;
#endif // CONFIG_DEBUG != 0

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

    return true;
}

time_t WateringRecord::GetLastWateringEpoch() const
{
    return m_LastWateringEpoch;
}


} // IrrigationSystem

// EOF
