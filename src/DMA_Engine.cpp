// DMA_Engine.cpp - Zero Elite DMA Memory Engine
// Direct Memory Access functionality using VMMDLL

#include "DMA_Engine.h"
#include <iostream>

// Link VMMDLL library
#pragma comment(lib, "..\\libs\\vmmdll.lib")

namespace ZeroElite {

    // Static member initialization
    VMM_HANDLE DMAEngine::s_hVMM = nullptr;
    DWORD DMAEngine::s_dwTargetPID = 0;
    HANDLE DMAEngine::s_hTargetHandle = nullptr;
    bool DMAEngine::s_bInitialized = false;
    ULONG64 DMAEngine::s_qwModuleBase = 0;

    bool DMAEngine::Initialize(DWORD targetPID, HANDLE targetHandle)
    {
        if (s_bInitialized)
        {
            return true;
        }

        s_dwTargetPID = targetPID;
        s_hTargetHandle = targetHandle;

        // Initialize VMMDLL with FPGA device
        const char* args[] = {
            "",
            "-device", "fpga",
            "-v",
            "-printf"
        };

        s_hVMM = VMMDLL_Initialize(5, const_cast<LPSTR*>(args));
        if (!s_hVMM)
        {
            std::cerr << "[DMA Engine] Failed to initialize VMMDLL" << std::endl;
            return false;
        }

        std::cout << "[DMA Engine] Initialized successfully" << std::endl;
        std::cout << "[DMA Engine] Target PID: " << s_dwTargetPID << std::endl;
        std::cout << "[DMA Engine] Target Handle: 0x" << std::hex << reinterpret_cast<ULONG64>(s_hTargetHandle) << std::dec << std::endl;

        s_bInitialized = true;
        return true;
    }

    void DMAEngine::Shutdown()
    {
        if (s_hVMM)
        {
            VMMDLL_Close(s_hVMM);
            s_hVMM = nullptr;
        }

        s_bInitialized = false;
        s_dwTargetPID = 0;
        s_hTargetHandle = nullptr;
        s_qwModuleBase = 0;

        std::cout << "[DMA Engine] Shutdown complete" << std::endl;
    }

    bool DMAEngine::IsInitialized()
    {
        return s_bInitialized && s_hVMM != nullptr;
    }

    bool DMAEngine::ReadMemory(ULONG64 address, PVOID buffer, SIZE_T size)
    {
        if (!IsInitialized() || !buffer || size == 0)
        {
            return false;
        }

        return VMMDLL_MemRead(s_hVMM, s_dwTargetPID, address, 
                              reinterpret_cast<PBYTE>(buffer), 
                              static_cast<DWORD>(size));
    }

    bool DMAEngine::WriteMemory(ULONG64 address, PVOID buffer, SIZE_T size)
    {
        if (!IsInitialized() || !buffer || size == 0)
        {
            return false;
        }

        return VMMDLL_MemWrite(s_hVMM, s_dwTargetPID, address,
                               reinterpret_cast<PBYTE>(buffer),
                               static_cast<DWORD>(size));
    }

    bool DMAEngine::ReadPhysicalMemory(ULONG64 physAddress, PVOID buffer, SIZE_T size)
    {
        if (!IsInitialized() || !buffer || size == 0)
        {
            return false;
        }

        return VMMDLL_MemReadPhysical(s_hVMM, physAddress,
                                       reinterpret_cast<PBYTE>(buffer),
                                       static_cast<DWORD>(size));
    }

    bool DMAEngine::WritePhysicalMemory(ULONG64 physAddress, PVOID buffer, SIZE_T size)
    {
        if (!IsInitialized() || !buffer || size == 0)
        {
            return false;
        }

        return VMMDLL_MemWritePhysical(s_hVMM, physAddress,
                                        reinterpret_cast<PBYTE>(buffer),
                                        static_cast<DWORD>(size));
    }

    ULONG64 DMAEngine::GetModuleBase(const char* moduleName)
    {
        if (!IsInitialized() || !moduleName)
        {
            return 0;
        }

        return VMMDLL_ProcessGetModuleBase(s_hVMM, s_dwTargetPID, 
                                           const_cast<LPSTR>(moduleName));
    }

    ULONG64 DMAEngine::GetProcAddress(const char* moduleName, const char* functionName)
    {
        if (!IsInitialized() || !moduleName || !functionName)
        {
            return 0;
        }

        return VMMDLL_ProcessGetProcAddress(s_hVMM, s_dwTargetPID,
                                            const_cast<LPSTR>(moduleName),
                                            const_cast<LPSTR>(functionName));
    }

    bool DMAEngine::VirtualToPhysical(ULONG64 virtualAddress, ULONG64* physicalAddress)
    {
        if (!IsInitialized() || !physicalAddress)
        {
            return false;
        }

        return VMMDLL_MemVirt2Phys(s_hVMM, s_dwTargetPID, virtualAddress, physicalAddress);
    }

    DWORD DMAEngine::GetTargetPID()
    {
        return s_dwTargetPID;
    }

    HANDLE DMAEngine::GetTargetHandle()
    {
        return s_hTargetHandle;
    }

    VMM_HANDLE DMAEngine::GetVMMHandle()
    {
        return s_hVMM;
    }

    // ScatterRead implementation
    ScatterRead::ScatterRead()
        : m_hScatter(nullptr)
        , m_dwPID(0)
    {
    }

    ScatterRead::~ScatterRead()
    {
        Close();
    }

    bool ScatterRead::Initialize(DWORD pid, DWORD flags)
    {
        if (!DMAEngine::IsInitialized())
        {
            return false;
        }

        m_dwPID = pid;
        m_hScatter = VMMDLL_Scatter_Initialize(DMAEngine::GetVMMHandle(), pid, flags);
        
        return m_hScatter != nullptr;
    }

    bool ScatterRead::Prepare(ULONG64 address, DWORD size)
    {
        if (!m_hScatter)
        {
            return false;
        }

        return VMMDLL_Scatter_Prepare(m_hScatter, address, size);
    }

    bool ScatterRead::PrepareEx(ULONG64 address, DWORD size, PVOID buffer)
    {
        if (!m_hScatter || !buffer)
        {
            return false;
        }

        return VMMDLL_Scatter_PrepareEx(m_hScatter, address, size,
                                        reinterpret_cast<PBYTE>(buffer), nullptr);
    }

    bool ScatterRead::Execute()
    {
        if (!m_hScatter)
        {
            return false;
        }

        return VMMDLL_Scatter_Execute(m_hScatter);
    }

    bool ScatterRead::Read(ULONG64 address, DWORD size, PVOID buffer)
    {
        if (!m_hScatter || !buffer)
        {
            return false;
        }

        DWORD cbRead = 0;
        return VMMDLL_Scatter_Read(m_hScatter, address, size,
                                   reinterpret_cast<PBYTE>(buffer), &cbRead);
    }

    void ScatterRead::Clear()
    {
        if (m_hScatter)
        {
            VMMDLL_Scatter_Clear(m_hScatter, m_dwPID, 0);
        }
    }

    void ScatterRead::Close()
    {
        if (m_hScatter)
        {
            VMMDLL_Scatter_CloseHandle(m_hScatter);
            m_hScatter = nullptr;
        }
    }

} // namespace ZeroElite
