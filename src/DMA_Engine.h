#pragma once

#include <cstdint>
#include <string>

// Offset entry structure for memory scanning
struct OffsetEntry {
    std::string name;
    std::uint64_t baseAddress;
    std::uint64_t offset;
    bool synced;
};

class DMAEngine
{
public:
    static constexpr std::uint32_t kTargetPid = 35028u;
    static constexpr std::uint64_t kTargetHandle = 0x021EE040ull;

    bool Initialize();
    void Shutdown();

    // Offset update checking - validates memory offsets are current
    void Step_CheckOffsetUpdates();

private:
    bool initialized_ = false;

    // Internal helper for syncing individual offsets
    bool SyncOffset(OffsetEntry& entry);
};

