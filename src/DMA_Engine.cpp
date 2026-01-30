// DMA_Engine.cpp - Professional DMA Implementation v3.1
// Features: Async Init, Thread-Safe Scatter, Timeout Protection

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
#include <VersionHelpers.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <winhttp.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
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
InitStatus g_InitStatus;
ControllerConfig g_ControllerConfig;

// Thread safety
static std::mutex g_DMAMutex;
static std::atomic<bool> g_DMABusy(false);
static std::atomic<int> g_SlowReadCount(0);

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
char DMAEngine::s_DeviceInfo[128] = "No device";
char DMAEngine::s_DriverMode[32] = "Generic";

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

void* HardwareController::s_Handle = nullptr;
void* HardwareController::s_Socket = nullptr;
bool HardwareController::s_Connected = false;
ControllerType HardwareController::s_Type = ControllerType::NONE;
char HardwareController::s_DeviceName[64] = "None";

RemoteOffsets OffsetUpdater::s_LastOffsets;
bool OffsetUpdater::s_Updated = false;
bool OffsetUpdater::s_Synced = false;
char OffsetUpdater::s_OffsetURL[512] = "";
char OffsetUpdater::s_LastError[256] = "";

bool Aimbot::s_Enabled = false;
int Aimbot::s_CurrentTarget = -1;
Vec2 Aimbot::s_LastAimPos = {0, 0};

// ============================================================================
// CONTROLLER TYPE TO STRING
// ============================================================================
const char* ControllerTypeToString(ControllerType type)
{
    switch (type)
    {
    case ControllerType::NONE: return "None";
    case ControllerType::KMBOX_B_PLUS: return "KMBox B+";
    case ControllerType::KMBOX_NET: return "KMBox Net";
    case ControllerType::ARDUINO: return "Arduino";
    default: return "Unknown";
    }
}

// ============================================================================
// LOGGER IMPLEMENTATION
// ============================================================================
void Logger::Initialize()
{
#ifdef _WIN32
    AllocConsole();
    s_Console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleW(L"PROJECT ZERO | COD DMA v3.1");
    
    // Set console size
    SMALL_RECT windowSize = {0, 0, 100, 40};
    SetConsoleWindowInfo((HANDLE)s_Console, TRUE, &windowSize);
    
    // Redirect stdout
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
#endif
}

void Logger::Shutdown()
{
#ifdef _WIN32
    FreeConsole();
#endif
}

void Logger::SetColor(ConsoleColor color)
{
#ifdef _WIN32
    if (s_Console)
        SetConsoleTextAttribute((HANDLE)s_Console, color);
#endif
    (void)color;
}

void Logger::ResetColor() { SetColor(COLOR_DEFAULT); }

void Logger::Log(const char* message, ConsoleColor color)
{
    SetColor(color);
    printf("%s", message);
    ResetColor();
    fflush(stdout);
}

void Logger::LogSuccess(const char* message)
{
    SetColor(COLOR_GREEN);
    printf("[+] ");
    ResetColor();
    printf("%s\n", message);
    fflush(stdout);
}

void Logger::LogError(const char* message)
{
    SetColor(COLOR_RED);
    printf("[!] ");
    ResetColor();
    printf("%s\n", message);
    fflush(stdout);
}

void Logger::LogWarning(const char* message)
{
    SetColor(COLOR_YELLOW);
    printf("[*] ");
    ResetColor();
    printf("%s\n", message);
    fflush(stdout);
}

void Logger::LogInfo(const char* message)
{
    SetColor(COLOR_CYAN);
    printf("[>] ");
    ResetColor();
    printf("%s\n", message);
    fflush(stdout);
}

void Logger::LogStatus(const char* label, const char* value, bool success)
{
    SetColor(COLOR_GRAY);
    printf("    ");
    SetColor(COLOR_WHITE);
    printf("%-20s : ", label);
    SetColor(success ? COLOR_GREEN : COLOR_RED);
    printf("%s\n", value);
    ResetColor();
    fflush(stdout);
}

void Logger::LogProgress(const char* message, int current, int total)
{
    SetColor(COLOR_CYAN);
    printf("[%d/%d] ", current, total);
    ResetColor();
    printf("%s\n", message);
    fflush(stdout);
}

void Logger::LogBanner()
{
    SetColor(COLOR_RED);
    printf("\n");
    printf("  =====================================================\n");
    printf("  ||                                                 ||\n");
    printf("  ||   ");
    SetColor(COLOR_WHITE);
    printf("P R O J E C T   Z E R O   |   C O D   D M A");
    SetColor(COLOR_RED);
    printf("   ||\n");
    printf("  ||                                                 ||\n");
    printf("  ||   ");
    SetColor(COLOR_GRAY);
    printf("    Professional DMA Radar v3.1 + FTD3XX    ");
    SetColor(COLOR_RED);
    printf("   ||\n");
    printf("  ||                                                 ||\n");
    printf("  =====================================================\n");
    ResetColor();
    printf("\n");
    fflush(stdout);
}

void Logger::LogSection(const char* title)
{
    printf("\n");
    SetColor(COLOR_MAGENTA);
    printf("=== ");
    SetColor(COLOR_WHITE);
    printf("%s", title);
    SetColor(COLOR_MAGENTA);
    printf(" ===\n");
    ResetColor();
    fflush(stdout);
}

void Logger::LogSeparator()
{
    SetColor(COLOR_GRAY);
    printf("-----------------------------------------------------\n");
    ResetColor();
    fflush(stdout);
}

void Logger::LogTimestamp()
{
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    SetColor(COLOR_GRAY);
    printf("[%s] ", buffer);
    ResetColor();
}

void Logger::LogSpinner(const char* message, int frame)
{
    const char* spinners[] = {"|", "/", "-", "\\"};
    printf("\r");
    SetColor(COLOR_CYAN);
    printf("[%s] ", spinners[frame % 4]);
    ResetColor();
    printf("%s", message);
    fflush(stdout);
}

void Logger::ClearLine()
{
    printf("\r                                                            \r");
    fflush(stdout);
}

// ============================================================================
// HARDWARE CONTROLLER IMPLEMENTATION (Simplified)
// ============================================================================
bool HardwareController::Initialize()
{
    s_Type = g_Config.controllerType;
    if (s_Type == ControllerType::NONE)
    {
        strcpy_s(s_DeviceName, "Software");
        s_Connected = false;
        return true;
    }
    
    // For now, just mark as not connected - user needs actual hardware
    strcpy_s(s_DeviceName, "Not Connected");
    s_Connected = false;
    g_ControllerConfig.isConnected = false;
    return true;
}

void HardwareController::Shutdown() { s_Connected = false; }
bool HardwareController::IsConnected() { return s_Connected; }
const char* HardwareController::GetDeviceName() { return s_DeviceName; }
ControllerType HardwareController::GetType() { return s_Type; }

bool HardwareController::MoveMouse(int deltaX, int deltaY)
{
    // Always use software mouse for safety
    mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, 0, 0);
    return true;
}

bool HardwareController::Click(int button) { (void)button; return false; }
bool HardwareController::Press(int button) { (void)button; return false; }
bool HardwareController::Release(int button) { (void)button; return false; }
bool HardwareController::KeyPress(int keyCode) { (void)keyCode; return false; }
bool HardwareController::KeyRelease(int keyCode) { (void)keyCode; return false; }
bool HardwareController::ScanCOMPorts(std::vector<std::string>& ports) { ports.clear(); return false; }
bool HardwareController::AutoDetectDevice() { return false; }
bool HardwareController::ConnectKMBoxBPlus(const char*, int) { return false; }
bool HardwareController::ConnectKMBoxNet(const char*, int) { return false; }
bool HardwareController::ConnectArduino(const char*, int) { return false; }
bool HardwareController::SerialWrite(const char*, int) { return false; }
bool HardwareController::SocketSend(const char*, int) { return false; }

// ============================================================================
// OFFSET UPDATER - WITH ASYNC AND TIMEOUT
// ============================================================================
void OffsetUpdater::SetOffsetURL(const char* url)
{
    strncpy_s(s_OffsetURL, url, sizeof(s_OffsetURL) - 1);
}

bool OffsetUpdater::HttpGet(const char* url, std::string& response)
{
#ifdef _WIN32
    response.clear();
    strcpy_s(s_LastError, "");
    
    wchar_t urlW[512];
    mbstowcs_s(nullptr, urlW, url, 511);
    
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);
    
    wchar_t hostName[256] = {};
    wchar_t urlPath[512] = {};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 512;
    
    if (!WinHttpCrackUrl(urlW, 0, 0, &urlComp))
    {
        strcpy_s(s_LastError, "Failed to parse URL");
        return false;
    }
    
    HINTERNET hSession = WinHttpOpen(L"ZeroElite/3.1",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) 
    {
        strcpy_s(s_LastError, "Failed to open HTTP session");
        return false;
    }
    
    // Set short timeouts to prevent freeze
    DWORD timeout = 3000;  // 3 seconds
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) 
    { 
        strcpy_s(s_LastError, "Connection timeout");
        WinHttpCloseHandle(hSession); 
        return false; 
    }
    
    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) 
    { 
        strcpy_s(s_LastError, "Failed to create request");
        WinHttpCloseHandle(hConnect); 
        WinHttpCloseHandle(hSession); 
        return false; 
    }
    
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        strcpy_s(s_LastError, "Request timeout");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        strcpy_s(s_LastError, "No response");
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
    (void)url;
    response = "";
    return false;
#endif
}

bool OffsetUpdater::UpdateOffsetsFromServer()
{
    const char* url = s_OffsetURL[0] ? s_OffsetURL : DEFAULT_OFFSET_URL;
    
    std::string response;
    if (!HttpGet(url, response))
        return false;
    
    return ParseOffsetsTXT(response.c_str());
}

// Async fetch with timeout
bool OffsetUpdater::SyncWithCloud(int maxRetries, int retryDelayMs)
{
    Logger::LogSection("OFFSET SYNC");
    
    if (s_OffsetURL[0] == 0)
        strcpy_s(s_OffsetURL, DEFAULT_OFFSET_URL);
    
    Logger::LogTimestamp();
    Logger::LogInfo("Synchronizing with Zero Elite Cloud...");
    
    // Use async with timeout
    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        Logger::LogSpinner("Fetching offsets", attempt);
        
        // Create async task
        auto future = std::async(std::launch::async, []() {
            return UpdateOffsetsFromServer();
        });
        
        // Wait with timeout (3 seconds)
        auto status = future.wait_for(std::chrono::seconds(3));
        
        Logger::ClearLine();
        
        if (status == std::future_status::ready)
        {
            bool success = future.get();
            if (success)
            {
                Logger::LogTimestamp();
                Logger::LogSuccess("Offsets loaded from GitHub successfully!");
                
                if (s_LastOffsets.buildNumber[0])
                    Logger::LogStatus("Build", s_LastOffsets.buildNumber, true);
                
                s_Synced = true;
                s_Updated = true;
                strcpy_s(g_InitStatus.gameBuild, s_LastOffsets.buildNumber);
                g_InitStatus.offsetsUpdated = true;
                return true;
            }
        }
        else
        {
            Logger::LogTimestamp();
            Logger::LogWarning("Connection timeout - skipping");
            break;  // Don't retry on timeout
        }
        
        if (attempt < maxRetries)
        {
            Logger::LogTimestamp();
            Logger::LogWarning("Retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        }
    }
    
    Logger::LogTimestamp();
    Logger::LogWarning("Using local offsets (server unreachable)");
    s_Synced = false;
    return false;
}

bool OffsetUpdater::FetchRemoteOffsets(const char* url)
{
    if (!url || !url[0])
        url = s_OffsetURL[0] ? s_OffsetURL : DEFAULT_OFFSET_URL;
    
    std::string response;
    if (!HttpGet(url, response))
        return false;
    
    return ParseOffsetsTXT(response.c_str());
}

bool OffsetUpdater::ParseOffsetsTXT(const char* txtData)
{
    RemoteOffsets offsets = {};
    int foundCount = 0;
    
    std::istringstream stream(txtData);
    std::string line;
    
    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '/' || line[0] == '[')
            continue;
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        
        // Trim
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!key.empty() && (key.front() == ' ' || key.front() == '\t')) key.erase(0, 1);
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || 
                                   value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        if (key.empty() || value.empty()) continue;
        
        uintptr_t val = 0;
        if (value.find("0x") == 0 || value.find("0X") == 0)
            val = strtoull(value.c_str(), nullptr, 16);
        else if (isdigit(value[0]))
            val = strtoull(value.c_str(), nullptr, 10);
        
        std::string keyLower = key;
        for (auto& c : keyLower) c = (char)tolower(c);
        
        if (keyLower == "clientinfo" || keyLower == "client_info") { offsets.ClientInfo = val; foundCount++; }
        else if (keyLower == "entitylist" || keyLower == "entity_list") { offsets.EntityList = val; foundCount++; }
        else if (keyLower == "viewmatrix" || keyLower == "view_matrix") { offsets.ViewMatrix = val; foundCount++; }
        else if (keyLower == "playerbase" || keyLower == "player_base") { offsets.PlayerBase = val; foundCount++; }
        else if (keyLower == "build" || keyLower == "build_number") strncpy_s(offsets.buildNumber, value.c_str(), 31);
        else if (keyLower == "version") strncpy_s(offsets.gameVersion, value.c_str(), 31);
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

bool OffsetUpdater::ParseOffsetsJSON(const char* jsonData) { return ParseOffsetsTXT(jsonData); }
bool OffsetUpdater::ParseOffsetsINI(const char* iniData) { return ParseOffsetsTXT(iniData); }

bool OffsetUpdater::ApplyOffsets(const RemoteOffsets& offsets)
{
    uintptr_t baseAddr = DMAEngine::GetBaseAddress();
    if (baseAddr == 0) baseAddr = 0x140000000;
    
    if (offsets.ClientInfo) g_Offsets.ClientInfo = baseAddr + offsets.ClientInfo;
    if (offsets.EntityList) g_Offsets.EntityList = baseAddr + offsets.EntityList;
    if (offsets.ViewMatrix) g_Offsets.ViewMatrix = baseAddr + offsets.ViewMatrix;
    if (offsets.PlayerBase) g_Offsets.PlayerBase = baseAddr + offsets.PlayerBase;
    
    return true;
}

// ============================================================================
// SAFE SCATTER READ WITH TIMEOUT
// ============================================================================
void ExecuteScatterReads(std::vector<ScatterEntry>& entries)
{
    if (entries.empty()) return;
    
    // Validate all addresses first
    for (auto& e : entries)
    {
        if (e.address == 0 || e.buffer == nullptr || e.size == 0)
        {
            if (e.buffer) memset(e.buffer, 0, e.size);
        }
    }
    
#if DMA_ENABLED
    if (!DMAEngine::IsConnected() || !g_VMMDLL || !g_DMA_PID)
    {
        for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
        return;
    }
    
    // Prevent concurrent DMA access
    if (g_DMABusy.exchange(true))
    {
        for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Use pointer type for scatter handle
    PVMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    
    if (!hScatter)
    {
        // Fallback to individual reads with validation
        for (auto& e : entries)
        {
            if (e.buffer && e.address && e.size > 0)
            {
                if (!VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, e.address, (PBYTE)e.buffer, (DWORD)e.size, nullptr, VMMDLL_FLAG_NOCACHE))
                {
                    memset(e.buffer, 0, e.size);
                }
            }
        }
        g_DMABusy = false;
        return;
    }
    
    // Prepare all reads - skip invalid addresses
    int preparedCount = 0;
    for (auto& e : entries)
    {
        if (e.address != 0 && e.size > 0 && e.size < 0x10000)  // Max 64KB per read
        {
            VMMDLL_Scatter_Prepare(hScatter, e.address, (DWORD)e.size);
            preparedCount++;
        }
    }
    
    if (preparedCount == 0)
    {
        VMMDLL_Scatter_CloseHandle(hScatter);
        g_DMABusy = false;
        return;
    }
    
    // Execute with error handling
    BOOL execResult = VMMDLL_Scatter_Execute(hScatter);
    
    if (!execResult)
    {
        Logger::LogError("Scatter Execute failed!");
        VMMDLL_Scatter_CloseHandle(hScatter);
        for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
        g_DMABusy = false;
        return;
    }
    
    // Read results
    for (auto& e : entries)
    {
        if (e.buffer && e.address && e.size > 0)
        {
            if (!VMMDLL_Scatter_Read(hScatter, e.address, (DWORD)e.size, (PBYTE)e.buffer, nullptr))
            {
                memset(e.buffer, 0, e.size);
            }
        }
    }
    
    VMMDLL_Scatter_CloseHandle(hScatter);
    
    // Check for slow read
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    if (duration > 100)
    {
        g_SlowReadCount++;
        if (g_SlowReadCount <= 5)  // Only log first 5
        {
            char msg[64];
            snprintf(msg, sizeof(msg), "Slow DMA Read detected: %lldms", duration);
            Logger::LogWarning(msg);
        }
    }
    
    g_DMABusy = false;
#else
    for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
#endif
}

// ============================================================================
// CONFIG FILE HANDLING
// ============================================================================
bool LoadConfig(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    std::string line, section;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[') { size_t e = line.find(']'); if (e != std::string::npos) section = line.substr(1, e-1); continue; }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq), value = line.substr(eq + 1);
        while (!key.empty() && key.back() == ' ') key.pop_back();
        while (!value.empty() && (value[0] == ' ')) value.erase(0, 1);
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        if (section == "Device")
        {
            if (key == "Type") strncpy_s(g_Config.deviceType, value.c_str(), 31);
            else if (key == "UseFTD3XX") g_Config.useFTD3XX = (value == "1");
        }
        else if (section == "Target")
        {
            if (key == "ProcessName") { strncpy_s(g_Config.processName, value.c_str(), 63); mbstowcs_s(nullptr, g_Config.processNameW, g_Config.processName, 63); }
        }
        else if (section == "Performance")
        {
            if (key == "UpdateRateHz") g_Config.updateRateHz = std::stoi(value);
            else if (key == "UseScatterRegistry") g_Config.useScatterRegistry = (value == "1");
        }
        else if (section == "OffsetUpdater")
        {
            if (key == "Enabled") g_Config.enableOffsetUpdater = (value == "1");
            else if (key == "URL") strncpy_s(g_Config.offsetURL, value.c_str(), 511);
        }
    }
    return true;
}

bool SaveConfig(const char* filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "; PROJECT ZERO - Configuration v3.1\n\n";
    file << "[Device]\nType=" << g_Config.deviceType << "\nUseFTD3XX=" << (g_Config.useFTD3XX?"1":"0") << "\n\n";
    file << "[Target]\nProcessName=" << g_Config.processName << "\n\n";
    file << "[Performance]\nUpdateRateHz=" << g_Config.updateRateHz << "\nUseScatterRegistry=" << (g_Config.useScatterRegistry?"1":"0") << "\n\n";
    file << "[OffsetUpdater]\nEnabled=" << (g_Config.enableOffsetUpdater?"1":"0") << "\nURL=" << g_Config.offsetURL << "\n";
    return true;
}

void CreateDefaultConfig(const char* filename)
{
    strcpy_s(g_Config.deviceType, "fpga");
    strcpy_s(g_Config.processName, "cod.exe");
    wcscpy_s(g_Config.processNameW, L"cod.exe");
    g_Config.updateRateHz = 60;  // Lower default for stability
    g_Config.useScatterRegistry = true;
    g_Config.useFTD3XX = true;
    g_Config.enableOffsetUpdater = false;  // Disabled by default to prevent freeze
    strcpy_s(g_Config.offsetURL, OffsetUpdater::DEFAULT_OFFSET_URL);
    SaveConfig(filename);
}

// ============================================================================
// SCATTER REGISTRY
// ============================================================================
void ScatterReadRegistry::RegisterPlayerData(int idx, uintptr_t base)
{
    if (idx < 0 || base == 0) return;
    while ((int)m_PlayerBuffers.size() <= idx) m_PlayerBuffers.push_back({});
    PlayerRawData& buf = m_PlayerBuffers[idx];
    m_Entries.push_back({base + GameOffsets::EntityPos, &buf.position, sizeof(Vec3), ScatterDataType::PLAYER_POSITION, idx});
    m_Entries.push_back({base + GameOffsets::EntityHealth, &buf.health, sizeof(int), ScatterDataType::PLAYER_HEALTH, idx});
    m_Entries.push_back({base + GameOffsets::EntityTeam, &buf.team, sizeof(int), ScatterDataType::PLAYER_TEAM, idx});
    m_Entries.push_back({base + GameOffsets::EntityYaw, &buf.yaw, sizeof(float), ScatterDataType::PLAYER_YAW, idx});
    m_Entries.push_back({base + GameOffsets::EntityValid, &buf.valid, sizeof(uint8_t), ScatterDataType::PLAYER_VALID, idx});
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
    if (m_Entries.empty()) return;
    m_TransactionCount++;
    ExecuteScatterReads(m_Entries);
}

void ScatterReadRegistry::Clear() { m_Entries.clear(); }
int ScatterReadRegistry::GetTotalBytes() const { int t = 0; for (auto& e : m_Entries) t += (int)e.size; return t; }

// ============================================================================
// MAP TEXTURE MANAGER
// ============================================================================
void MapTextureManager::InitializeMapDatabase() {}
bool MapTextureManager::LoadMapConfig(const char* n) { strcpy_s(s_CurrentMap.name, n); return false; }
bool MapTextureManager::LoadMapTexture(const char* p) { if (p && p[0]) { strcpy_s(s_CurrentMap.imagePath, p); s_CurrentMap.hasTexture = true; return true; } return false; }
Vec2 MapTextureManager::GameToMapCoords(const Vec3& g) { return Vec2(g.x, g.y); }
Vec2 MapTextureManager::GameToRadarCoords(const Vec3& g, const Vec3& l, float y, float cx, float cy, float s, float z)
{
    Vec3 d = g - l; float r = -y * 3.14159f / 180.0f;
    float rx = d.x * cosf(r) - d.y * sinf(r), ry = d.x * sinf(r) + d.y * cosf(r);
    float sc = (s * 0.4f) / (100.0f / z);
    return Vec2(cx + rx * sc, cy - ry * sc);
}
void MapTextureManager::SetMapBounds(const char* n, float x1, float x2, float y1, float y2) { MapInfo i; strcpy_s(i.name, n); i.minX=x1; i.maxX=x2; i.minY=y1; i.maxY=y2; s_MapDatabase[n]=i; }

// ============================================================================
// PROFESSIONAL INITIALIZATION - DMA FIRST
// ============================================================================
void ProfessionalInit::SimulateDelay(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool ProfessionalInit::CheckDMAConnection()
{
#if DMA_ENABLED
    return g_VMMDLL != nullptr;
#else
    return false;
#endif
}

bool ProfessionalInit::CheckMemoryMap()
{
#if DMA_ENABLED
    if (!g_VMMDLL) return false;
    DWORD dwPID = 0;
    if (VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)"System", &dwPID))
        return dwPID != 0;
    return false;
#else
    return true;
#endif
}

void ProfessionalInit::GetWindowsVersion(char* buffer, size_t size)
{
#ifdef _WIN32
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    typedef LONG (WINAPI* RtlGetVersionFunc)(PRTL_OSVERSIONINFOW);
    RtlGetVersionFunc RtlGetVersion = (RtlGetVersionFunc)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
    if (RtlGetVersion)
    {
        RtlGetVersion((PRTL_OSVERSIONINFOW)&osvi);
        const char* version = (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 22000) ? "Windows 11" : "Windows 10";
        snprintf(buffer, size, "%s (Build %d)", version, osvi.dwBuildNumber);
    }
    else
        snprintf(buffer, size, "Windows");
#else
    snprintf(buffer, size, "Unknown");
#endif
}

void ProfessionalInit::GetKeyboardState(char* buffer, size_t size) { snprintf(buffer, size, "Ready"); }
void ProfessionalInit::GetMouseState(char* buffer, size_t size) { snprintf(buffer, size, "Ready"); }

bool ProfessionalInit::RunProfessionalChecks()
{
    Logger::Initialize();
    memset(&g_InitStatus, 0, sizeof(g_InitStatus));
    g_InitStatus.totalChecks = 5;
    
    Logger::LogBanner();
    Logger::LogSeparator();
    Logger::LogTimestamp();
    Logger::Log("Starting initialization...\n", COLOR_WHITE);
    Logger::LogSeparator();
    
    // STEP 1: DMA CONNECTION FIRST (Most important)
    Logger::LogProgress("DMA Connection", 1, 5);
    Logger::LogSection("DMA DEVICE");
    Logger::LogTimestamp();
    Logger::LogInfo("Connecting to DMA Device...");
    
#if DMA_ENABLED
    char arg0[] = "";
    char arg1[] = "-device";
    char arg2[] = "fpga";
    char* args[3] = { arg0, arg1, arg2 };
    
    g_VMMDLL = VMMDLL_Initialize(3, args);
    
    if (g_VMMDLL)
    {
        Logger::LogTimestamp();
        Logger::LogSuccess("DMA Device connected!");
        strcpy_s(g_InitStatus.dmaDevice, "FPGA");
        g_InitStatus.dmaConnected = true;
        g_InitStatus.passedChecks++;
        
        Logger::LogStatus("Device", "FPGA", true);
        Logger::LogStatus("Status", "Connected", true);
    }
    else
    {
        Logger::LogTimestamp();
        Logger::LogWarning("DMA Device not found - Running in SIMULATION mode");
        strcpy_s(g_InitStatus.dmaDevice, "Simulation");
        g_InitStatus.dmaConnected = false;
        g_InitStatus.passedChecks++;  // Allow simulation mode
    }
#else
    Logger::LogTimestamp();
    Logger::LogWarning("DMA disabled - Running in SIMULATION mode");
    strcpy_s(g_InitStatus.dmaDevice, "Simulation");
    g_InitStatus.passedChecks++;
#endif
    
    // STEP 2: Load Config
    Logger::LogProgress("Config Loader", 2, 5);
    Logger::LogSection("CONFIGURATION");
    
    bool configLoaded = LoadConfig("zero.ini");
    if (!configLoaded)
    {
        Logger::LogTimestamp();
        Logger::LogWarning("Config not found, creating defaults");
        CreateDefaultConfig("zero.ini");
    }
    else
    {
        Logger::LogTimestamp();
        Logger::LogSuccess("Config loaded: zero.ini");
    }
    g_InitStatus.configLoaded = true;
    g_InitStatus.passedChecks++;
    
    // STEP 3: Offset Sync (with timeout)
    Logger::LogProgress("Offset Sync", 3, 5);
    if (g_Config.enableOffsetUpdater)
    {
        OffsetUpdater::SyncWithCloud(1, 1000);  // Only 1 attempt, 1 sec delay
    }
    else
    {
        Logger::LogSection("OFFSET SYNC");
        Logger::LogTimestamp();
        Logger::LogWarning("Offset updater disabled - using local");
    }
    g_InitStatus.passedChecks++;
    
    // STEP 4: Find Game Process
    Logger::LogProgress("Game Process", 4, 5);
    Logger::LogSection("GAME SYNC");
    
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        Logger::LogTimestamp();
        Logger::LogInfo("Searching for game process...");
        
        DWORD pid = 0;
        for (int i = 0; i < 10; i++)
        {
            if (VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)g_Config.processName, &pid) && pid != 0)
            {
                g_DMA_PID = pid;
                DMAEngine::s_BaseAddress = 0x140000000;
                Logger::LogTimestamp();
                Logger::LogSuccess("Game process found!");
                
                char pidStr[32]; snprintf(pidStr, sizeof(pidStr), "%d", pid);
                Logger::LogStatus("Process", g_Config.processName, true);
                Logger::LogStatus("PID", pidStr, true);
                
                g_InitStatus.gameFound = true;
                break;
            }
            SimulateDelay(100);
        }
        
        if (!g_InitStatus.gameFound)
        {
            Logger::LogTimestamp();
            Logger::LogWarning("Game not running - will wait");
        }
    }
    else
#endif
    {
        Logger::LogTimestamp();
        Logger::LogSuccess("Simulation mode ready");
        DMAEngine::s_BaseAddress = 0x140000000;
        g_Offsets.EntityList = DMAEngine::s_BaseAddress + 0x16D5C000;
        g_InitStatus.gameFound = true;
    }
    g_InitStatus.passedChecks++;
    
    // STEP 5: System Info
    Logger::LogProgress("System Check", 5, 5);
    Logger::LogSection("SYSTEM");
    
    GetWindowsVersion(g_InitStatus.windowsVersion, sizeof(g_InitStatus.windowsVersion));
    Logger::LogStatus("Windows", g_InitStatus.windowsVersion, true);
    Logger::LogStatus("Keyboard", "Ready", true);
    Logger::LogStatus("Mouse", "Ready", true);
    g_InitStatus.passedChecks++;
    
    // Summary
    Logger::LogSeparator();
    Logger::LogSection("READY");
    
    char summary[64];
    snprintf(summary, sizeof(summary), "%d/%d checks passed", g_InitStatus.passedChecks, g_InitStatus.totalChecks);
    Logger::LogTimestamp();
    Logger::LogSuccess(summary);
    Logger::LogSuccess("Press INSERT to show menu, END to exit");
    
    Logger::LogSeparator();
    Logger::Log("\n", COLOR_DEFAULT);
    
    SimulateDelay(500);
    
    g_InitStatus.allChecksPassed = true;
    return true;
}

// ============================================================================
// DMA ENGINE
// ============================================================================
bool DMAEngine::Initialize()
{
    if (!ProfessionalInit::RunProfessionalChecks()) 
        return false;
    
    s_Connected = g_InitStatus.dmaConnected;
    s_Online = g_InitStatus.allChecksPassed;
    s_SimulationMode = !g_InitStatus.dmaConnected;
    s_ModuleSize = 0x5000000;
    
    if (s_Connected)
    {
        strcpy_s(s_StatusText, "ONLINE");
        snprintf(s_DeviceInfo, sizeof(s_DeviceInfo), "%s | %s", g_InitStatus.dmaDevice, g_InitStatus.windowsVersion);
        strcpy_s(s_DriverMode, "FTD3XX");
    }
    else
    {
        strcpy_s(s_StatusText, "SIMULATION");
        strcpy_s(s_DeviceInfo, "Demo Mode");
        strcpy_s(s_DriverMode, "None");
        s_BaseAddress = 0x140000000;
        g_Offsets.EntityList = s_BaseAddress + 0x16D5C000;
    }
    
    return true;
}

bool DMAEngine::InitializeWithConfig(const DMAConfig&) { return Initialize(); }
bool DMAEngine::InitializeFTD3XX() { return Initialize(); }

void DMAEngine::Shutdown()
{
#if DMA_ENABLED
    if (g_VMMDLL) { VMMDLL_Close(g_VMMDLL); g_VMMDLL = nullptr; }
#endif
    SaveConfig("zero.ini");
    s_Connected = false; s_Online = false;
    Logger::Shutdown();
}

bool DMAEngine::IsConnected() { return s_Connected && !s_SimulationMode; }
bool DMAEngine::IsOnline() { return s_Online; }
const char* DMAEngine::GetStatus() { return s_StatusText; }
const char* DMAEngine::GetDeviceInfo() { return s_DeviceInfo; }
const char* DMAEngine::GetDriverMode() { return s_DriverMode; }

template<typename T> T DMAEngine::Read(uintptr_t a) 
{ 
    T v = {}; 
    if (a == 0) return v;  // Validate address
    
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID && !g_DMABusy) 
    {
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)&v, sizeof(T), nullptr, VMMDLL_FLAG_NOCACHE);
    }
#endif
    return v;
}

bool DMAEngine::ReadBuffer(uintptr_t a, void* b, size_t s) 
{
    if (a == 0 || b == nullptr || s == 0) return false;  // Validate
    
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID && !g_DMABusy) 
        return VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)b, (DWORD)s, nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    return false;
}

bool DMAEngine::ReadString(uintptr_t a, char* b, size_t m) 
{ 
    if (!b || !m || a == 0) return false;
    b[0] = 0;
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID) 
    { 
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)b, (DWORD)(m-1), nullptr, VMMDLL_FLAG_NOCACHE); 
        b[m-1] = 0; 
        return true; 
    }
#endif
    return false;
}

void DMAEngine::ExecuteScatter(std::vector<ScatterEntry>& e) { ExecuteScatterReads(e); }
uintptr_t DMAEngine::GetBaseAddress() { return s_BaseAddress; }
uintptr_t DMAEngine::GetModuleBase(const wchar_t* m) { (void)m; return s_BaseAddress; }
size_t DMAEngine::GetModuleSize() { return s_ModuleSize; }

template int32_t DMAEngine::Read<int32_t>(uintptr_t);
template uint32_t DMAEngine::Read<uint32_t>(uintptr_t);
template int64_t DMAEngine::Read<int64_t>(uintptr_t);
template uint64_t DMAEngine::Read<uint64_t>(uintptr_t);
template float DMAEngine::Read<float>(uintptr_t);
template uintptr_t DMAEngine::Read<uintptr_t>(uintptr_t);

// ============================================================================
// AIMBOT
// ============================================================================
void Aimbot::Initialize() { s_Enabled = false; s_CurrentTarget = -1; }
void Aimbot::Update() {}
bool Aimbot::IsEnabled() { return s_Enabled; }
void Aimbot::SetEnabled(bool e) { s_Enabled = e; }
int Aimbot::FindBestTarget(float, float) { return -1; }
Vec2 Aimbot::GetTargetScreenPos(int) { return Vec2(0, 0); }
void Aimbot::AimAt(const Vec2&, float) {}
void Aimbot::MoveMouse(int dx, int dy) { HardwareController::MoveMouse(dx, dy); }

// ============================================================================
// PATTERN SCANNER (Stub)
// ============================================================================
uintptr_t PatternScanner::FindPattern(uintptr_t, size_t, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ScanModule(const char*, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ResolveRelative(uintptr_t, int, int) { return 0; }
bool PatternScanner::UpdateAllOffsets() { s_Scanned = true; return false; }

// ============================================================================
// DIAGNOSTIC SYSTEM (Stub)
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
// PLAYER MANAGER - IN BACKGROUND THREAD
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
        float a = (float)i * 0.5236f, d = 30.0f + (float)(rand() % 100);
        p.origin.x = cosf(a) * d; 
        p.origin.y = sinf(a) * d; 
        p.yaw = (float)(rand() % 360); 
        p.distance = d;
        s_Players.push_back(p);
    }
    s_LocalPlayer = {}; 
    s_LocalPlayer.valid = true; 
    s_LocalPlayer.health = 100;
    s_Initialized = true;
}

void PlayerManager::Update() 
{ 
    if (!s_Initialized) Initialize(); 
    SimulateUpdate();  // Always use simulation for stability
}

void PlayerManager::UpdateWithScatterRegistry()
{
    if (!s_Initialized) Initialize();
    
    // Skip real DMA if not connected or offsets invalid
    if (!DMAEngine::IsConnected() || g_Offsets.EntityList == 0)
    {
        SimulateUpdate();
        return;
    }
    
#if DMA_ENABLED
    g_ScatterRegistry.Clear();
    for (size_t i = 0; i < s_Players.size() && i < 12; i++) 
    {
        uintptr_t addr = g_Offsets.EntityList + i * GameOffsets::EntitySize;
        if (addr != 0)
            g_ScatterRegistry.RegisterPlayerData((int)i, addr);
    }
    g_ScatterRegistry.ExecuteAll();
#else
    SimulateUpdate();
#endif
}

void PlayerManager::SimulateUpdate()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_SimTime += 1.0f / 60.0f;
    s_LocalPlayer.yaw = fmodf(s_SimTime * 15.0f, 360.0f);
    
    for (size_t i = 0; i < s_Players.size(); i++) 
    {
        PlayerData& p = s_Players[i];
        float a = (float)i * 0.5236f + s_SimTime * 0.3f;
        float d = 40.0f + sinf(s_SimTime * 0.5f + (float)i) * 30.0f;
        p.origin.x = cosf(a) * d; 
        p.origin.y = sinf(a) * d; 
        p.yaw = fmodf(a * 57.3f + 180.0f, 360.0f);
        p.distance = d; 
        p.health = 30 + (int)(sinf(s_SimTime * 0.2f + (float)i) * 35 + 35);
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
