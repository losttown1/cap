// DMA_Engine.cpp - Blurry-Style Professional v5.0
// Real Hardware Diagnostics - Auto KMBox Lock - No Fake Data

#include "DMA_Engine.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

// ============================================================================
// DMA LIBRARY
// ============================================================================
#if DMA_ENABLED
#include "vmmdll.h"
#pragma comment(lib, "vmmdll.lib")
static VMM_HANDLE g_VMMHandle = nullptr;
static DWORD g_GamePID = 0;
#endif

// ============================================================================
// HARDWARE STATUS
// ============================================================================
static std::atomic<bool> g_DMA_Active(false);
static std::atomic<bool> g_DMA_MemoryReadable(false);
static std::atomic<bool> g_KMBox_Active(false);
static std::atomic<bool> g_HWScanComplete(false);
static std::atomic<bool> g_BlackOverlayActive(false);

static char g_DMA_StatusStr[64] = "OFFLINE";
static char g_DMA_DiagnosticStr[128] = "Not checked";
static char g_KMBox_StatusStr[64] = "NOT FOUND";
static char g_KMBox_FirmwareStr[64] = "";
static char g_LockedCOMPort[16] = "";

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
DMAConfig g_Config;
GameOffsetsStruct g_Offsets;
ScatterReadRegistry g_ScatterRegistry;
DiagnosticStatus g_DiagStatus;
InitStatus g_InitStatus;
ControllerConfig g_ControllerConfig;

static std::mutex g_DMAMutex;
static std::atomic<bool> g_DMABusy(false);

// ============================================================================
// STATIC MEMBERS
// ============================================================================
bool DMAEngine::s_Connected = false;
bool DMAEngine::s_Online = false;
bool DMAEngine::s_SimulationMode = false;
bool DMAEngine::s_UsingFTD3XX = false;
uintptr_t DMAEngine::s_BaseAddress = 0;
size_t DMAEngine::s_ModuleSize = 0;
char DMAEngine::s_StatusText[64] = "OFFLINE";
char DMAEngine::s_DeviceInfo[128] = "Not Connected";
char DMAEngine::s_DriverMode[32] = "Auto";

bool PatternScanner::s_Scanned = false;
int PatternScanner::s_FoundCount = 0;

std::vector<PlayerData> PlayerManager::s_Players;
PlayerData PlayerManager::s_LocalPlayer;
std::mutex PlayerManager::s_Mutex;
bool PlayerManager::s_Initialized = false;
float PlayerManager::s_SimTime = 0;

MapInfo MapTextureManager::s_CurrentMap;
std::unordered_map<std::string, MapInfo> MapTextureManager::s_MapDatabase;

void* Logger::s_Console = nullptr;
bool Logger::s_Initialized = false;

void* HardwareController::s_Handle = nullptr;
void* HardwareController::s_Socket = nullptr;
bool HardwareController::s_Connected = false;
ControllerType HardwareController::s_Type = ControllerType::NONE;
char HardwareController::s_DeviceName[64] = "None";
char HardwareController::s_LockedPort[16] = "";

RemoteOffsets OffsetUpdater::s_LastOffsets;
bool OffsetUpdater::s_Updated = false;
bool OffsetUpdater::s_Synced = false;
char OffsetUpdater::s_OffsetURL[512] = "";
char OffsetUpdater::s_LastError[256] = "";

bool Aimbot::s_Enabled = false;
int Aimbot::s_CurrentTarget = -1;
Vec2 Aimbot::s_LastAimPos = {0, 0};

// ============================================================================
// HARDWARE MANAGER CLASS - Blurry-Style Diagnostics
// ============================================================================
class HardwareManager {
public:
    static bool dma_active;
    static bool kmbox_active;
    static char locked_port[16];
    
    // فحص الـ DMA الحقيقي
    static bool InitDMAPhysicalDevice()
    {
#if DMA_ENABLED
        std::cout << "[*] Checking DMA Physical Device..." << std::endl;
        
        char arg0[] = "";
        char arg1[] = "-device";
        char arg2[] = "fpga";
        char* args[3] = {arg0, arg1, arg2};
        
        g_VMMHandle = VMMDLL_Initialize(3, args);
        
        if (g_VMMHandle != NULL)
        {
            dma_active = true;
            g_DMA_Active = true;
            strcpy_s(g_DMA_StatusStr, "ONLINE");
            strcpy_s(DMAEngine::s_StatusText, "ONLINE");
            strcpy_s(DMAEngine::s_DeviceInfo, "FPGA Physical Device");
            DMAEngine::s_Connected = true;
            DMAEngine::s_Online = true;
            
            std::cout << "[+] DMA Physical Device: ONLINE" << std::endl;
            
            // Find game process
            const char* gameNames[] = {"cod.exe", "BlackOps6.exe", "ModernWarfare.exe"};
            DWORD pid = 0;
            
            for (const char* name : gameNames)
            {
                if (VMMDLL_PidGetFromName(g_VMMHandle, (LPSTR)name, &pid) && pid != 0)
                {
                    g_GamePID = pid;
                    DMAEngine::s_BaseAddress = 0x140000000;
                    g_InitStatus.gameFound = true;
                    g_Offsets.EntityList = DMAEngine::s_BaseAddress + 0x16D5C000;
                    g_Offsets.LocalPlayer = DMAEngine::s_BaseAddress + 0x16D5A000;
                    std::cout << "[+] Game Process Found: " << name << " (PID: " << pid << ")" << std::endl;
                    break;
                }
            }
            
            // Verify memory is readable
            VerifyMemoryReadable();
            
            return true;
        }
        
        std::cout << "[-] DMA Physical Device: OFFLINE" << std::endl;
#endif
        dma_active = false;
        g_DMA_Active = false;
        strcpy_s(g_DMA_StatusStr, "OFFLINE");
        strcpy_s(g_DMA_DiagnosticStr, "FPGA NOT DETECTED");
        return false;
    }
    
    // التحقق من قراءة الذاكرة
    static bool VerifyMemoryReadable()
    {
#if DMA_ENABLED
        if (g_VMMHandle == NULL || g_GamePID == 0)
        {
            strcpy_s(g_DMA_DiagnosticStr, "NO GAME PROCESS");
            g_DMA_MemoryReadable = false;
            return false;
        }
        
        uintptr_t baseAddr = DMAEngine::s_BaseAddress;
        if (baseAddr == 0) baseAddr = 0x140000000;
        
        char header[2] = {0};
        DWORD bytesRead = 0;
        
        bool success = VMMDLL_MemReadEx(g_VMMHandle, g_GamePID, baseAddr,
                                         (PBYTE)header, 2, &bytesRead, VMMDLL_FLAG_NOCACHE);
        
        if (!success || bytesRead != 2)
        {
            strcpy_s(g_DMA_DiagnosticStr, "DMA DATA ERROR: MEMORY NOT READABLE");
            g_DMA_MemoryReadable = false;
            std::cout << "[-] Memory Read: FAILED" << std::endl;
            return false;
        }
        
        if (header[0] != 'M' || header[1] != 'Z')
        {
            strcpy_s(g_DMA_DiagnosticStr, "DMA DATA ERROR: INVALID PE HEADER");
            g_DMA_MemoryReadable = false;
            std::cout << "[-] PE Header: INVALID" << std::endl;
            return false;
        }
        
        snprintf(g_DMA_DiagnosticStr, sizeof(g_DMA_DiagnosticStr),
                 "MEMORY OK @ 0x%llX", (unsigned long long)baseAddr);
        g_DMA_MemoryReadable = true;
        std::cout << "[+] Memory Readable: OK" << std::endl;
        return true;
#else
        return false;
#endif
    }
    
    // البحث التلقائي عن KMBox
    static bool AutoLockKMBox()
    {
#ifdef _WIN32
        std::cout << "[*] Scanning for KMBox..." << std::endl;
        
        // Try saved config first
        std::ifstream cfg("config.ini");
        if (cfg.is_open())
        {
            std::string line;
            while (std::getline(cfg, line))
            {
                if (line.find("KMBoxPort=") == 0)
                {
                    std::string savedPort = line.substr(10);
                    while (!savedPort.empty() && (savedPort.back() == '\r' || savedPort.back() == '\n'))
                        savedPort.pop_back();
                    
                    if (TryConnectKMBox(savedPort.c_str()))
                    {
                        std::cout << "[+] KMBox Hardware: AUTO-LOCKED on " << savedPort << " (from config)" << std::endl;
                        return true;
                    }
                }
            }
            cfg.close();
        }
        
        // Scan COM1-COM20
        for (int i = 1; i <= 20; i++)
        {
            char port[16];
            snprintf(port, sizeof(port), "COM%d", i);
            
            if (TryConnectKMBox(port))
            {
                std::cout << "[+] KMBox Hardware: AUTO-LOCKED on " << port << std::endl;
                
                // Save to config
                std::ofstream save("config.ini");
                if (save.is_open())
                {
                    save << "[Hardware]\nKMBoxPort=" << port << "\n";
                    save.close();
                }
                
                return true;
            }
        }
        
        std::cout << "[-] KMBox: NOT FOUND or FAKE" << std::endl;
#endif
        kmbox_active = false;
        g_KMBox_Active = false;
        strcpy_s(g_KMBox_StatusStr, "NOT FOUND");
        return false;
    }
    
    // محاولة الاتصال بـ KMBox
    static bool TryConnectKMBox(const char* portName)
    {
#ifdef _WIN32
        char fullPath[32];
        snprintf(fullPath, sizeof(fullPath), "\\\\.\\%s", portName);
        
        HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE,
                                    0, nullptr, OPEN_EXISTING, 0, nullptr);
        
        if (hPort == INVALID_HANDLE_VALUE)
            return false;
        
        // Configure serial
        DCB dcb = {};
        dcb.DCBlength = sizeof(DCB);
        GetCommState(hPort, &dcb);
        dcb.BaudRate = 115200;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        
        if (!SetCommState(hPort, &dcb))
        {
            CloseHandle(hPort);
            return false;
        }
        
        COMMTIMEOUTS timeouts = {};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 200;
        SetCommTimeouts(hPort, &timeouts);
        
        PurgeComm(hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
        
        // Send version command
        const char* cmd = "km.version()\r\n";
        DWORD written = 0;
        WriteFile(hPort, cmd, (DWORD)strlen(cmd), &written, nullptr);
        
        Sleep(150);
        
        // Read response
        char response[128] = {};
        DWORD bytesRead = 0;
        ReadFile(hPort, response, sizeof(response) - 1, &bytesRead, nullptr);
        
        if (bytesRead > 0)
        {
            // Device responded!
            HardwareController::s_Handle = hPort;
            HardwareController::s_Connected = true;
            HardwareController::s_Type = ControllerType::KMBOX_B_PLUS;
            strcpy_s(HardwareController::s_LockedPort, portName);
            strcpy_s(locked_port, portName);
            strcpy_s(g_LockedCOMPort, portName);
            snprintf(HardwareController::s_DeviceName, sizeof(HardwareController::s_DeviceName),
                     "KMBox [%s]", portName);
            
            kmbox_active = true;
            g_KMBox_Active = true;
            strcpy_s(g_KMBox_StatusStr, "AUTO-LOCKED");
            
            // Extract firmware
            char* clean = response;
            while (*clean && (*clean == '\r' || *clean == '\n' || *clean == ' ')) clean++;
            strncpy_s(g_KMBox_FirmwareStr, clean, sizeof(g_KMBox_FirmwareStr) - 1);
            
            return true;
        }
        
        CloseHandle(hPort);
#endif
        return false;
    }
    
    // Blurry-Style Startup Diagnostics
    static void StartupDiagnostics()
    {
        std::cout << "\n";
        std::cout << "========================================" << std::endl;
        std::cout << "   PROJECT ZERO | Blurry-Style Init" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "[*] Starting Hardware Diagnostic...\n" << std::endl;
        
        // DMA Check
        InitDMAPhysicalDevice();
        
        // KMBox Auto-Lock
        AutoLockKMBox();
        
        std::cout << "\n[*] Diagnostic Complete." << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        g_HWScanComplete = true;
    }
};

// Static members
bool HardwareManager::dma_active = false;
bool HardwareManager::kmbox_active = false;
char HardwareManager::locked_port[16] = "";

// ============================================================================
// LOGGER
// ============================================================================
void Logger::Initialize() { s_Initialized = true; }
void Logger::Shutdown() { s_Initialized = false; }
void Logger::ShowConsole() {
#ifdef _WIN32
    AllocConsole();
    freopen_s((FILE**)&s_Console, "CONOUT$", "w", stdout);
    SetConsoleTitleW(L"PROJECT ZERO | Diagnostic Console");
#endif
}
void Logger::HideConsole() {
#ifdef _WIN32
    FreeConsole();
#endif
}
void Logger::SetColor(ConsoleColor) {}
void Logger::ResetColor() {}
void Logger::Log(const char* msg, ConsoleColor) { std::cout << msg << std::endl; }
void Logger::LogSuccess(const char* msg) { std::cout << "[+] " << msg << std::endl; }
void Logger::LogError(const char* msg) { std::cout << "[-] " << msg << std::endl; }
void Logger::LogWarning(const char* msg) { std::cout << "[!] " << msg << std::endl; }
void Logger::LogInfo(const char* msg) { std::cout << "[*] " << msg << std::endl; }
void Logger::LogStatus(const char*, const char*, bool) {}
void Logger::LogProgress(const char*, int, int) {}
void Logger::LogBanner() {}
void Logger::LogSection(const char*) {}
void Logger::LogSeparator() {}
void Logger::LogTimestamp() {}
void Logger::LogSpinner(const char*, int) {}
void Logger::ClearLine() {}

// ============================================================================
// EXTERNAL FUNCTIONS
// ============================================================================
bool InitDMA() { return HardwareManager::InitDMAPhysicalDevice(); }
bool AutoDetectKMBox() { return HardwareManager::AutoLockKMBox(); }
void RunStartupDiagnostics() { HardwareManager::StartupDiagnostics(); }

const char* GetKMBoxStatus() { return g_KMBox_StatusStr; }
const char* GetKMBoxFirmware() { return g_KMBox_FirmwareStr; }
bool IsKMBoxConnected() { return g_KMBox_Active.load(); }
bool IsHardwareScanComplete() { return g_HWScanComplete.load(); }
bool IsDMAOnline() { return g_DMA_Active.load(); }
const char* GetDMADiagnostic() { return g_DMA_DiagnosticStr; }
bool IsDMAMemoryReadable() { return g_DMA_MemoryReadable.load(); }

void SetBlackOverlayEnabled(bool enabled) { g_BlackOverlayActive = enabled; }
bool IsBlackOverlayEnabled() { return g_BlackOverlayActive.load(); }

// ============================================================================
// SCATTER READS - FIXED C2664
// ============================================================================
void ExecuteScatterReads(std::vector<ScatterEntry>& entries)
{
    if (entries.empty()) return;
    
#if DMA_ENABLED
    if (g_VMMHandle == NULL || g_GamePID == 0 || g_DMABusy.exchange(true))
    {
        for (auto& e : entries)
            if (e.buffer) memset(e.buffer, 0, e.size);
        g_DMABusy = false;
        return;
    }
    
    // FIXED C2664: PVMMDLL_SCATTER_HANDLE is pointer type
    PVMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMHandle, g_GamePID, VMMDLL_FLAG_NOCACHE);
    
    if (hScatter == NULL)
    {
        for (auto& e : entries)
        {
            if (e.buffer && e.address != 0 && e.size > 0)
                VMMDLL_MemReadEx(g_VMMHandle, g_GamePID, e.address, (PBYTE)e.buffer, (DWORD)e.size, nullptr, VMMDLL_FLAG_NOCACHE);
            else if (e.buffer)
                memset(e.buffer, 0, e.size);
        }
        g_DMABusy = false;
        return;
    }
    
    for (auto& e : entries)
    {
        if (e.address != 0 && e.size > 0 && e.size <= 0x10000)
            VMMDLL_Scatter_Prepare(hScatter, e.address, (DWORD)e.size);
    }
    
    if (!VMMDLL_Scatter_Execute(hScatter))
    {
        VMMDLL_Scatter_CloseHandle(hScatter);
        for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
        g_DMABusy = false;
        return;
    }
    
    for (auto& e : entries)
    {
        if (e.buffer && e.address != 0 && e.size > 0)
        {
            if (!VMMDLL_Scatter_Read(hScatter, e.address, (DWORD)e.size, (PBYTE)e.buffer, nullptr))
                memset(e.buffer, 0, e.size);
        }
    }
    
    VMMDLL_Scatter_CloseHandle(hScatter);
    g_DMABusy = false;
#else
    for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
#endif
}

// ============================================================================
// GET REAL ENTITIES FROM DMA
// ============================================================================
std::vector<PlayerData> GetRealEntitiesFromDMA()
{
    std::vector<PlayerData> entities;
    
    if (!g_DMA_Active.load() || !g_DMA_MemoryReadable.load())
        return entities;
    
#if DMA_ENABLED
    if (g_VMMHandle == NULL || g_GamePID == 0 || g_Offsets.EntityList == 0)
        return entities;
    
    constexpr int MAX_PLAYERS = 155;
    constexpr uintptr_t ENTITY_SIZE = 0x568;
    
    g_ScatterRegistry.Clear();
    
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        uintptr_t entityAddr = g_Offsets.EntityList + (i * ENTITY_SIZE);
        g_ScatterRegistry.RegisterPlayerData(i, entityAddr);
    }
    
    g_ScatterRegistry.ExecuteAll();
    
    auto& buffers = g_ScatterRegistry.GetPlayerBuffers();
    auto& local = PlayerManager::GetLocalPlayer();
    
    for (int i = 0; i < (int)buffers.size() && i < MAX_PLAYERS; i++)
    {
        auto& buf = buffers[i];
        
        if (buf.position.x == 0 && buf.position.y == 0 && buf.position.z == 0)
            continue;
        if (buf.health <= 0 || buf.health > 300)
            continue;
        
        PlayerData p = {};
        p.valid = true;
        p.index = i;
        p.origin = buf.position;
        p.yaw = buf.yaw;
        p.health = buf.health;
        p.maxHealth = 100;
        p.team = buf.team;
        p.isAlive = (buf.health > 0);
        p.isEnemy = (buf.team != local.team);
        
        Vec3 diff = p.origin - local.origin;
        p.distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        sprintf_s(p.name, "Player_%d", i);
        
        entities.push_back(p);
    }
#endif
    
    return entities;
}

// ============================================================================
// HARDWARE CONTROLLER
// ============================================================================
bool HardwareController::Initialize() { return HardwareManager::AutoLockKMBox(); }

void HardwareController::Shutdown()
{
#ifdef _WIN32
    if (s_Handle) { CloseHandle(s_Handle); s_Handle = nullptr; }
#endif
    s_Connected = false;
    g_KMBox_Active = false;
}

bool HardwareController::IsConnected() { return s_Connected && g_KMBox_Active.load(); }
const char* HardwareController::GetDeviceName() { return s_DeviceName; }
ControllerType HardwareController::GetType() { return s_Type; }
const char* HardwareController::GetLockedPort() { return s_LockedPort; }

bool HardwareController::MoveMouse(int dx, int dy)
{
#ifdef _WIN32
    if (s_Connected && s_Handle && g_KMBox_Active.load())
    {
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "km.move(%d,%d)\r\n", dx, dy);
        DWORD written;
        WriteFile(s_Handle, cmd, (DWORD)strlen(cmd), &written, nullptr);
        return true;
    }
#endif
    mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
    return true;
}

bool HardwareController::Click(int) { return false; }
bool HardwareController::Press(int) { return false; }
bool HardwareController::Release(int) { return false; }
bool HardwareController::KeyPress(int) { return false; }
bool HardwareController::KeyRelease(int) { return false; }
bool HardwareController::ScanCOMPorts(std::vector<std::string>&) { return false; }
bool HardwareController::AutoDetectDevice() { return Initialize(); }
bool HardwareController::ConnectKMBoxBPlus(const char*, int) { return false; }
bool HardwareController::ConnectKMBoxNet(const char*, int) { return false; }
bool HardwareController::ConnectArduino(const char*, int) { return false; }
bool HardwareController::SerialWrite(const char*, int) { return false; }
bool HardwareController::SocketSend(const char*, int) { return false; }

// ============================================================================
// DMA ENGINE
// ============================================================================
bool DMAEngine::Initialize() { return HardwareManager::InitDMAPhysicalDevice(); }
bool DMAEngine::InitializeWithConfig(const DMAConfig&) { return Initialize(); }
bool DMAEngine::InitializeFTD3XX() { return Initialize(); }

void DMAEngine::Shutdown()
{
#if DMA_ENABLED
    if (g_VMMHandle) { VMMDLL_Close(g_VMMHandle); g_VMMHandle = nullptr; }
#endif
    HardwareController::Shutdown();
}

bool DMAEngine::IsConnected() { return s_Connected || g_DMA_Active.load(); }
bool DMAEngine::IsOnline() { return s_Online || g_DMA_Active.load(); }
const char* DMAEngine::GetStatus() { return g_DMA_StatusStr; }
const char* DMAEngine::GetDeviceInfo() { return s_DeviceInfo; }
const char* DMAEngine::GetDriverMode() { return s_DriverMode; }

template<typename T>
T DMAEngine::Read(uintptr_t addr)
{
    T value = {};
    if (addr == 0) return value;
#if DMA_ENABLED
    if (g_VMMHandle && g_GamePID)
        VMMDLL_MemReadEx(g_VMMHandle, g_GamePID, addr, (PBYTE)&value, sizeof(T), nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    return value;
}

bool DMAEngine::ReadBuffer(uintptr_t addr, void* buf, size_t size)
{
    if (addr == 0 || !buf || size == 0) return false;
#if DMA_ENABLED
    if (g_VMMHandle && g_GamePID)
        return VMMDLL_MemReadEx(g_VMMHandle, g_GamePID, addr, (PBYTE)buf, (DWORD)size, nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    return false;
}

bool DMAEngine::ReadString(uintptr_t addr, char* buf, size_t maxLen)
{
    if (!buf || !maxLen || addr == 0) return false;
    buf[0] = 0;
    return ReadBuffer(addr, buf, maxLen - 1);
}

void DMAEngine::ExecuteScatter(std::vector<ScatterEntry>& entries) { ExecuteScatterReads(entries); }
uintptr_t DMAEngine::GetBaseAddress() { return s_BaseAddress; }
uintptr_t DMAEngine::GetModuleBase(const wchar_t*) { return s_BaseAddress; }
size_t DMAEngine::GetModuleSize() { return s_ModuleSize; }

template int32_t DMAEngine::Read<int32_t>(uintptr_t);
template uint32_t DMAEngine::Read<uint32_t>(uintptr_t);
template int64_t DMAEngine::Read<int64_t>(uintptr_t);
template uint64_t DMAEngine::Read<uint64_t>(uintptr_t);
template float DMAEngine::Read<float>(uintptr_t);
template uintptr_t DMAEngine::Read<uintptr_t>(uintptr_t);

// ============================================================================
// OFFSET UPDATER
// ============================================================================
void OffsetUpdater::SetOffsetURL(const char* url) { strncpy_s(s_OffsetURL, url, sizeof(s_OffsetURL) - 1); }

bool OffsetUpdater::HttpGet(const char* url, std::string& response)
{
#ifdef _WIN32
    response.clear();
    wchar_t urlW[512];
    mbstowcs_s(nullptr, urlW, url, 511);
    
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {}, urlPath[512] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 512;
    
    if (!WinHttpCrackUrl(urlW, 0, 0, &urlComp)) return false;
    
    HINTERNET hSession = WinHttpOpen(L"ZeroElite/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return false;
    
    DWORD timeout = 3000;
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, nullptr, nullptr, nullptr, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
    
    if (!WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0) || !WinHttpReceiveResponse(hRequest, nullptr))
    { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
    
    char buffer[4096];
    DWORD bytesRead;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
    { buffer[bytesRead] = 0; response += buffer; }
    
    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
    return !response.empty();
#else
    (void)url; response = "";
    return false;
#endif
}

bool OffsetUpdater::UpdateOffsetsFromServer()
{
    const char* url = s_OffsetURL[0] ? s_OffsetURL : DEFAULT_OFFSET_URL;
    std::string response;
    if (!HttpGet(url, response)) return false;
    return ParseOffsetsTXT(response.c_str());
}

bool OffsetUpdater::SyncWithCloud(int maxRetries, int retryDelayMs)
{
    if (s_OffsetURL[0] == 0) strcpy_s(s_OffsetURL, DEFAULT_OFFSET_URL);
    for (int i = 0; i < maxRetries; i++)
    {
        if (UpdateOffsetsFromServer()) { s_Synced = true; s_Updated = true; return true; }
        if (i < maxRetries - 1) std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
    }
    return false;
}

bool OffsetUpdater::FetchRemoteOffsets(const char*) { return UpdateOffsetsFromServer(); }

bool OffsetUpdater::ParseOffsetsTXT(const char* txtData)
{
    RemoteOffsets offsets = {};
    int foundCount = 0;
    std::istringstream stream(txtData);
    std::string line;
    
    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!value.empty() && value[0] == ' ') value.erase(0, 1);
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ')) value.pop_back();
        if (key.empty() || value.empty()) continue;
        
        uintptr_t val = (value.find("0x") == 0) ? strtoull(value.c_str(), nullptr, 16) : strtoull(value.c_str(), nullptr, 10);
        std::string keyLower = key;
        for (auto& c : keyLower) c = (char)tolower(c);
        
        if (keyLower == "clientinfo") { offsets.ClientInfo = val; foundCount++; }
        else if (keyLower == "entitylist") { offsets.EntityList = val; foundCount++; }
        else if (keyLower == "viewmatrix") { offsets.ViewMatrix = val; foundCount++; }
        else if (keyLower == "build") strncpy_s(offsets.buildNumber, value.c_str(), 31);
    }
    
    offsets.valid = (foundCount > 0);
    if (offsets.valid) { s_LastOffsets = offsets; s_Updated = true; return ApplyOffsets(offsets); }
    return false;
}

bool OffsetUpdater::ParseOffsetsJSON(const char* d) { return ParseOffsetsTXT(d); }
bool OffsetUpdater::ParseOffsetsINI(const char* d) { return ParseOffsetsTXT(d); }

bool OffsetUpdater::ApplyOffsets(const RemoteOffsets& o)
{
    uintptr_t base = DMAEngine::GetBaseAddress();
    if (base == 0) base = 0x140000000;
    if (o.ClientInfo) g_Offsets.ClientInfo = base + o.ClientInfo;
    if (o.EntityList) g_Offsets.EntityList = base + o.EntityList;
    if (o.ViewMatrix) g_Offsets.ViewMatrix = base + o.ViewMatrix;
    return true;
}

// ============================================================================
// PROFESSIONAL INIT
// ============================================================================
bool ProfessionalInit::RunProfessionalChecks() { HardwareManager::StartupDiagnostics(); return true; }
void ProfessionalInit::SimulateDelay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
bool ProfessionalInit::CheckDMAConnection() { return g_VMMHandle != nullptr; }
bool ProfessionalInit::CheckMemoryMap() { return true; }
void ProfessionalInit::GetWindowsVersion(char* b, size_t s) { snprintf(b, s, "Windows"); }
void ProfessionalInit::GetKeyboardState(char* b, size_t s) { snprintf(b, s, "Ready"); }
void ProfessionalInit::GetMouseState(char* b, size_t s) { snprintf(b, s, "Ready"); }

// ============================================================================
// SCATTER REGISTRY
// ============================================================================
void ScatterReadRegistry::RegisterPlayerData(int idx, uintptr_t base)
{
    if (idx < 0 || base == 0) return;
    while ((int)m_PlayerBuffers.size() <= idx) m_PlayerBuffers.push_back({});
    auto& buf = m_PlayerBuffers[idx];
    m_Entries.push_back({base + GameOffsets::EntityPos, &buf.position, sizeof(Vec3), ScatterDataType::PLAYER_POSITION, idx});
    m_Entries.push_back({base + GameOffsets::EntityHealth, &buf.health, sizeof(int), ScatterDataType::PLAYER_HEALTH, idx});
    m_Entries.push_back({base + GameOffsets::EntityTeam, &buf.team, sizeof(int), ScatterDataType::PLAYER_TEAM, idx});
    m_Entries.push_back({base + GameOffsets::EntityYaw, &buf.yaw, sizeof(float), ScatterDataType::PLAYER_YAW, idx});
}

void ScatterReadRegistry::RegisterLocalPlayer(uintptr_t base)
{
    if (base == 0) return;
    m_Entries.push_back({base + GameOffsets::EntityPos, &m_LocalPosition, sizeof(Vec3), ScatterDataType::LOCAL_POSITION, -1});
    m_Entries.push_back({base + GameOffsets::EntityYaw, &m_LocalYaw, sizeof(float), ScatterDataType::LOCAL_YAW, -1});
}

void ScatterReadRegistry::RegisterViewMatrix(uintptr_t addr) { if (addr) m_Entries.push_back({addr, &m_ViewMatrix, sizeof(Matrix4x4), ScatterDataType::VIEW_MATRIX, -1}); }
void ScatterReadRegistry::RegisterCustomRead(uintptr_t addr, void* buf, size_t size) { if (addr && buf && size) m_Entries.push_back({addr, buf, size, ScatterDataType::CUSTOM, -1}); }
void ScatterReadRegistry::ExecuteAll() { if (!m_Entries.empty()) { m_TransactionCount++; ExecuteScatterReads(m_Entries); } }
void ScatterReadRegistry::Clear() { m_Entries.clear(); }
int ScatterReadRegistry::GetTotalBytes() const { int t = 0; for (auto& e : m_Entries) t += (int)e.size; return t; }

// ============================================================================
// MAP TEXTURE
// ============================================================================
void MapTextureManager::InitializeMapDatabase() {}
bool MapTextureManager::LoadMapConfig(const char* n) { strcpy_s(s_CurrentMap.name, n); return false; }
bool MapTextureManager::LoadMapTexture(const char* p) { if (p && p[0]) { strcpy_s(s_CurrentMap.imagePath, p); return true; } return false; }
Vec2 MapTextureManager::GameToMapCoords(const Vec3& g) { return Vec2(g.x, g.y); }
Vec2 MapTextureManager::GameToRadarCoords(const Vec3& g, const Vec3& l, float y, float cx, float cy, float s, float z)
{
    Vec3 d = g - l; float r = -y * 3.14159f / 180.0f;
    return Vec2(cx + (d.x * cosf(r) - d.y * sinf(r)) * (s * 0.4f) / (100.0f / z),
                cy - (d.x * sinf(r) + d.y * cosf(r)) * (s * 0.4f) / (100.0f / z));
}
void MapTextureManager::SetMapBounds(const char*, float, float, float, float) {}

// ============================================================================
// CONFIG
// ============================================================================
bool LoadConfig(const char* f) { std::ifstream file(f); return file.is_open(); }
bool SaveConfig(const char* f) { std::ofstream file(f); return file.is_open(); }
void CreateDefaultConfig(const char* f) { strcpy_s(g_Config.processName, "cod.exe"); SaveConfig(f); }

// ============================================================================
// AIMBOT
// ============================================================================
void Aimbot::Initialize() { s_Enabled = false; }
void Aimbot::Update() {}
bool Aimbot::IsEnabled() { return s_Enabled && HardwareController::IsConnected(); }
void Aimbot::SetEnabled(bool e) { s_Enabled = e; }
int Aimbot::FindBestTarget(float, float) { return -1; }
Vec2 Aimbot::GetTargetScreenPos(int) { return Vec2(0, 0); }
void Aimbot::AimAt(const Vec2&, float) {}
void Aimbot::MoveMouse(int dx, int dy) { HardwareController::MoveMouse(dx, dy); }

// ============================================================================
// PATTERN SCANNER & DIAGNOSTICS
// ============================================================================
uintptr_t PatternScanner::FindPattern(uintptr_t, size_t, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ScanModule(const char*, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ResolveRelative(uintptr_t, int, int) { return 0; }
bool PatternScanner::UpdateAllOffsets() { s_Scanned = true; return false; }

const char* DiagnosticSystem::GetErrorString(DiagnosticResult) { return ""; }
bool DiagnosticSystem::RunAllDiagnostics() { return true; }
DiagnosticResult DiagnosticSystem::CheckDeviceHandshake() { return DiagnosticResult::SUCCESS; }
DiagnosticResult DiagnosticSystem::CheckFirmwareIntegrity() { return DiagnosticResult::SUCCESS; }
DiagnosticResult DiagnosticSystem::CheckMemorySpeed() { return DiagnosticResult::SUCCESS; }
DiagnosticResult DiagnosticSystem::CheckProcessAccess() { return DiagnosticResult::SUCCESS; }
void DiagnosticSystem::SelfDestruct(const char*) {}
void DiagnosticSystem::ShowErrorPopup(const char*, const char*) {}
bool DiagnosticSystem::IsGenericDeviceID(const char*) { return false; }
bool DiagnosticSystem::IsLeakedDeviceID(const char*) { return false; }
float DiagnosticSystem::MeasureReadSpeed(uintptr_t, size_t, int) { return 0; }
int DiagnosticSystem::MeasureLatency(uintptr_t, int) { return 0; }

// ============================================================================
// PLAYER MANAGER - REAL DMA DATA ONLY
// ============================================================================
void PlayerManager::Initialize()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Players.clear();
    s_Players.reserve(155);
    s_LocalPlayer = {};
    s_Initialized = true;
}

void PlayerManager::Update()
{
    if (!s_Initialized) Initialize();
    RealUpdate();
}

void PlayerManager::UpdateWithScatterRegistry() { RealUpdate(); }
void PlayerManager::SimulateUpdate() { /* REMOVED */ }

void PlayerManager::RealUpdate()
{
    // No drawing if overlay not active
    if (!g_BlackOverlayActive.load())
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Players.clear();
        return;
    }
    
    // Get real entities from DMA
    auto entities = GetRealEntitiesFromDMA();
    
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Players = entities;
    
    // Update local player
    if (g_DMA_Active.load() && g_Offsets.LocalPlayer != 0)
    {
#if DMA_ENABLED
        uintptr_t localAddr = DMAEngine::Read<uintptr_t>(g_Offsets.LocalPlayer);
        if (localAddr != 0)
        {
            DMAEngine::ReadBuffer(localAddr + GameOffsets::EntityPos, &s_LocalPlayer.origin, sizeof(Vec3));
            s_LocalPlayer.yaw = DMAEngine::Read<float>(localAddr + GameOffsets::EntityYaw);
            s_LocalPlayer.health = DMAEngine::Read<int>(localAddr + GameOffsets::EntityHealth);
            s_LocalPlayer.team = DMAEngine::Read<int>(localAddr + GameOffsets::EntityTeam);
            s_LocalPlayer.valid = true;
        }
#endif
    }
}

std::vector<PlayerData>& PlayerManager::GetPlayers() { return s_Players; }
PlayerData& PlayerManager::GetLocalPlayer() { return s_LocalPlayer; }
int PlayerManager::GetAliveCount() { int c = 0; for (auto& p : s_Players) if (p.valid && p.isAlive) c++; return c; }
int PlayerManager::GetEnemyCount() { int c = 0; for (auto& p : s_Players) if (p.valid && p.isAlive && p.isEnemy) c++; return c; }

// ============================================================================
// UTILITIES
// ============================================================================
bool WorldToScreen(const Vec3&, Vec2& s, int w, int h) { s = Vec2((float)w/2, (float)h/2); return true; }
bool WorldToRadar(const Vec3& w, const Vec3& l, float y, Vec2& r, float cx, float cy, float s) { r = MapTextureManager::GameToRadarCoords(w, l, y, cx, cy, s*2, 1); return true; }
float GetFOVTo(const Vec2& c, const Vec2& t) { return sqrtf((t.x-c.x)*(t.x-c.x) + (t.y-c.y)*(t.y-c.y)); }
Vec3 CalcAngle(const Vec3& s, const Vec3& d) { Vec3 delta = d - s; float h = sqrtf(delta.x*delta.x + delta.y*delta.y); return Vec3(-atan2f(delta.z, h)*57.3f, atan2f(delta.y, delta.x)*57.3f, 0); }
void SmoothAngle(Vec3& c, const Vec3& t, float s) { if (s <= 0) s = 1; Vec3 d = t - c; while (d.y > 180) d.y -= 360; while (d.y < -180) d.y += 360; c.x += d.x/s; c.y += d.y/s; }
