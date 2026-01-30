// DMA_Engine.h - Zero Elite DMA Memory Engine Header
// Direct Memory Access functionality using VMMDLL

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "../include/vmmdll.h"

namespace ZeroElite {

    // Configuration Constants
    constexpr DWORD TARGET_PID = 35028;
    constexpr ULONG64 TARGET_HANDLE = 0x021EE040;

    class DMAEngine
    {
    public:
        // Initialize DMA engine with target process
        static bool Initialize(DWORD targetPID = TARGET_PID, 
                               HANDLE targetHandle = reinterpret_cast<HANDLE>(TARGET_HANDLE));
        
        // Shutdown DMA engine
        static void Shutdown();
        
        // Check if initialized
        static bool IsInitialized();
        
        // Memory operations
        static bool ReadMemory(ULONG64 address, PVOID buffer, SIZE_T size);
        static bool WriteMemory(ULONG64 address, PVOID buffer, SIZE_T size);
        
        // Physical memory operations
        static bool ReadPhysicalMemory(ULONG64 physAddress, PVOID buffer, SIZE_T size);
        static bool WritePhysicalMemory(ULONG64 physAddress, PVOID buffer, SIZE_T size);
        
        // Module operations
        static ULONG64 GetModuleBase(const char* moduleName);
        static ULONG64 GetProcAddress(const char* moduleName, const char* functionName);
        
        // Utility functions
        static bool VirtualToPhysical(ULONG64 virtualAddress, ULONG64* physicalAddress);
        
        // Getters
        static DWORD GetTargetPID();
        static HANDLE GetTargetHandle();
        static VMM_HANDLE GetVMMHandle();
        
        // Template read functions
        template<typename T>
        static T Read(ULONG64 address)
        {
            T value{};
            ReadMemory(address, &value, sizeof(T));
            return value;
        }
        
        template<typename T>
        static bool Read(ULONG64 address, T* value)
        {
            return ReadMemory(address, value, sizeof(T));
        }
        
        template<typename T>
        static bool Write(ULONG64 address, const T& value)
        {
            return WriteMemory(address, const_cast<T*>(&value), sizeof(T));
        }
        
    private:
        static VMM_HANDLE s_hVMM;
        static DWORD s_dwTargetPID;
        static HANDLE s_hTargetHandle;
        static bool s_bInitialized;
        static ULONG64 s_qwModuleBase;
    };

    // Scatter read helper for batch memory operations
    class ScatterRead
    {
    public:
        ScatterRead();
        ~ScatterRead();
        
        bool Initialize(DWORD pid, DWORD flags = 0);
        bool Prepare(ULONG64 address, DWORD size);
        bool PrepareEx(ULONG64 address, DWORD size, PVOID buffer);
        bool Execute();
        bool Read(ULONG64 address, DWORD size, PVOID buffer);
        void Clear();
        void Close();
        
        template<typename T>
        bool Prepare(ULONG64 address)
        {
            return Prepare(address, sizeof(T));
        }
        
        template<typename T>
        bool Read(ULONG64 address, T* value)
        {
            return Read(address, sizeof(T), value);
        }
        
    private:
        PVMMDLL_SCATTER_HANDLE m_hScatter;
        DWORD m_dwPID;
    };

} // namespace ZeroElite
