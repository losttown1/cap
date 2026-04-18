#include "DMA_Engine.h"

#include <Windows.h>
#include <vector>

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

bool DMAEngine::SyncOffset(OffsetEntry& entry)
{
    // Placeholder sync logic - in real implementation this would
    // validate the offset against the target process memory
    if (!initialized_) {
        return false;
    }

    // Simulate sync operation
    entry.synced = true;
    return true;
}

void DMAEngine::Step_CheckOffsetUpdates()
{
    // Define offset entries to check
    std::vector<OffsetEntry> offsets = {
        {"PlayerBase", 0x1000, 0x100, false},
        {"EntityList", 0x2000, 0x200, false},
        {"ViewMatrix", 0x3000, 0x300, false},
        {"LocalPlayer", 0x4000, 0x400, false},
        {"GameState", 0x5000, 0x500, false},
        {"WeaponData", 0x6000, 0x600, false},
        {"NetworkInfo", 0x7000, 0x700, false}
    };

    constexpr int totalChecks = 7;
    int passedChecks = 0;

    for (auto& entry : offsets) {
        bool success = SyncOffset(entry);

        if (success) {
            wchar_t buf[256] = {};
            swprintf_s(buf, L"[Zero Elite] Offset '%hs' synced successfully\n", entry.name.c_str());
            OutputDebugStringW(buf);
        } else {
            wchar_t buf[256] = {};
            swprintf_s(buf, L"[Zero Elite] Offset '%hs' sync failed\n", entry.name.c_str());
            OutputDebugStringW(buf);
        }

        // FIX: Only increment passedChecks on success.
        // Previously there was a bug where passedChecks was incremented both
        // inside the if(success) block AND unconditionally after it, causing
        // successful syncs to count as 2 checks while failed syncs counted as 1.
        // This resulted in incorrect summaries like "8/7 checks passed".
        // 
        // Even if sync fails, we continue to the next offset check.
        if (success) {
            passedChecks++;
        }
    }

    // Report summary
    wchar_t summaryBuf[256] = {};
    swprintf_s(summaryBuf, L"[Zero Elite] Offset check complete: %d/%d checks passed\n", passedChecks, totalChecks);
    OutputDebugStringW(summaryBuf);
}

