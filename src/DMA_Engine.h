#pragma once
#include <windows.h>
#include <iostream>
#include <vector>

// Forward declare if we had the real header, or include it
#include "../include/vmmdll.h"

class DMA_Engine {
public:
    DMA_Engine();
    ~DMA_Engine();

    bool Init();
    bool AttachToProcess();
    
    // Generic read function
    template <typename T>
    T Read(uint64_t address) {
        T buffer;
        if (ReadMemory(address, &buffer, sizeof(T))) {
            return buffer;
        }
        return {};
    }

    // Generic write function
    template <typename T>
    bool Write(uint64_t address, T value) {
        return WriteMemory(address, &value, sizeof(T));
    }

    bool ReadMemory(uint64_t address, void* buffer, size_t size);
    bool WriteMemory(uint64_t address, void* buffer, size_t size);

private:
    DWORD targetPID = 35028;
    uint64_t targetHandle = 0x021EE040; // Assuming this is a handle or base address as per prompt
    
    // VMMDLL handle if we were using the real lib
    // VMM_HANDLE hVMM; 
};
