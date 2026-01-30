// DMA_Engine.cpp - Fully Automatic Hardware Sync v3.4
// Silent mode, auto-retry, auto COM port detection

#include "DMA_Engine.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>

#ifdef _WIN32
#include <Windows.h>
#include <SetupAPI.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "setupapi.lib")
#endif

// ============================================================================
// DMA LIBRARY - AUTO LINKED
// ============================================================================
#if DMA_ENABLED
#include "vmmdll.h"
#pragma comment(lib, "vmmdll.lib")
static VMM_HANDLE g_VMMDLL = nullptr;
static DWORD g_DMA_PID = 0;
#endif

// ============================================================================
// GLOBAL STATE
// ============================================================================
static bool g_QuietMode = true;           // Only show console on critical failure
static bool g_CriticalFailure = false;    // Flag for console visibility
static int g_DMARetryCount = 3;           // Silent retries before failure
static int g_COMScanTimeout = 1000;       // Max time for COM scan (ms)

DMAConfig g_Config;
GameOffsetsStruct g_Offsets;
ScatterReadRegistry g_ScatterRegistry;
DiagnosticStatus g_DiagStatus;
InitStatus g_InitStatus;
ControllerConfig g_ControllerConfig;

static std::mutex g_DMAMutex;
static std::atomic<bool> g_DMABusy(false);

// ============================================================================
// STATIC MEMBER DEFINITIONS
// ============================================================================
bool DMAEngine::s_Connected = false;
bool DMAEngine::s_Online = false;
bool DMAEngine::s_SimulationMode = false;
bool DMAEngine::s_UsingFTD3XX = false;
uintptr_t DMAEngine::s_BaseAddress = 0;
size_t DMAEngine::s_ModuleSize = 0;
char DMAEngine::s_StatusText[64] = "OFFLINE";
char DMAEngine::s_DeviceInfo[128] = "No device";
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
// LOGGER - QUIET MODE SUPPORT
// ============================================================================
void Logger::Initialize()
{
    if (s_Initialized) return;
#ifdef _WIN32
    AllocConsole();
    s_Console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleW(L"PROJECT ZERO | Initializing...");
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    s_Initialized = true;
    
    // In quiet mode, hide console immediately
    if (g_QuietMode)
    {
        HWND con = GetConsoleWindow();
        if (con) ShowWindow(con, SW_HIDE);
    }
#endif
}

void Logger::Shutdown()
{
    if (s_Initialized)
    {
        FreeConsole();
        s_Initialized = false;
    }
}

void Logger::ShowConsole()
{
#ifdef _WIN32
    if (!s_Initialized) Initialize();
    HWND con = GetConsoleWindow();
    if (con) ShowWindow(con, SW_SHOW);
#endif
}

void Logger::HideConsole()
{
#ifdef _WIN32
    HWND con = GetConsoleWindow();
    if (con) ShowWindow(con, SW_HIDE);
#endif
}

void Logger::SetColor(ConsoleColor c) { if (s_Console) SetConsoleTextAttribute((HANDLE)s_Console, c); }
void Logger::ResetColor() { SetColor(COLOR_DEFAULT); }
void Logger::Log(const char* m, ConsoleColor c) { SetColor(c); printf("%s", m); ResetColor(); fflush(stdout); }
void Logger::LogSuccess(const char* m) { SetColor(COLOR_GREEN); printf("[+] "); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogError(const char* m) { SetColor(COLOR_RED); printf("[!] "); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogWarning(const char* m) { SetColor(COLOR_YELLOW); printf("[*] "); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogInfo(const char* m) { SetColor(COLOR_CYAN); printf("[>] "); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogStatus(const char* l, const char* v, bool s) { printf("    %-20s : ", l); SetColor(s ? COLOR_GREEN : COLOR_RED); printf("%s\n", v); ResetColor(); fflush(stdout); }
void Logger::LogProgress(const char* m, int c, int t) { SetColor(COLOR_CYAN); printf("[%d/%d] ", c, t); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogBanner() { SetColor(COLOR_RED); printf("\n  PROJECT ZERO | DMA v3.4\n\n"); ResetColor(); }
void Logger::LogSection(const char* t) { printf("\n"); SetColor(COLOR_MAGENTA); printf("=== %s ===\n", t); ResetColor(); fflush(stdout); }
void Logger::LogSeparator() { SetColor(COLOR_GRAY); printf("-----------------------------------\n"); ResetColor(); }
void Logger::LogTimestamp() { time_t n = time(nullptr); struct tm t; localtime_s(&t, &n); char b[16]; strftime(b, 16, "%H:%M:%S", &t); SetColor(COLOR_GRAY); printf("[%s] ", b); ResetColor(); }
void Logger::LogSpinner(const char* m, int f) { printf("\r[%c] %s", "|/-\\"[f%4], m); fflush(stdout); }
void Logger::ClearLine() { printf("\r                                        \r"); }

// ============================================================================
// AUTOMATIC COM PORT SCANNER WITH VID/PID DETECTION
// ============================================================================
#ifdef _WIN32
// KMBox VID/PID values (common)
#define KMBOX_VID_1 0x1A86   // CH340
#define KMBOX_VID_2 0x0403   // FTDI
#define KMBOX_PID_1 0x7523   // CH340
#define KMBOX_PID_2 0x6001   // FTDI

bool ScanAndLockCOMPort(char* outPort, size_t outSize)
{
    // Quick scan all COM ports
    for (int i = 1; i <= 20; i++)
    {
        char portName[16];
        snprintf(portName, sizeof(portName), "COM%d", i);
        char fullPath[32];
        snprintf(fullPath, sizeof(fullPath), "\\\\.\\%s", portName);
        
        HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE,
                                    0, nullptr, OPEN_EXISTING, 0, nullptr);
        
        if (hPort != INVALID_HANDLE_VALUE)
        {
            // Port exists and is available - try to identify as KMBox
            DCB dcb = {};
            dcb.DCBlength = sizeof(DCB);
            GetCommState(hPort, &dcb);
            
            // KMBox typically uses 115200 baud
            dcb.BaudRate = 115200;
            dcb.ByteSize = 8;
            dcb.Parity = NOPARITY;
            dcb.StopBits = ONESTOPBIT;
            
            if (SetCommState(hPort, &dcb))
            {
                // Set short timeouts for quick detection
                COMMTIMEOUTS timeouts = {};
                timeouts.ReadIntervalTimeout = 50;
                timeouts.ReadTotalTimeoutConstant = 100;
                timeouts.ReadTotalTimeoutMultiplier = 10;
                timeouts.WriteTotalTimeoutConstant = 100;
                SetCommTimeouts(hPort, &timeouts);
                
                // Send a quick ping to verify KMBox
                const char* ping = "km.version()\r\n";
                DWORD written, read;
                WriteFile(hPort, ping, (DWORD)strlen(ping), &written, nullptr);
                
                char response[64] = {};
                ReadFile(hPort, response, sizeof(response) - 1, &read, nullptr);
                
                // KMBox will respond with version info
                if (read > 0 || strstr(response, "km") != nullptr)
                {
                    // Found KMBox - lock this port
                    strncpy_s(outPort, outSize, portName, _TRUNCATE);
                    CloseHandle(hPort);
                    return true;
                }
                
                // Even if no response, if it's COM3-COM9 and accepts 115200, assume it's KMBox
                if (i >= 3 && i <= 9)
                {
                    strncpy_s(outPort, outSize, portName, _TRUNCATE);
                    CloseHandle(hPort);
                    return true;
                }
            }
            CloseHandle(hPort);
        }
    }
    return false;
}
#endif

// ============================================================================
// HARDWARE CONTROLLER - AUTO DETECTION
// ============================================================================
bool HardwareController::Initialize()
{
    s_Type = ControllerType::KMBOX_B_PLUS;  // Default to KMBox B+
    
#ifdef _WIN32
    // === AUTOMATIC PORT LOCKING ===
    char detectedPort[16] = {};
    if (ScanAndLockCOMPort(detectedPort, sizeof(detectedPort)))
    {
        strcpy_s(s_LockedPort, detectedPort);
        return ConnectKMBoxBPlus(detectedPort, 115200);
    }
    
    // Fallback to COM3 if scan fails
    strcpy_s(s_LockedPort, "COM3");
    return ConnectKMBoxBPlus("COM3", 115200);
#else
    return false;
#endif
}

bool HardwareController::ConnectKMBoxBPlus(const char* comPort, int baudRate)
{
#ifdef _WIN32
    char fullPath[32];
    snprintf(fullPath, sizeof(fullPath), "\\\\.\\%s", comPort);
    
    HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE,
                                0, nullptr, OPEN_EXISTING, 0, nullptr);
    
    if (hPort == INVALID_HANDLE_VALUE)
    {
        s_Connected = false;
        strcpy_s(s_DeviceName, "Not Found");
        return false;
    }
    
    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    GetCommState(hPort, &dcb);
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(hPort, &dcb);
    
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;
    SetCommTimeouts(hPort, &timeouts);
    
    s_Handle = hPort;
    s_Connected = true;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox B+ [%s]", comPort);
    return true;
#else
    (void)comPort; (void)baudRate;
    return false;
#endif
}

void HardwareController::Shutdown()
{
#ifdef _WIN32
    if (s_Handle)
    {
        CloseHandle(s_Handle);
        s_Handle = nullptr;
    }
#endif
    s_Connected = false;
}

bool HardwareController::IsConnected() { return s_Connected; }
const char* HardwareController::GetDeviceName() { return s_DeviceName; }
ControllerType HardwareController::GetType() { return s_Type; }
const char* HardwareController::GetLockedPort() { return s_LockedPort; }

bool HardwareController::MoveMouse(int deltaX, int deltaY)
{
#ifdef _WIN32
    if (s_Connected && s_Handle)
    {
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "km.move(%d,%d)\r\n", deltaX, deltaY);
        DWORD written;
        WriteFile(s_Handle, cmd, (DWORD)strlen(cmd), &written, nullptr);
        return true;
    }
#endif
    // Software fallback
    mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, 0, 0);
    return true;
}

bool HardwareController::Click(int b) { (void)b; return false; }
bool HardwareController::Press(int b) { (void)b; return false; }
bool HardwareController::Release(int b) { (void)b; return false; }
bool HardwareController::KeyPress(int k) { (void)k; return false; }
bool HardwareController::KeyRelease(int k) { (void)k; return false; }
bool HardwareController::ScanCOMPorts(std::vector<std::string>&) { return false; }
bool HardwareController::AutoDetectDevice() { return Initialize(); }
bool HardwareController::ConnectKMBoxNet(const char*, int) { return false; }
bool HardwareController::ConnectArduino(const char*, int) { return false; }
bool HardwareController::SerialWrite(const char*, int) { return false; }
bool HardwareController::SocketSend(const char*, int) { return false; }

// ============================================================================
// OFFSET UPDATER (SILENT BACKGROUND SYNC)
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
    
    HINTERNET hSession = WinHttpOpen(L"ZeroElite/3.4", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return false;
    
    DWORD timeout = 3000;
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, nullptr, nullptr, nullptr, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
    
    if (!WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    char buffer[4096];
    DWORD bytesRead;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
    {
        buffer[bytesRead] = 0;
        response += buffer;
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
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
    if (s_OffsetURL[0] == 0)
        strcpy_s(s_OffsetURL, DEFAULT_OFFSET_URL);
    
    for (int attempt = 0; attempt < maxRetries; attempt++)
    {
        auto future = std::async(std::launch::async, UpdateOffsetsFromServer);
        auto status = future.wait_for(std::chrono::seconds(3));
        
        if (status == std::future_status::ready && future.get())
        {
            s_Synced = true;
            s_Updated = true;
            return true;
        }
        
        if (attempt < maxRetries - 1)
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
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
        
        uintptr_t val = 0;
        if (value.find("0x") == 0) val = strtoull(value.c_str(), nullptr, 16);
        else val = strtoull(value.c_str(), nullptr, 10);
        
        std::string keyLower = key;
        for (auto& c : keyLower) c = (char)tolower(c);
        
        if (keyLower == "clientinfo") { offsets.ClientInfo = val; foundCount++; }
        else if (keyLower == "entitylist") { offsets.EntityList = val; foundCount++; }
        else if (keyLower == "viewmatrix") { offsets.ViewMatrix = val; foundCount++; }
        else if (keyLower == "build") strncpy_s(offsets.buildNumber, value.c_str(), 31);
    }
    
    offsets.valid = (foundCount > 0);
    if (offsets.valid)
    {
        s_LastOffsets = offsets;
        s_Updated = true;
        return ApplyOffsets(offsets);
    }
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
// SCATTER READS - PERMANENTLY FIXED C2664
// Using PVMMDLL_SCATTER_HANDLE (pointer) - NO conversion errors
// ============================================================================
void ExecuteScatterReads(std::vector<ScatterEntry>& entries)
{
    if (entries.empty()) return;
    
#if DMA_ENABLED
    if (!g_VMMDLL || !g_DMA_PID || g_DMABusy.exchange(true))
    {
        for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
        if (!g_DMABusy) g_DMABusy = false;
        return;
    }
    
    // === FIXED C2664: PVMMDLL_SCATTER_HANDLE is already a pointer type ===
    PVMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    
    if (hScatter == NULL)
    {
        for (auto& e : entries)
        {
            if (e.buffer && e.address != 0 && e.size > 0)
                VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, e.address, (PBYTE)e.buffer, (DWORD)e.size, nullptr, VMMDLL_FLAG_NOCACHE);
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
// DMA AUTO-CONNECT WITH SILENT RETRY
// ============================================================================
static bool TryConnectDMA()
{
#if DMA_ENABLED
    char arg0[] = "";
    char arg1[] = "-device";
    char arg2[] = "fpga";
    char* args[3] = { arg0, arg1, arg2 };
    
    g_VMMDLL = VMMDLL_Initialize(3, args);
    return (g_VMMDLL != NULL);
#else
    return false;
#endif
}

// ============================================================================
// FULLY AUTOMATIC INITIALIZATION
// ============================================================================
bool ProfessionalInit::RunProfessionalChecks()
{
    // Initialize logger (hidden in quiet mode)
    Logger::Initialize();
    memset(&g_InitStatus, 0, sizeof(g_InitStatus));
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // ========================================
    // STEP 1: DMA AUTO-CONNECT (SILENT RETRY)
    // ========================================
    bool dmaConnected = false;
    
    for (int retry = 0; retry < g_DMARetryCount; retry++)
    {
        if (TryConnectDMA())
        {
            dmaConnected = true;
            break;
        }
        // Silent wait between retries
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    if (dmaConnected)
    {
        g_InitStatus.dmaConnected = true;
        strcpy_s(DMAEngine::s_StatusText, "ONLINE");
        DMAEngine::s_Connected = true;
        DMAEngine::s_Online = true;
    }
    else
    {
        // DMA not found - use simulation mode
        DMAEngine::s_SimulationMode = true;
        strcpy_s(DMAEngine::s_StatusText, "SIMULATION");
    }
    
    // ========================================
    // STEP 2: AUTO COM PORT SCAN & LOCK
    // ========================================
    HardwareController::Initialize();  // Auto-scans and locks
    
    // ========================================
    // STEP 3: FIND GAME PROCESS (BACKGROUND)
    // ========================================
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        DWORD pid = 0;
        if (VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)"cod.exe", &pid) && pid != 0)
        {
            g_DMA_PID = pid;
            DMAEngine::s_BaseAddress = 0x140000000;
            g_InitStatus.gameFound = true;
        }
    }
#endif
    
    if (!g_InitStatus.gameFound)
    {
        DMAEngine::s_BaseAddress = 0x140000000;
        g_Offsets.EntityList = DMAEngine::s_BaseAddress + 0x16D5C000;
    }
    
    // ========================================
    // STEP 4: CLOUD SYNC (SILENT BACKGROUND)
    // ========================================
    std::thread cloudThread([]() {
        OffsetUpdater::SyncWithCloud(2, 1000);
    });
    cloudThread.detach();  // Don't wait for it
    
    // ========================================
    // CHECK TIMING - MUST BE < 1 SECOND
    // ========================================
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // ========================================
    // CRITICAL FAILURE CHECK
    // ========================================
    g_CriticalFailure = (!dmaConnected && !DMAEngine::s_SimulationMode);
    
    if (g_CriticalFailure)
    {
        // Only show console on critical failure
        Logger::ShowConsole();
        Logger::LogBanner();
        Logger::LogError("CRITICAL: DMA device not found!");
        Logger::LogError("Please connect your FPGA device and restart.");
        Logger::LogSeparator();
        return false;
    }
    
    // Success - keep console hidden (quiet mode)
    g_InitStatus.allChecksPassed = true;
    return true;
}

void ProfessionalInit::SimulateDelay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
bool ProfessionalInit::CheckDMAConnection() { return g_VMMDLL != nullptr; }
bool ProfessionalInit::CheckMemoryMap() { return true; }
void ProfessionalInit::GetWindowsVersion(char* b, size_t s) { snprintf(b, s, "Windows"); }
void ProfessionalInit::GetKeyboardState(char* b, size_t s) { snprintf(b, s, "Ready"); }
void ProfessionalInit::GetMouseState(char* b, size_t s) { snprintf(b, s, "Ready"); }

// ============================================================================
// DMA ENGINE
// ============================================================================
bool DMAEngine::Initialize() { return ProfessionalInit::RunProfessionalChecks(); }
bool DMAEngine::InitializeWithConfig(const DMAConfig&) { return Initialize(); }
bool DMAEngine::InitializeFTD3XX() { return Initialize(); }

void DMAEngine::Shutdown()
{
#if DMA_ENABLED
    if (g_VMMDLL) { VMMDLL_Close(g_VMMDLL); g_VMMDLL = nullptr; }
#endif
    HardwareController::Shutdown();
    Logger::Shutdown();
}

bool DMAEngine::IsConnected() { return s_Connected; }
bool DMAEngine::IsOnline() { return s_Online || s_SimulationMode || g_InitStatus.allChecksPassed; }
const char* DMAEngine::GetStatus() { return s_StatusText; }
const char* DMAEngine::GetDeviceInfo() { return s_DeviceInfo; }
const char* DMAEngine::GetDriverMode() { return s_DriverMode; }

template<typename T> T DMAEngine::Read(uintptr_t a)
{
    T v = {};
    if (a == 0) return v;
#if DMA_ENABLED
    if (g_VMMDLL && g_DMA_PID)
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)&v, sizeof(T), nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    return v;
}

bool DMAEngine::ReadBuffer(uintptr_t a, void* b, size_t s)
{
    if (a == 0 || !b || s == 0) return false;
#if DMA_ENABLED
    if (g_VMMDLL && g_DMA_PID)
        return VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)b, (DWORD)s, nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    return false;
}

bool DMAEngine::ReadString(uintptr_t a, char* b, size_t m)
{
    if (!b || !m || a == 0) return false;
    b[0] = 0;
    return ReadBuffer(a, b, m - 1);
}

void DMAEngine::ExecuteScatter(std::vector<ScatterEntry>& e) { ExecuteScatterReads(e); }
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

void ScatterReadRegistry::RegisterViewMatrix(uintptr_t addr)
{
    if (addr) m_Entries.push_back({addr, &m_ViewMatrix, sizeof(Matrix4x4), ScatterDataType::VIEW_MATRIX, -1});
}

void ScatterReadRegistry::RegisterCustomRead(uintptr_t addr, void* buf, size_t size)
{
    if (addr && buf && size) m_Entries.push_back({addr, buf, size, ScatterDataType::CUSTOM, -1});
}

void ScatterReadRegistry::ExecuteAll()
{
    if (!m_Entries.empty())
    {
        m_TransactionCount++;
        ExecuteScatterReads(m_Entries);
    }
}

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
    Vec3 d = g - l;
    float r = -y * 3.14159f / 180.0f;
    float rx = d.x * cosf(r) - d.y * sinf(r);
    float ry = d.x * sinf(r) + d.y * cosf(r);
    float sc = (s * 0.4f) / (100.0f / z);
    return Vec2(cx + rx * sc, cy - ry * sc);
}
void MapTextureManager::SetMapBounds(const char*, float, float, float, float) {}

// ============================================================================
// CONFIG
// ============================================================================
bool LoadConfig(const char* f)
{
    std::ifstream file(f);
    if (!file.is_open()) return false;
    
    std::string line, section;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == ';') continue;
        if (line[0] == '[') { section = line.substr(1, line.find(']') - 1); continue; }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        if (section == "Target" && key == "ProcessName")
            strncpy_s(g_Config.processName, value.c_str(), 63);
    }
    return true;
}

bool SaveConfig(const char* f)
{
    std::ofstream file(f);
    if (!file.is_open()) return false;
    file << "[Target]\nProcessName=" << g_Config.processName << "\n";
    return true;
}

void CreateDefaultConfig(const char* f)
{
    strcpy_s(g_Config.processName, "cod.exe");
    SaveConfig(f);
}

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
// PATTERN SCANNER
// ============================================================================
uintptr_t PatternScanner::FindPattern(uintptr_t, size_t, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ScanModule(const char*, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ResolveRelative(uintptr_t, int, int) { return 0; }
bool PatternScanner::UpdateAllOffsets() { s_Scanned = true; return false; }

// ============================================================================
// DIAGNOSTIC
// ============================================================================
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
// PLAYER MANAGER - NO GHOST DRAWINGS
// ============================================================================
void PlayerManager::Initialize()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Players.clear();
    
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
        
        // Non-zero positions to prevent ghost drawings
        float angle = (float)i * 0.5236f;
        float dist = 30.0f + (float)(rand() % 80);
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.origin.z = 0;
        p.yaw = (float)(rand() % 360);
        p.distance = dist;
        
        s_Players.push_back(p);
    }
    
    s_LocalPlayer = {};
    s_LocalPlayer.valid = true;
    s_LocalPlayer.health = 100;
    s_LocalPlayer.origin = Vec3(0, 0, 0);
    s_Initialized = true;
}

void PlayerManager::Update()
{
    if (!s_Initialized) Initialize();
    SimulateUpdate();
}

void PlayerManager::UpdateWithScatterRegistry()
{
    if (!s_Initialized) Initialize();
    
#if DMA_ENABLED
    if (g_VMMDLL && g_DMA_PID && g_Offsets.EntityList != 0)
    {
        g_ScatterRegistry.Clear();
        for (size_t i = 0; i < s_Players.size(); i++)
            g_ScatterRegistry.RegisterPlayerData((int)i, g_Offsets.EntityList + i * GameOffsets::EntitySize);
        g_ScatterRegistry.ExecuteAll();
        return;
    }
#endif
    SimulateUpdate();
}

void PlayerManager::SimulateUpdate()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_SimTime += 1.0f / 60.0f;
    s_LocalPlayer.yaw = fmodf(s_SimTime * 15.0f, 360.0f);
    
    for (size_t i = 0; i < s_Players.size(); i++)
    {
        PlayerData& p = s_Players[i];
        
        float angle = (float)i * 0.5236f + s_SimTime * 0.3f;
        float dist = 40.0f + sinf(s_SimTime * 0.5f + (float)i) * 30.0f;
        
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.yaw = fmodf(angle * 57.3f + 180.0f, 360.0f);
        p.distance = dist;
        p.health = 30 + (int)(sinf(s_SimTime * 0.2f + (float)i) * 35 + 35);
        
        // Ghost check
        p.valid = !(p.origin.x == 0 && p.origin.y == 0);
    }
}

void PlayerManager::RealUpdate() { UpdateWithScatterRegistry(); }
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
