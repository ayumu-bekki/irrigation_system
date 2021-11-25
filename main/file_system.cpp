// ESP32 Irrigation System
// (C)2021 bekki.jp
// FileSystem

// Include ----------------------
#include "file_system.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#include "logger.h"

namespace IrrigationSystem {
namespace FileSystem {

static constexpr char *const base_path = (char*)"/spiflash";

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE; // Handle of the wear levelling library instance

/// Mount File system
bool Mount()
{
    // Mount File System
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {true, 4, CONFIG_WL_SECTOR_SIZE};
    esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return false;
    }
    return true;
}

/// Unmount File System
void Unmount()
{
    ESP_LOGI(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK( esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));
}

/// Write
bool Write(const std::string& filePath, const std::string& body)
{
    std::fstream fileOpenStream;
    fileOpenStream.open(base_path + std::string("/") + filePath, std::ios::out);
    if (!fileOpenStream.is_open()) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return false;
    }
    fileOpenStream << body << std::flush;
    return true;
}

/// Read
bool Read(const std::string& filePath, std::string& body)
{
    std::fstream fileReadStream;
    fileReadStream.open(base_path + std::string("/") + filePath, std::ios::in);
    if (!fileReadStream.is_open()) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return false;
    }
    body = static_cast<std::stringstream const&>(std::stringstream() << fileReadStream.rdbuf()).str();
    return true;
}

/// Delete
bool Delete(const std::string& filePath)
{
    return std::remove((base_path + std::string("/") + filePath).c_str()) == 0;
}

} // FileSystem
} // IrrigationSystem

// EOF
