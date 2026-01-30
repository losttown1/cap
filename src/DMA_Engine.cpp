#include "DMA_Engine.h"

#include <windows.h>

#pragma comment(lib, "..\\libs\\vmmdll.lib")

bool DMAEngine::Initialize() {
  return true;
}

void DMAEngine::Shutdown() {}

uint32_t DMAEngine::TargetPid() const {
  return kTargetPid;
}

uintptr_t DMAEngine::TargetHandle() const {
  return kTargetHandle;
}
