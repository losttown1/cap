// DMA_Engine.cpp - Professional DMA Implementation
// HIGH-PERFORMANCE: Uses VMMDLL_Scatter for batched reads

#include "DMA_Engine.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#endif

#if DMA_ENABLED
#include "vmmdll.h"
#pragma comment(lib, "vmmdll.lib")
static VMM_HANDLE g_VMMDLL = nullptr;
static DWORD g_DMA_PID = 0;
#endif

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
DMAConfig g_Config;
GameOffsets g_Offsets;
ScatterReadRegistry g_ScatterRegistry;
DiagnosticStatus g_DiagStatus;

// ============================================================================
// STATIC MEMBERS
// ============================================================================
bool DMAEngine::s_Connected = false;
bool DMAEngine::s_Online = false;
bool DMAEngine::s_SimulationMode = false;
uintptr_t DMAEngine::s_BaseAddress = 0;
size_t DMAEngine::s_ModuleSize = 0;
char DMAEngine::s_StatusText[64] = "OFFLINE";
char DMAEngine::s_DeviceInfo[128] = "No device";

bool PatternScanner::s_Scanned = false;
int PatternScanner::s_FoundCount = 0;

std::vector<PlayerData> PlayerManager::s_Players;
PlayerData PlayerManager::s_LocalPlayer;
std::mutex PlayerManager::s_Mutex;
bool PlayerManager::s_Initialized = false;
float PlayerManager::s_SimTime = 0;

MapInfo MapTextureManager::s_CurrentMap;
std::unordered_map<std::string, MapInfo> MapTextureManager::s_MapDatabase;

// ============================================================================
// KNOWN GENERIC/LEAKED DEVICE IDS
// ============================================================================
static const char* GENERIC_DEVICE_IDS[] = {
    "10EE:0007", "10EE:7024", "1234:5678", nullptr
};

static const char* LEAKED_DEVICE_IDS[] = {
    "1172:E001", "10EE:7011", "10EE:7021", "CAFE:BABE", nullptr
};

// ============================================================================
// VMMDLL SCATTER WRAPPER CLASS
// ============================================================================
#if DMA_ENABLED
class VMDMAScatter {
public:
    VMDMAScatter() : m_hScatter(nullptr), m_Prepared(false) {}
    
    ~VMDMAScatter() {
        Close();
    }
    
    bool Initialize() {
        if (!g_VMMDLL || !g_DMA_PID) return false;
        
        // Create scatter handle with NOCACHE flag for real-time data
        m_hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
        m_Prepared = false;
        return m_hScatter != nullptr;
    }
    
    bool Prepare(ULONG64 address, DWORD size) {
        if (!m_hScatter) return false;
        m_Prepared = true;
        return VMMDLL_Scatter_Prepare(m_hScatter, address, size);
    }
    
    bool PrepareWrite(ULONG64 address, PBYTE data, DWORD size) {
        if (!m_hScatter) return false;
        m_Prepared = true;
        return VMMDLL_Scatter_PrepareWrite(m_hScatter, address, data, size);
    }
    
    bool Execute() {
        if (!m_hScatter || !m_Prepared) return false;
        return VMMDLL_Scatter_Execute(m_hScatter);
    }
    
    bool Read(ULONG64 address, PBYTE buffer, DWORD size) {
        if (!m_hScatter) return false;
        return VMMDLL_Scatter_Read(m_hScatter, address, size, buffer, nullptr);
    }
    
    bool Clear() {
        if (!m_hScatter) return false;
        m_Prepared = false;
        return VMMDLL_Scatter_Clear(m_hScatter, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    }
    
    void Close() {
        if (m_hScatter) {
            VMMDLL_Scatter_CloseHandle(m_hScatter);
            m_hScatter = nullptr;
        }
        m_Prepared = false;
    }
    
    bool IsValid() const { return m_hScatter != nullptr; }
    
private:
    VMMDLL_SCATTER_HANDLE m_hScatter;
    bool m_Prepared;
};

// Global scatter instance for reuse
static VMDMAScatter g_Scatter;
#endif

// ============================================================================
// HIGH-PERFORMANCE SCATTER READ FUNCTION
// ============================================================================
void ExecuteScatterReads(std::vector<ScatterEntry>& entries)
{
    if (entries.empty()) return;
    
#if DMA_ENABLED
    if (!DMAEngine::IsConnected() || !g_VMMDLL || !g_DMA_PID)
    {
        // Simulation mode - zero fill
        for (auto& entry : entries)
            if (entry.buffer) memset(entry.buffer, 0, entry.size);
        return;
    }
    
    // Initialize scatter handle
    VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    if (!hScatter)
    {
        // Fallback to individual reads
        for (auto& entry : entries)
        {
            if (entry.buffer && entry.address && entry.size > 0)
            {
                VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, entry.address, 
                                (PBYTE)entry.buffer, (DWORD)entry.size, nullptr, VMMDLL_FLAG_NOCACHE);
            }
        }
        return;
    }
    
    // STEP 1: Prepare all reads (queue them)
    for (auto& entry : entries)
    {
        if (entry.address && entry.size > 0)
        {
            VMMDLL_Scatter_Prepare(hScatter, entry.address, (DWORD)entry.size);
        }
    }
    
    // STEP 2: Execute all reads in ONE DMA transaction
    VMMDLL_Scatter_Execute(hScatter);
    
    // STEP 3: Read results from scatter buffer
    for (auto& entry : entries)
    {
        if (entry.buffer && entry.address && entry.size > 0)
        {
            VMMDLL_Scatter_Read(hScatter, entry.address, (DWORD)entry.size, (PBYTE)entry.buffer, nullptr);
        }
    }
    
    // STEP 4: Clean up
    VMMDLL_Scatter_CloseHandle(hScatter);
    
#else
    // Simulation mode
    for (auto& entry : entries)
        if (entry.buffer) memset(entry.buffer, 0, entry.size);
#endif
}

// ============================================================================
// DIAGNOSTIC SYSTEM IMPLEMENTATION
// ============================================================================
const char* DiagnosticSystem::GetErrorString(DiagnosticResult result)
{
    switch (result)
    {
    case DiagnosticResult::SUCCESS: return "Success";
    case DiagnosticResult::DEVICE_NOT_FOUND: return "DMA Device Not Found";
    case DiagnosticResult::DEVICE_BUSY: return "DMA Device Busy";
    case DiagnosticResult::FIRMWARE_INVALID: return "Invalid Firmware";
    case DiagnosticResult::FIRMWARE_GENERIC: return "Generic Firmware Detected";
    case DiagnosticResult::FIRMWARE_LEAKED: return "Leaked Firmware ID Detected";
    case DiagnosticResult::SPEED_TOO_SLOW: return "DMA Speed Insufficient";
    case DiagnosticResult::PROCESS_NOT_FOUND: return "Target Process Not Found";
    case DiagnosticResult::BASE_ADDRESS_FAIL: return "Failed to Get Base Address";
    case DiagnosticResult::MEMORY_READ_FAIL: return "Memory Read Test Failed";
    default: return "Unknown Error";
    }
}

bool DiagnosticSystem::IsGenericDeviceID(const char* deviceID)
{
    if (!deviceID || !deviceID[0]) return false;
    for (int i = 0; GENERIC_DEVICE_IDS[i] != nullptr; i++)
        if (strstr(deviceID, GENERIC_DEVICE_IDS[i]) != nullptr) return true;
    return false;
}

bool DiagnosticSystem::IsLeakedDeviceID(const char* deviceID)
{
    if (!deviceID || !deviceID[0]) return false;
    for (int i = 0; LEAKED_DEVICE_IDS[i] != nullptr; i++)
        if (strstr(deviceID, LEAKED_DEVICE_IDS[i]) != nullptr) return true;
    return false;
}

void DiagnosticSystem::ShowErrorPopup(const char* title, const char* message)
{
#ifdef _WIN32
    wchar_t titleW[256], messageW[512];
    mbstowcs_s(nullptr, titleW, title, 255);
    mbstowcs_s(nullptr, messageW, message, 511);
    MessageBoxW(nullptr, messageW, titleW, MB_OK | MB_ICONERROR | MB_TOPMOST);
#endif
    (void)title; (void)message;
}

void DiagnosticSystem::SelfDestruct(const char* reason)
{
    FILE* logFile = nullptr;
    fopen_s(&logFile, "zero_error.log", "w");
    if (logFile)
    {
        fprintf(logFile, "PROJECT ZERO - Self-Destruct\nReason: %s\nTime: %lld\n", reason, (long long)time(nullptr));
        fclose(logFile);
    }
    
    char message[512];
    snprintf(message, sizeof(message), "Security check failed.\n\nReason: %s\n\nProgram will close.", reason);
    ShowErrorPopup("PROJECT ZERO - Security Alert", message);
    
    DMAEngine::Shutdown();
    
#ifdef _WIN32
    ExitProcess(1);
#else
    exit(1);
#endif
}

DiagnosticResult DiagnosticSystem::CheckDeviceHandshake()
{
    g_DiagStatus.deviceHandshake = false;
    strcpy_s(g_DiagStatus.deviceName, "Unknown");
    
#if DMA_ENABLED
    char arg0[] = "";
    char arg1[] = "-device";
    char arg2[64];
    strncpy_s(arg2, g_Config.deviceType, 63);
    char* args[3] = { arg0, arg1, arg2 };
    
    VMM_HANDLE testHandle = VMMDLL_Initialize(3, args);
    if (!testHandle)
    {
        g_DiagStatus.lastError = DiagnosticResult::DEVICE_NOT_FOUND;
        strcpy_s(g_DiagStatus.errorMessage, "VMMDLL_Initialize failed");
        return DiagnosticResult::DEVICE_NOT_FOUND;
    }
    
    g_VMMDLL = testHandle;
    strcpy_s(g_DiagStatus.deviceName, g_Config.deviceType);
    g_DiagStatus.deviceHandshake = true;
    return DiagnosticResult::SUCCESS;
#else
    strcpy_s(g_DiagStatus.deviceName, "Simulation");
    g_DiagStatus.deviceHandshake = true;
    return DiagnosticResult::SUCCESS;
#endif
}

DiagnosticResult DiagnosticSystem::CheckFirmwareIntegrity()
{
    g_DiagStatus.firmwareCheck = false;
    strcpy_s(g_DiagStatus.firmwareVersion, "Unknown");
    strcpy_s(g_DiagStatus.deviceID, "Unknown");
    
#if DMA_ENABLED
    if (!g_VMMDLL)
    {
        g_DiagStatus.lastError = DiagnosticResult::DEVICE_NOT_FOUND;
        return DiagnosticResult::DEVICE_NOT_FOUND;
    }
    
    char deviceID[64];
    snprintf(deviceID, sizeof(deviceID), "%s", g_Config.deviceType);
    if (g_Config.useCustomPCIe && g_Config.customPCIeID[0])
        strncpy_s(deviceID, g_Config.customPCIeID, 63);
    
    strcpy_s(g_DiagStatus.deviceID, deviceID);
    
    if (g_Config.warnGenericID && IsGenericDeviceID(deviceID))
    {
        g_DiagStatus.isGenericID = true;
        if (g_Config.autoCloseOnFail)
        {
            g_DiagStatus.lastError = DiagnosticResult::FIRMWARE_GENERIC;
            strcpy_s(g_DiagStatus.errorMessage, "Generic FPGA ID - Detection risk");
            return DiagnosticResult::FIRMWARE_GENERIC;
        }
    }
    
    if (g_Config.warnLeakedID && IsLeakedDeviceID(deviceID))
    {
        g_DiagStatus.isLeakedID = true;
        if (g_Config.autoCloseOnFail)
        {
            g_DiagStatus.lastError = DiagnosticResult::FIRMWARE_LEAKED;
            strcpy_s(g_DiagStatus.errorMessage, "Leaked firmware ID - High risk");
            return DiagnosticResult::FIRMWARE_LEAKED;
        }
    }
    
    strcpy_s(g_DiagStatus.firmwareVersion, "OK");
    g_DiagStatus.firmwareCheck = true;
    return DiagnosticResult::SUCCESS;
#else
    strcpy_s(g_DiagStatus.firmwareVersion, "Simulation");
    strcpy_s(g_DiagStatus.deviceID, "SIM:0000");
    g_DiagStatus.firmwareCheck = true;
    return DiagnosticResult::SUCCESS;
#endif
}

float DiagnosticSystem::MeasureReadSpeed(uintptr_t testAddr, size_t size, int iterations)
{
#if DMA_ENABLED
    if (!g_VMMDLL || !g_DMA_PID || testAddr == 0) return 0;
    
    std::vector<uint8_t> buffer(size);
    auto start = std::chrono::high_resolution_clock::now();
    
    // Use scatter for speed test
    VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    if (hScatter)
    {
        for (int i = 0; i < iterations; i++)
        {
            VMMDLL_Scatter_Clear(hScatter, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
            VMMDLL_Scatter_Prepare(hScatter, testAddr, (DWORD)size);
            VMMDLL_Scatter_Execute(hScatter);
            VMMDLL_Scatter_Read(hScatter, testAddr, (DWORD)size, buffer.data(), nullptr);
        }
        VMMDLL_Scatter_CloseHandle(hScatter);
    }
    else
    {
        for (int i = 0; i < iterations; i++)
            VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, testAddr, buffer.data(), (DWORD)size, nullptr, 0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    double totalBytes = (double)size * iterations;
    double seconds = duration / 1000000.0;
    return (float)((totalBytes / (1024.0 * 1024.0)) / seconds);
#else
    (void)testAddr; (void)size; (void)iterations;
    return 100.0f;
#endif
}

int DiagnosticSystem::MeasureLatency(uintptr_t testAddr, int iterations)
{
#if DMA_ENABLED
    if (!g_VMMDLL || !g_DMA_PID || testAddr == 0) return 99999;
    
    uint64_t value;
    int64_t totalLatency = 0;
    
    for (int i = 0; i < iterations; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, testAddr, (PBYTE)&value, sizeof(value), nullptr, 0);
        auto end = std::chrono::high_resolution_clock::now();
        totalLatency += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    
    return (int)(totalLatency / iterations);
#else
    (void)testAddr; (void)iterations;
    return 100;
#endif
}

DiagnosticResult DiagnosticSystem::CheckMemorySpeed()
{
    g_DiagStatus.speedTest = false;
    
#if DMA_ENABLED
    if (!g_VMMDLL || !g_DMA_PID)
    {
        g_DiagStatus.lastError = DiagnosticResult::DEVICE_NOT_FOUND;
        return DiagnosticResult::DEVICE_NOT_FOUND;
    }
    
    uintptr_t testAddr = DMAEngine::GetBaseAddress();
    if (testAddr == 0)
    {
        g_DiagStatus.lastError = DiagnosticResult::BASE_ADDRESS_FAIL;
        return DiagnosticResult::BASE_ADDRESS_FAIL;
    }
    
    g_DiagStatus.readSpeedMBps = MeasureReadSpeed(testAddr, 4096, 100);
    g_DiagStatus.latencyUs = MeasureLatency(testAddr, 50);
    g_DiagStatus.speedSufficient = (g_DiagStatus.readSpeedMBps >= g_Config.minSpeedMBps &&
                                     g_DiagStatus.latencyUs <= g_Config.maxLatencyUs);
    
    if (!g_DiagStatus.speedSufficient && g_Config.autoCloseOnFail)
    {
        g_DiagStatus.lastError = DiagnosticResult::SPEED_TOO_SLOW;
        snprintf(g_DiagStatus.errorMessage, sizeof(g_DiagStatus.errorMessage),
                 "Speed: %.1f MB/s (min: %.1f), Latency: %d us (max: %d)",
                 g_DiagStatus.readSpeedMBps, g_Config.minSpeedMBps,
                 g_DiagStatus.latencyUs, g_Config.maxLatencyUs);
        return DiagnosticResult::SPEED_TOO_SLOW;
    }
    
    g_DiagStatus.speedTest = true;
    return DiagnosticResult::SUCCESS;
#else
    g_DiagStatus.readSpeedMBps = 150.0f;
    g_DiagStatus.latencyUs = 50;
    g_DiagStatus.speedSufficient = true;
    g_DiagStatus.speedTest = true;
    return DiagnosticResult::SUCCESS;
#endif
}

DiagnosticResult DiagnosticSystem::CheckProcessAccess()
{
    g_DiagStatus.processFound = false;
    g_DiagStatus.memoryAccess = false;
    
#if DMA_ENABLED
    if (!g_VMMDLL)
    {
        g_DiagStatus.lastError = DiagnosticResult::DEVICE_NOT_FOUND;
        return DiagnosticResult::DEVICE_NOT_FOUND;
    }
    
    if (!VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)g_Config.processName, &g_DMA_PID) || g_DMA_PID == 0)
    {
        g_DiagStatus.lastError = DiagnosticResult::PROCESS_NOT_FOUND;
        snprintf(g_DiagStatus.errorMessage, sizeof(g_DiagStatus.errorMessage),
                 "Process '%s' not found", g_Config.processName);
        return DiagnosticResult::PROCESS_NOT_FOUND;
    }
    
    g_DiagStatus.processFound = true;
    
    DMAEngine::s_BaseAddress = VMMDLL_ProcessGetModuleBase(g_VMMDLL, g_DMA_PID, (LPSTR)g_Config.processName);
    if (DMAEngine::s_BaseAddress == 0)
    {
        g_DiagStatus.lastError = DiagnosticResult::BASE_ADDRESS_FAIL;
        strcpy_s(g_DiagStatus.errorMessage, "Failed to get module base");
        return DiagnosticResult::BASE_ADDRESS_FAIL;
    }
    
    // Test read using scatter
    uint64_t testValue = 0;
    VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    if (hScatter)
    {
        VMMDLL_Scatter_Prepare(hScatter, DMAEngine::s_BaseAddress, sizeof(testValue));
        VMMDLL_Scatter_Execute(hScatter);
        VMMDLL_Scatter_Read(hScatter, DMAEngine::s_BaseAddress, sizeof(testValue), (PBYTE)&testValue, nullptr);
        VMMDLL_Scatter_CloseHandle(hScatter);
    }
    else
    {
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, DMAEngine::s_BaseAddress, (PBYTE)&testValue, sizeof(testValue), nullptr, 0);
    }
    
    g_DiagStatus.memoryAccess = true;
    return DiagnosticResult::SUCCESS;
#else
    g_DiagStatus.processFound = true;
    g_DiagStatus.memoryAccess = true;
    DMAEngine::s_BaseAddress = 0x140000000;
    return DiagnosticResult::SUCCESS;
#endif
}

bool DiagnosticSystem::RunAllDiagnostics()
{
    memset(&g_DiagStatus, 0, sizeof(g_DiagStatus));
    
    DiagnosticResult result = CheckDeviceHandshake();
    if (result != DiagnosticResult::SUCCESS)
    {
        if (g_Config.autoCloseOnFail)
        {
            ShowErrorPopup("DMA Device Not Found", 
                          "No DMA device detected.\n\n"
                          "1. Check FPGA connection\n"
                          "2. Install drivers\n"
                          "3. Close other DMA programs");
            SelfDestruct(GetErrorString(result));
        }
        return false;
    }
    
    result = CheckFirmwareIntegrity();
    if (result != DiagnosticResult::SUCCESS)
    {
        if (g_Config.autoCloseOnFail)
        {
            char msg[512];
            snprintf(msg, sizeof(msg), "Firmware check failed.\n\nDevice ID: %s\nIssue: %s",
                    g_DiagStatus.deviceID, GetErrorString(result));
            ShowErrorPopup("Firmware Warning", msg);
            SelfDestruct(GetErrorString(result));
        }
        return false;
    }
    
    result = CheckProcessAccess();
    if (result != DiagnosticResult::SUCCESS)
    {
        if (result == DiagnosticResult::PROCESS_NOT_FOUND)
        {
            DMAEngine::s_SimulationMode = true;
        }
        else if (g_Config.autoCloseOnFail)
        {
            SelfDestruct(GetErrorString(result));
        }
    }
    
    result = CheckMemorySpeed();
    if (result != DiagnosticResult::SUCCESS)
    {
        if (g_Config.autoCloseOnFail)
        {
            char msg[512];
            snprintf(msg, sizeof(msg), "DMA performance insufficient.\n\nSpeed: %.1f MB/s\nLatency: %d us",
                    g_DiagStatus.readSpeedMBps, g_DiagStatus.latencyUs);
            ShowErrorPopup("Performance Warning", msg);
            SelfDestruct(GetErrorString(result));
        }
        return false;
    }
    
    g_DiagStatus.allChecksPassed = true;
    return true;
}

// ============================================================================
// CONFIG FILE HANDLING
// ============================================================================
bool LoadConfig(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        CreateDefaultConfig(filename);
        file.open(filename);
        if (!file.is_open()) return false;
    }
    
    std::string line, section;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        
        if (line[0] == '[')
        {
            size_t end = line.find(']');
            if (end != std::string::npos) section = line.substr(1, end - 1);
            continue;
        }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0, 1);
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        if (section == "Device")
        {
            if (key == "Type") strncpy_s(g_Config.deviceType, value.c_str(), 31);
            else if (key == "Arg") strncpy_s(g_Config.deviceArg, value.c_str(), 63);
            else if (key == "Algorithm") strncpy_s(g_Config.deviceAlgo, value.c_str(), 15);
            else if (key == "UseCustomPCIe") g_Config.useCustomPCIe = (value == "1" || value == "true");
            else if (key == "CustomPCIeID") strncpy_s(g_Config.customPCIeID, value.c_str(), 31);
        }
        else if (section == "Target")
        {
            if (key == "ProcessName")
            {
                strncpy_s(g_Config.processName, value.c_str(), 63);
                mbstowcs_s(nullptr, g_Config.processNameW, g_Config.processName, 63);
            }
        }
        else if (section == "Performance")
        {
            if (key == "ScatterBatchSize") g_Config.scatterBatchSize = std::stoi(value);
            else if (key == "UpdateRateHz") g_Config.updateRateHz = std::stoi(value);
            else if (key == "UseScatterRegistry") g_Config.useScatterRegistry = (value == "1" || value == "true");
        }
        else if (section == "Diagnostics")
        {
            if (key == "EnableDiagnostics") g_Config.enableDiagnostics = (value == "1" || value == "true");
            else if (key == "AutoCloseOnFail") g_Config.autoCloseOnFail = (value == "1" || value == "true");
            else if (key == "MinSpeedMBps") g_Config.minSpeedMBps = std::stof(value);
            else if (key == "MaxLatencyUs") g_Config.maxLatencyUs = std::stoi(value);
            else if (key == "WarnGenericID") g_Config.warnGenericID = (value == "1" || value == "true");
            else if (key == "WarnLeakedID") g_Config.warnLeakedID = (value == "1" || value == "true");
        }
        else if (section == "Map")
        {
            if (key == "ImagePath") strncpy_s(g_Config.mapImagePath, value.c_str(), 255);
            else if (key == "ScaleX") g_Config.mapScaleX = std::stof(value);
            else if (key == "ScaleY") g_Config.mapScaleY = std::stof(value);
            else if (key == "OffsetX") g_Config.mapOffsetX = std::stof(value);
            else if (key == "OffsetY") g_Config.mapOffsetY = std::stof(value);
            else if (key == "Rotation") g_Config.mapRotation = std::stof(value);
        }
        else if (section == "Debug")
        {
            if (key == "DebugMode") g_Config.debugMode = (value == "1" || value == "true");
            else if (key == "LogReads") g_Config.logReads = (value == "1" || value == "true");
        }
    }
    return true;
}

bool SaveConfig(const char* filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "; PROJECT ZERO - Configuration\n\n";
    
    file << "[Device]\n";
    file << "Type=" << g_Config.deviceType << "\n";
    file << "Arg=" << g_Config.deviceArg << "\n";
    file << "Algorithm=" << g_Config.deviceAlgo << "\n";
    file << "UseCustomPCIe=" << (g_Config.useCustomPCIe ? "1" : "0") << "\n";
    file << "CustomPCIeID=" << g_Config.customPCIeID << "\n\n";
    
    file << "[Target]\n";
    file << "ProcessName=" << g_Config.processName << "\n\n";
    
    file << "[Performance]\n";
    file << "ScatterBatchSize=" << g_Config.scatterBatchSize << "\n";
    file << "UpdateRateHz=" << g_Config.updateRateHz << "\n";
    file << "UseScatterRegistry=" << (g_Config.useScatterRegistry ? "1" : "0") << "\n\n";
    
    file << "[Diagnostics]\n";
    file << "EnableDiagnostics=" << (g_Config.enableDiagnostics ? "1" : "0") << "\n";
    file << "AutoCloseOnFail=" << (g_Config.autoCloseOnFail ? "1" : "0") << "\n";
    file << "MinSpeedMBps=" << g_Config.minSpeedMBps << "\n";
    file << "MaxLatencyUs=" << g_Config.maxLatencyUs << "\n";
    file << "WarnGenericID=" << (g_Config.warnGenericID ? "1" : "0") << "\n";
    file << "WarnLeakedID=" << (g_Config.warnLeakedID ? "1" : "0") << "\n\n";
    
    file << "[Map]\n";
    file << "ImagePath=" << g_Config.mapImagePath << "\n";
    file << "ScaleX=" << g_Config.mapScaleX << "\n";
    file << "ScaleY=" << g_Config.mapScaleY << "\n";
    file << "OffsetX=" << g_Config.mapOffsetX << "\n";
    file << "OffsetY=" << g_Config.mapOffsetY << "\n";
    file << "Rotation=" << g_Config.mapRotation << "\n\n";
    
    file << "[Debug]\n";
    file << "DebugMode=" << (g_Config.debugMode ? "1" : "0") << "\n";
    file << "LogReads=" << (g_Config.logReads ? "1" : "0") << "\n";
    
    return true;
}

void CreateDefaultConfig(const char* filename)
{
    strcpy_s(g_Config.deviceType, "fpga");
    strcpy_s(g_Config.deviceArg, "");
    strcpy_s(g_Config.deviceAlgo, "0");
    g_Config.useCustomPCIe = false;
    strcpy_s(g_Config.customPCIeID, "");
    
    strcpy_s(g_Config.processName, "cod.exe");
    wcscpy_s(g_Config.processNameW, L"cod.exe");
    
    g_Config.scatterBatchSize = 128;
    g_Config.updateRateHz = 120;
    g_Config.useScatterRegistry = true;
    
    g_Config.enableDiagnostics = true;
    g_Config.autoCloseOnFail = true;
    g_Config.minSpeedMBps = 50.0f;
    g_Config.maxLatencyUs = 5000;
    g_Config.warnGenericID = true;
    g_Config.warnLeakedID = true;
    
    strcpy_s(g_Config.mapImagePath, "");
    g_Config.mapScaleX = 1.0f;
    g_Config.mapScaleY = 1.0f;
    g_Config.mapOffsetX = 0.0f;
    g_Config.mapOffsetY = 0.0f;
    g_Config.mapRotation = 0.0f;
    
    g_Config.debugMode = false;
    g_Config.logReads = false;
    
    SaveConfig(filename);
}

// ============================================================================
// SCATTER READ REGISTRY - HIGH PERFORMANCE
// ============================================================================
void ScatterReadRegistry::RegisterPlayerData(int playerIndex, uintptr_t baseAddr)
{
    if (playerIndex < 0 || baseAddr == 0) return;
    
    while ((int)m_PlayerBuffers.size() <= playerIndex)
        m_PlayerBuffers.push_back({});
    
    PlayerRawData& buf = m_PlayerBuffers[playerIndex];
    
    m_Entries.push_back({baseAddr + GameOffsets::EntityPos, &buf.position, sizeof(Vec3), ScatterDataType::PLAYER_POSITION, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityHealth, &buf.health, sizeof(int), ScatterDataType::PLAYER_HEALTH, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityMaxHealth, &buf.maxHealth, sizeof(int), ScatterDataType::PLAYER_HEALTH, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityTeam, &buf.team, sizeof(int), ScatterDataType::PLAYER_TEAM, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityYaw, &buf.yaw, sizeof(float), ScatterDataType::PLAYER_YAW, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityValid, &buf.valid, sizeof(uint8_t), ScatterDataType::PLAYER_VALID, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityStance, &buf.stance, sizeof(uint8_t), ScatterDataType::PLAYER_STANCE, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityName, &buf.name, 32, ScatterDataType::PLAYER_NAME, playerIndex});
}

void ScatterReadRegistry::RegisterLocalPlayer(uintptr_t baseAddr)
{
    if (baseAddr == 0) return;
    m_Entries.push_back({baseAddr + GameOffsets::EntityPos, &m_LocalPosition, sizeof(Vec3), ScatterDataType::LOCAL_POSITION, -1});
    m_Entries.push_back({baseAddr + GameOffsets::EntityYaw, &m_LocalYaw, sizeof(float), ScatterDataType::LOCAL_YAW, -1});
    m_Entries.push_back({baseAddr + GameOffsets::EntityTeam, &m_LocalTeam, sizeof(int), ScatterDataType::PLAYER_TEAM, -1});
}

void ScatterReadRegistry::RegisterViewMatrix(uintptr_t addr)
{
    if (addr == 0) return;
    m_Entries.push_back({addr, &m_ViewMatrix, sizeof(Matrix4x4), ScatterDataType::VIEW_MATRIX, -1});
}

void ScatterReadRegistry::RegisterCustomRead(uintptr_t addr, void* buf, size_t size)
{
    if (addr == 0 || buf == nullptr || size == 0) return;
    m_Entries.push_back({addr, buf, size, ScatterDataType::CUSTOM, -1});
}

void ScatterReadRegistry::ExecuteAll()
{
    if (m_Entries.empty()) return;
    m_TransactionCount++;
    
    // Use high-performance scatter function
    ExecuteScatterReads(m_Entries);
    
    // Copy data to PlayerManager
    if (DMAEngine::IsOnline() || DMAEngine::IsConnected())
    {
        std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
        auto& players = PlayerManager::GetPlayers();
        auto& local = PlayerManager::GetLocalPlayer();
        
        local.origin = m_LocalPosition;
        local.yaw = m_LocalYaw;
        local.team = m_LocalTeam;
        
        for (size_t i = 0; i < m_PlayerBuffers.size() && i < players.size(); i++)
        {
            PlayerRawData& raw = m_PlayerBuffers[i];
            PlayerData& p = players[i];
            
            p.origin = raw.position;
            p.health = raw.health;
            p.maxHealth = raw.maxHealth > 0 ? raw.maxHealth : 100;
            p.team = raw.team;
            p.yaw = raw.yaw;
            p.valid = (raw.valid != 0 && raw.health > 0);
            p.stance = raw.stance;
            p.isAlive = (raw.health > 0);
            p.isEnemy = (raw.team != m_LocalTeam);
            p.distance = (p.origin - m_LocalPosition).Length();
            
            if (raw.name[0] != 0)
                strncpy_s(p.name, raw.name, 31);
        }
    }
}

void ScatterReadRegistry::Clear() { m_Entries.clear(); }

int ScatterReadRegistry::GetTotalBytes() const
{
    int total = 0;
    for (const auto& e : m_Entries) total += (int)e.size;
    return total;
}

// ============================================================================
// MAP TEXTURE MANAGER
// ============================================================================
void MapTextureManager::InitializeMapDatabase()
{
    MapInfo nuketown;
    strcpy_s(nuketown.name, "Nuketown");
    nuketown.minX = -2000.0f; nuketown.maxX = 2000.0f;
    nuketown.minY = -2000.0f; nuketown.maxY = 2000.0f;
    s_MapDatabase["mp_nuketown"] = nuketown;
}

bool MapTextureManager::LoadMapConfig(const char* mapName)
{
    auto it = s_MapDatabase.find(mapName);
    if (it != s_MapDatabase.end()) { s_CurrentMap = it->second; return true; }
    strcpy_s(s_CurrentMap.name, mapName);
    return false;
}

bool MapTextureManager::LoadMapTexture(const char* imagePath)
{
    if (imagePath && imagePath[0])
    {
        strcpy_s(s_CurrentMap.imagePath, imagePath);
        s_CurrentMap.hasTexture = true;
        return true;
    }
    s_CurrentMap.hasTexture = false;
    return false;
}

Vec2 MapTextureManager::GameToMapCoords(const Vec3& gamePos)
{
    MapInfo& map = s_CurrentMap;
    float nx = (gamePos.x - map.minX) / (map.maxX - map.minX);
    float ny = (gamePos.y - map.minY) / (map.maxY - map.minY);
    return Vec2(nx * map.imageWidth, (1.0f - ny) * map.imageHeight);
}

Vec2 MapTextureManager::GameToRadarCoords(const Vec3& gamePos, const Vec3& localPos, float localYaw,
                                           float radarCX, float radarCY, float radarSize, float zoom)
{
    Vec3 delta = gamePos - localPos;
    float yawRad = -localYaw * 3.14159265f / 180.0f;
    float rotX = delta.x * cosf(yawRad) - delta.y * sinf(yawRad);
    float rotY = delta.x * sinf(yawRad) + delta.y * cosf(yawRad);
    float scale = (radarSize * 0.4f) / (100.0f / zoom);
    return Vec2(radarCX + rotX * scale, radarCY - rotY * scale);
}

void MapTextureManager::SetMapBounds(const char* mapName, float minX, float maxX, float minY, float maxY)
{
    MapInfo info;
    strcpy_s(info.name, mapName);
    info.minX = minX; info.maxX = maxX;
    info.minY = minY; info.maxY = maxY;
    s_MapDatabase[mapName] = info;
}

// ============================================================================
// DMA ENGINE
// ============================================================================
bool DMAEngine::Initialize()
{
    LoadConfig("zero.ini");
    return InitializeWithConfig(g_Config);
}

bool DMAEngine::InitializeWithConfig(const DMAConfig& config)
{
    strcpy_s(s_StatusText, "INITIALIZING...");
    s_Online = false;
    
    if (config.enableDiagnostics)
    {
        strcpy_s(s_StatusText, "DIAGNOSTICS...");
        if (!DiagnosticSystem::RunAllDiagnostics())
        {
            if (!s_SimulationMode)
            {
                strcpy_s(s_StatusText, "DIAG FAILED");
                strcpy_s(s_DeviceInfo, g_DiagStatus.errorMessage);
                return false;
            }
        }
    }
    
#if DMA_ENABLED
    if (g_VMMDLL && g_DMA_PID)
    {
        s_Connected = true;
        s_SimulationMode = false;
        s_Online = g_DiagStatus.allChecksPassed;
        s_ModuleSize = 0x5000000;
        
        strcpy_s(s_StatusText, s_Online ? "ONLINE" : "LIMITED");
        snprintf(s_DeviceInfo, sizeof(s_DeviceInfo), "SCATTER MODE | %.1f MB/s | %d us",
                 g_DiagStatus.readSpeedMBps, g_DiagStatus.latencyUs);
        
        PatternScanner::UpdateAllOffsets();
        MapTextureManager::InitializeMapDatabase();
        return true;
    }
#endif
    
    s_Connected = false;
    s_SimulationMode = true;
    s_Online = false;
    s_BaseAddress = 0x140000000;
    s_ModuleSize = 0x5000000;
    strcpy_s(s_StatusText, "SIMULATION");
    strcpy_s(s_DeviceInfo, "Demo mode - No DMA");
    
    g_Offsets.PlayerBase = s_BaseAddress + 0x17AA8E98;
    g_Offsets.ClientInfo = s_BaseAddress + 0x17AA9000;
    g_Offsets.EntityList = s_BaseAddress + 0x16D5B8D8;
    
    MapTextureManager::InitializeMapDatabase();
    return true;
}

void DMAEngine::Shutdown()
{
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        VMMDLL_Close(g_VMMDLL);
        g_VMMDLL = nullptr;
    }
#endif
    SaveConfig("zero.ini");
    s_Connected = false;
    s_Online = false;
    strcpy_s(s_StatusText, "OFFLINE");
}

bool DMAEngine::IsConnected() { return s_Connected && !s_SimulationMode; }
bool DMAEngine::IsOnline() { return s_Online; }
const char* DMAEngine::GetStatus() { return s_StatusText; }
const char* DMAEngine::GetDeviceInfo() { return s_DeviceInfo; }

template<typename T>
T DMAEngine::Read(uintptr_t address)
{
    T value = {};
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)&value, sizeof(T), nullptr, VMMDLL_FLAG_NOCACHE);
#else
    (void)address;
#endif
    return value;
}

bool DMAEngine::ReadBuffer(uintptr_t address, void* buffer, size_t size)
{
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
        return VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)buffer, (DWORD)size, nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    (void)address; (void)buffer; (void)size;
    return false;
}

bool DMAEngine::ReadString(uintptr_t address, char* buffer, size_t maxLen)
{
    if (!buffer || maxLen == 0) return false;
    buffer[0] = 0;
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
    {
        if (VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)buffer, (DWORD)(maxLen - 1), nullptr, VMMDLL_FLAG_NOCACHE))
        {
            buffer[maxLen - 1] = 0;
            return true;
        }
    }
#endif
    (void)address;
    return false;
}

void DMAEngine::ExecuteScatter(std::vector<ScatterEntry>& entries)
{
    ExecuteScatterReads(entries);
}

uintptr_t DMAEngine::GetBaseAddress() { return s_BaseAddress; }

uintptr_t DMAEngine::GetModuleBase(const wchar_t* moduleName)
{
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
    {
        char moduleNameA[256];
        wcstombs_s(nullptr, moduleNameA, moduleName, 255);
        return VMMDLL_ProcessGetModuleBase(g_VMMDLL, g_DMA_PID, moduleNameA);
    }
#endif
    (void)moduleName;
    return s_BaseAddress;
}

size_t DMAEngine::GetModuleSize() { return s_ModuleSize; }

// Template instantiations
template int32_t DMAEngine::Read<int32_t>(uintptr_t);
template uint32_t DMAEngine::Read<uint32_t>(uintptr_t);
template int64_t DMAEngine::Read<int64_t>(uintptr_t);
template uint64_t DMAEngine::Read<uint64_t>(uintptr_t);
template float DMAEngine::Read<float>(uintptr_t);
template uintptr_t DMAEngine::Read<uintptr_t>(uintptr_t);
template Vec2 DMAEngine::Read<Vec2>(uintptr_t);
template Vec3 DMAEngine::Read<Vec3>(uintptr_t);

// ============================================================================
// PATTERN SCANNER
// ============================================================================
uintptr_t PatternScanner::FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask)
{
    size_t patternLen = strlen(mask);
    if (patternLen == 0 || size < patternLen) return 0;
    
#if DMA_ENABLED
    if (DMAEngine::IsConnected())
    {
        const size_t chunkSize = 0x10000;
        std::vector<uint8_t> buffer(chunkSize + patternLen);
        
        for (size_t offset = 0; offset < size; offset += chunkSize)
        {
            size_t readSize = (std::min)(chunkSize + patternLen, size - offset);
            if (!DMAEngine::ReadBuffer(start + offset, buffer.data(), readSize)) continue;
            
            for (size_t i = 0; i < readSize - patternLen; i++)
            {
                bool found = true;
                for (size_t j = 0; j < patternLen && found; j++)
                    if (mask[j] == 'x' && buffer[i + j] != (uint8_t)pattern[j]) found = false;
                if (found) return start + offset + i;
            }
        }
    }
#endif
    (void)start; (void)size; (void)pattern; (void)mask;
    return 0;
}

uintptr_t PatternScanner::ScanModule(const char* moduleName, const char* pattern, const char* mask)
{
    (void)moduleName;
    return FindPattern(DMAEngine::GetBaseAddress(), DMAEngine::GetModuleSize(), pattern, mask);
}

uintptr_t PatternScanner::ResolveRelative(uintptr_t instructionAddr, int offset, int instructionSize)
{
#if DMA_ENABLED
    if (DMAEngine::IsConnected())
    {
        int32_t relOffset = DMAEngine::Read<int32_t>(instructionAddr + offset);
        return instructionAddr + instructionSize + relOffset;
    }
#endif
    (void)instructionAddr; (void)offset; (void)instructionSize;
    return 0;
}

bool PatternScanner::UpdateAllOffsets()
{
    s_FoundCount = 0;
    s_Scanned = true;
    return s_FoundCount > 0;
}

// ============================================================================
// PLAYER MANAGER
// ============================================================================
void PlayerManager::Initialize()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Players.clear();
    s_Players.reserve(150);
    
    for (int i = 0; i < 12; i++)
    {
        PlayerData p = {};
        p.valid = true;
        p.index = i;
        p.isEnemy = (i < 6);
        p.isAlive = true;
        p.health = 50 + rand() % 50;
        p.maxHealth = 100;
        p.team = p.isEnemy ? 1 : 2;
        sprintf_s(p.name, "Player_%d", i + 1);
        
        float angle = (float)i * (6.28318f / 12.0f);
        float dist = 30.0f + (float)(rand() % 100);
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.yaw = (float)(rand() % 360);
        p.distance = dist;
        
        s_Players.push_back(p);
    }
    
    s_LocalPlayer = {};
    s_LocalPlayer.valid = true;
    s_LocalPlayer.health = 100;
    s_LocalPlayer.maxHealth = 100;
    strcpy_s(s_LocalPlayer.name, "LocalPlayer");
    
    s_Initialized = true;
}

void PlayerManager::Update()
{
    if (!s_Initialized) Initialize();
    if (g_Config.useScatterRegistry) UpdateWithScatterRegistry();
    else SimulateUpdate();
}

void PlayerManager::UpdateWithScatterRegistry()
{
    if (!s_Initialized) Initialize();
    
#if DMA_ENABLED
    if ((DMAEngine::IsOnline() || DMAEngine::IsConnected()) && g_Offsets.EntityList)
    {
        g_ScatterRegistry.Clear();
        
        // Register all player data for batch read
        for (size_t i = 0; i < s_Players.size(); i++)
            g_ScatterRegistry.RegisterPlayerData((int)i, g_Offsets.EntityList + i * GameOffsets::EntitySize);
        
        // Register local player
        if (g_Offsets.ClientInfo)
        {
            uintptr_t clientBase = DMAEngine::Read<uintptr_t>(g_Offsets.ClientInfo);
            if (clientBase)
                g_ScatterRegistry.RegisterLocalPlayer(clientBase);
        }
        
        // Execute ALL reads in ONE DMA transaction
        g_ScatterRegistry.ExecuteAll();
        return;
    }
#endif
    SimulateUpdate();
}

void PlayerManager::SimulateUpdate()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_SimTime += 1.0f / (float)g_Config.updateRateHz;
    
    s_LocalPlayer.yaw = fmodf(s_SimTime * 15.0f, 360.0f);
    
    for (size_t i = 0; i < s_Players.size(); i++)
    {
        PlayerData& p = s_Players[i];
        float angle = (float)i * (6.28318f / (float)s_Players.size()) + s_SimTime * 0.3f;
        float dist = 40.0f + sinf(s_SimTime * 0.5f + (float)i) * 30.0f;
        
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.yaw = fmodf(angle * 57.3f + 180.0f, 360.0f);
        p.distance = dist;
        p.health = 30 + (int)(sinf(s_SimTime * 0.2f + (float)i) * 35 + 35);
    }
}

void PlayerManager::RealUpdate() { UpdateWithScatterRegistry(); }
std::vector<PlayerData>& PlayerManager::GetPlayers() { return s_Players; }
PlayerData& PlayerManager::GetLocalPlayer() { return s_LocalPlayer; }

int PlayerManager::GetAliveCount()
{
    int count = 0;
    for (const auto& p : s_Players) if (p.valid && p.isAlive) count++;
    return count;
}

int PlayerManager::GetEnemyCount()
{
    int count = 0;
    for (const auto& p : s_Players) if (p.valid && p.isAlive && p.isEnemy) count++;
    return count;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
bool WorldToScreen(const Vec3& worldPos, Vec2& screenPos, int screenW, int screenH)
{
    (void)worldPos;
    screenPos = Vec2((float)screenW / 2, (float)screenH / 2);
    return true;
}

bool WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float localYaw,
                  Vec2& radarPos, float radarCX, float radarCY, float radarScale)
{
    radarPos = MapTextureManager::GameToRadarCoords(worldPos, localPos, localYaw, radarCX, radarCY, radarScale * 2, 1.0f);
    return true;
}

float GetFOVTo(const Vec2& screenCenter, const Vec2& targetScreen)
{
    float dx = targetScreen.x - screenCenter.x;
    float dy = targetScreen.y - screenCenter.y;
    return sqrtf(dx * dx + dy * dy);
}

Vec3 CalcAngle(const Vec3& src, const Vec3& dst)
{
    Vec3 delta = dst - src;
    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y);
    return Vec3(-atan2f(delta.z, hyp) * 57.3f, atan2f(delta.y, delta.x) * 57.3f, 0);
}

void SmoothAngle(Vec3& currentAngle, const Vec3& targetAngle, float smoothness)
{
    if (smoothness <= 0) smoothness = 1;
    Vec3 delta = targetAngle - currentAngle;
    while (delta.y > 180.0f) delta.y -= 360.0f;
    while (delta.y < -180.0f) delta.y += 360.0f;
    currentAngle.x += delta.x / smoothness;
    currentAngle.y += delta.y / smoothness;
}
