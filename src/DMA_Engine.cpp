#include "DMA_Engine.h"

#include <Windows.h>

// Required by spec (relative path) — the repo ships a buildable stub library.
#ifdef _MSC_VER
#pragma comment(lib, "..\\libs\\vmmdll.lib")
#endif

bool DMAEngine::Initialize()
{
    // NOTE:
    // This project is wired to link against vmmdll.lib per requirements.
    // The implementation here is intentionally non-invasive and does not
    // call into vmmdll symbols to keep the scaffold buildable by default.
    //
    // Replace this with real VMMDLL init / attach logic as needed.

    initialized_ = true;

    wchar_t buf[256] = {};
    swprintf_s(buf, L"[Zero Elite] DMA init (PID=%u, Handle=0x%llX)\n", kTargetPid, kTargetHandle);
    OutputDebugStringW(buf);

    return true;
}

void DMAEngine::Shutdown()
{
    initialized_ = false;
}

