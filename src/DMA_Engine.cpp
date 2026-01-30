// DMA_Engine.cpp - PROJECT ZERO DMA Memory Engine
// Direct Memory Access using VMMDLL library

#include "DMA_Engine.hpp"
#include <vmmdll.h>
#include <iostream>

// ربط المكتبة من مجلد libs
#pragma comment(lib, "..\\libs\\vmmdll.lib")

// متغيرات عامة للمحرك
static VMM_HANDLE g_hVMM = nullptr;
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
        // استخدام الـ PID المحدد مباشرة (35028)
        g_dwTargetPID = TARGET_PID;
        std::cout << "[DMA Engine] Using predefined PID: " << TARGET_PID << std::endl;
    }

    g_bConnected = true;
    std::cout << "[DMA Engine] Connection established!" << std::endl;
    
    return true;
}

// إغلاق محرك DMA
void ShutdownZeroDMA() {
    if (g_hVMM) {
        VMMDLL_Close(g_hVMM);
        g_hVMM = nullptr;
        std::cout << "[DMA Engine] Shutdown complete" << std::endl;
    }
    g_bConnected = false;
}

// قراءة الذاكرة
bool ReadMemory(ULONG64 address, void* buffer, SIZE_T size) {
    if (!g_bConnected || !g_hVMM || !buffer || size == 0) {
        return false;
    }

    return VMMDLL_MemRead(g_hVMM, g_dwTargetPID, address, 
                          reinterpret_cast<PBYTE>(buffer), 
                          static_cast<DWORD>(size));
}

// كتابة الذاكرة
bool WriteMemory(ULONG64 address, void* buffer, SIZE_T size) {
    if (!g_bConnected || !g_hVMM || !buffer || size == 0) {
        return false;
    }

    return VMMDLL_MemWrite(g_hVMM, g_dwTargetPID, address,
                           reinterpret_cast<PBYTE>(buffer),
                           static_cast<DWORD>(size));
}

// الحصول على عنوان الـ Module
ULONG64 GetModuleBase(const char* moduleName) {
    if (!g_bConnected || !g_hVMM || !moduleName) {
        return 0;
    }

    return VMMDLL_ProcessGetModuleBase(g_hVMM, g_dwTargetPID, 
                                       const_cast<LPSTR>(moduleName));
}

// التحقق من الاتصال
bool IsConnected() {
    return g_bConnected && g_hVMM != nullptr;
}
