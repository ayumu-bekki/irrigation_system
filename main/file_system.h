#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp
// FileSystem

// Include ----------------------
#include <string>

namespace IrrigationSystem::FileSystem {

/// Mount File system
bool Mount();

/// Unmount File System
void Unmount();

/// Write
bool Write(const std::string& file_path, const std::string& body);

/// Read
bool Read(const std::string& file_path, std::string& body);

/// Delete
bool Delete(const std::string& file_path);

}  // namespace IrrigationSystem::FileSystem

#endif  // FILE_SYSTEM_H_
// EOF
