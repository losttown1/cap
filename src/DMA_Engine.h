#pragma once

#include <cstdint>

class DMAEngine
{
public:
    static constexpr std::uint32_t kTargetPid = 35028u;
    static constexpr std::uint64_t kTargetHandle = 0x021EE040ull;

    bool Initialize();
    void Shutdown();

private:
    bool initialized_ = false;
};

