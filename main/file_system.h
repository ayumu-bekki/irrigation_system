#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_
// ESP32 Irrigation System
// (C)2021 bekki.jp
// FileSystem

// Include ----------------------
#include <string>

namespace IrrigationSystem {
namespace FileSystem {

/// Mount File system
bool Mount();

/// Unmount File System
void Unmount();

/// Write
bool Write(const std::string& filePath, const std::string& body);

/// Read
bool Read(const std::string& filePath, std::string& body);

/// Delete
bool Delete(const std::string& filePath);


} // FileSystem
} // IrrigationSystem

#endif // FILE_SYSTEM_H_
// EOF
