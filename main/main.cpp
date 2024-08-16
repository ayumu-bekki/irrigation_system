// ESP32 Irrigation System
// (C)2021 bekki.jp

// Include ----------------------
#include "irrigation_controller.h"

/// Entry Point
extern "C" void app_main() {
  const auto irrigation_controller =
      std::make_shared<IrrigationSystem::IrrigationController>();
  irrigation_controller->Start();
}

// EOF
