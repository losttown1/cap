#pragma once

#include <cstdint>

class DMAEngine {
 public:
  static constexpr uint32_t kTargetPid = 35028;
  static constexpr uintptr_t kTargetHandle = 0x021EE040;

  bool Initialize();
  void Shutdown();

  uint32_t TargetPid() const;
  uintptr_t TargetHandle() const;
};
