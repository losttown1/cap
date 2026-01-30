// DMA_Engine.cpp - PROJECT ZERO DMA Memory Engine
// Direct Memory Access using VMMDLL library
// Note: DMA functions are optional - overlay works without DMA hardware

#include "DMA_Engine.hpp"
#include <iostream>

// تعطيل ربط vmmdll.lib مؤقتاً - فعّله عندما يكون لديك الملف الصحيح x64
// #pragma comment(lib, "..\\libs\\vmmdll.lib")

// تعريف مؤقت للدوال - استبدلها بـ vmmdll.h الحقيقي عند التوفر
#define DMA_ENABLED 0  // غيّر إلى 1 عندما يكون vmmdll.lib x64 متوفر

#if DMA_ENABLED
#include <vmmdll.h>
#pragma comment(lib, "..\\libs\\vmmdll.lib")
static VMM_HANDLE g_hVMM = nullptr;
#else
// Stub definitions when DMA is disabled
typedef void* VMM_HANDLE;
static VMM_HANDLE g_hVMM = nullptr;
#endif

// متغيرات عامة للمحرك
static DWORD g_dwTargetPID = TARGET_PID;
static bool g_bConnected = false;

// تهيئة محرك DMA
bool InitializeZeroDMA() {
    if (g_bConnected) {
        std::cout << "[DMA Engine] Already initialized" << std::endl;
        return true;
    }

    std::cout << "[DMA Engine] Initializing..." << std::endl;
    std::cout << "[DMA Engine] Target PID: " << TARGET_PID << std::endl;
    std::cout << "[DMA Engine] Target Handle: 0x" << std::hex << TARGET_HANDLE << std::dec << std::endl;

#if DMA_ENABLED
    // إعداد معاملات VMMDLL
    LPSTR args[] = { 
        (LPSTR)"", 
        (LPSTR)"-device", 
        (LPSTR)"fpga",
        (LPSTR)"-v"
    };
    
    // تهيئة VMMDLL
    g_hVMM = VMMDLL_Initialize(4, args);
    
    if (!g_hVMM) {
        std::cerr << "[DMA Engine] Failed to initialize VMMDLL" << std::endl;
        std::cerr << "[DMA Engine] Make sure FPGA device is connected" << std::endl;
        return false;
    }

    std::cout << "[DMA Engine] VMMDLL initialized successfully" << std::endl;

    // محاولة البحث عن العملية بالاسم أولاً
    DWORD dwPID = 0;
    if (VMMDLL_PidGetFromName(g_hVMM, (LPSTR)"Game.exe", &dwPID)) {
        g_dwTargetPID = dwPID;
        std::cout << "[DMA Engine] Found process by name, PID: " << dwPID << std::endl;
    } else {
        g_dwTargetPID = TARGET_PID;
        std::cout << "[DMA Engine] Using predefined PID: " << TARGET_PID << std::endl;
    }

    g_bConnected = true;
    std::cout << "[DMA Engine] Connection established!" << std::endl;
#else
    // وضع المحاكاة - بدون DMA حقيقي
    std::cout << "[DMA Engine] Running in SIMULATION MODE (no real DMA)" << std::endl;
    std::cout << "[DMA Engine] To enable real DMA:" << std::endl;
    std::cout << "[DMA Engine]   1. Get vmmdll.lib (x64) from MemProcFS" << std::endl;
    std::cout << "[DMA Engine]   2. Set DMA_ENABLED to 1 in DMA_Engine.cpp" << std::endl;
    g_bConnected = true;
#endif
    
    return true;
}

// إغلاق محرك DMA
void ShutdownZeroDMA() {
#if DMA_ENABLED
    if (g_hVMM) {
        VMMDLL_Close(g_hVMM);
        g_hVMM = nullptr;
        std::cout << "[DMA Engine] Shutdown complete" << std::endl;
    }
#else
    std::cout << "[DMA Engine] Simulation shutdown complete" << std::endl;
#endif
    g_bConnected = false;
}

// قراءة الذاكرة
bool ReadMemory(ULONG64 address, void* buffer, SIZE_T size) {
    if (!g_bConnected || !buffer || size == 0) {
        return false;
    }

#if DMA_ENABLED
    if (!g_hVMM) return false;
    return VMMDLL_MemRead(g_hVMM, g_dwTargetPID, address, 
                          reinterpret_cast<PBYTE>(buffer), 
                          static_cast<DWORD>(size));
#else
    // محاكاة - إرجاع أصفار
    memset(buffer, 0, size);
    return true;
#endif
}

// كتابة الذاكرة
bool WriteMemory(ULONG64 address, void* buffer, SIZE_T size) {
    if (!g_bConnected || !buffer || size == 0) {
        return false;
    }

#if DMA_ENABLED
    if (!g_hVMM) return false;
    return VMMDLL_MemWrite(g_hVMM, g_dwTargetPID, address,
                           reinterpret_cast<PBYTE>(buffer),
                           static_cast<DWORD>(size));
#else
    // محاكاة - لا شيء
    return true;
#endif
}

// الحصول على عنوان الـ Module
ULONG64 GetModuleBase(const char* moduleName) {
    if (!g_bConnected || !moduleName) {
        return 0;
    }

#if DMA_ENABLED
    if (!g_hVMM) return 0;
    return VMMDLL_ProcessGetModuleBase(g_hVMM, g_dwTargetPID, 
                                       const_cast<LPSTR>(moduleName));
#else
    // محاكاة - إرجاع عنوان وهمي
    return 0x140000000;
#endif
}

// التحقق من الاتصال
bool IsConnected() {
    return g_bConnected;
}
