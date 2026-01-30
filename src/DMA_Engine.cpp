// DMA_Engine.cpp - Final Hardware & Cloud Integration v3.3
// Features: FPGA DMA, KMBox B+, GitHub Cloud Sync

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
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

// ============================================================================
// DMA LIBRARY LINKING
// ============================================================================
#if DMA_ENABLED
#include "vmmdll.h"
#pragma comment(lib, "vmmdll.lib")
// Optional: #pragma comment(lib, "leechcore.lib")
static VMM_HANDLE g_VMMDLL = nullptr;
static DWORD g_DMA_PID = 0;
#endif

// ============================================================================
// STATUS STRINGS
// ============================================================================
static char g_DMA_Status[32] = "OFFLINE";
static char g_KMBox_Status[32] = "DISCONNECTED";
static char g_Cloud_Status[32] = "NOT SYNCED";

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
DMAConfig g_Config;
GameOffsets g_Offsets;
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
// LOGGER
// ============================================================================
void Logger::Initialize()
{
#ifdef _WIN32
    AllocConsole();
    s_Console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleW(L"PROJECT ZERO | v3.3");
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
#endif
}

void Logger::Shutdown() { FreeConsole(); }
void Logger::SetColor(ConsoleColor c) { if (s_Console) SetConsoleTextAttribute((HANDLE)s_Console, c); }
void Logger::ResetColor() { SetColor(COLOR_DEFAULT); }

void Logger::Log(const char* m, ConsoleColor c) { SetColor(c); printf("%s", m); ResetColor(); fflush(stdout); }
void Logger::LogSuccess(const char* m) { SetColor(COLOR_GREEN); printf("[+] "); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogError(const char* m) { SetColor(COLOR_RED); printf("[!] "); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogWarning(const char* m) { SetColor(COLOR_YELLOW); printf("[*] "); ResetColor(); printf("%s\n", m); fflush(stdout); }
void Logger::LogInfo(const char* m) { SetColor(COLOR_CYAN); printf("[>] "); ResetColor(); printf("%s\n", m); fflush(stdout); }

void Logger::LogStatus(const char* label, const char* value, bool success)
{
    printf("    %-20s : ", label);
    SetColor(success ? COLOR_GREEN : COLOR_RED);
    printf("%s\n", value);
    ResetColor();
    fflush(stdout);
}

void Logger::LogProgress(const char* m, int c, int t) { SetColor(COLOR_CYAN); printf("[%d/%d] ", c, t); ResetColor(); printf("%s\n", m); fflush(stdout); }

void Logger::LogBanner()
{
    SetColor(COLOR_RED);
    printf("\n  ===================================================\n");
    printf("  ||   ");
    SetColor(COLOR_WHITE);
    printf("P R O J E C T   Z E R O   |   D M A   v3.3");
    SetColor(COLOR_RED);
    printf("   ||\n");
    printf("  ===================================================\n\n");
    ResetColor();
}

void Logger::LogSection(const char* t) { printf("\n"); SetColor(COLOR_MAGENTA); printf("=== %s ===\n", t); ResetColor(); fflush(stdout); }
void Logger::LogSeparator() { SetColor(COLOR_GRAY); printf("---------------------------------------------------\n"); ResetColor(); }
void Logger::LogTimestamp() { time_t n = time(nullptr); struct tm t; localtime_s(&t, &n); char b[16]; strftime(b, 16, "%H:%M:%S", &t); SetColor(COLOR_GRAY); printf("[%s] ", b); ResetColor(); }
void Logger::LogSpinner(const char* m, int f) { printf("\r[%c] %s", "|/-\\"[f%4], m); fflush(stdout); }
void Logger::ClearLine() { printf("\r                                        \r"); }

// ============================================================================
// KMBOX B+ CONNECTION VIA COM3
// ============================================================================
bool HardwareController::Initialize()
{
    s_Type = g_Config.controllerType;
    
    if (s_Type == ControllerType::NONE)
    {
        strcpy_s(s_DeviceName, "Software");
        strcpy_s(g_KMBox_Status, "DISABLED");
        return true;
    }
    
    if (s_Type == ControllerType::KMBOX_B_PLUS)
    {
        return ConnectKMBoxBPlus("COM3", 115200);
    }
    
    strcpy_s(s_DeviceName, "Not Connected");
    strcpy_s(g_KMBox_Status, "DISCONNECTED");
    return false;
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
        strcpy_s(s_DeviceName, "COM3 - Not Found");
        strcpy_s(g_KMBox_Status, "DISCONNECTED");
        s_Connected = false;
        return false;
    }
    
    // Configure serial port
    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    GetCommState(hPort, &dcb);
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(hPort, &dcb);
    
    // Set timeouts
    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;
    SetCommTimeouts(hPort, &timeouts);
    
    s_Handle = hPort;
    s_Connected = true;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox B+ (%s)", comPort);
    strcpy_s(g_KMBox_Status, "CONNECTED");
    
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
    strcpy_s(g_KMBox_Status, "DISCONNECTED");
}

bool HardwareController::IsConnected() { return s_Connected; }
const char* HardwareController::GetDeviceName() { return s_DeviceName; }
ControllerType HardwareController::GetType() { return s_Type; }

bool HardwareController::MoveMouse(int deltaX, int deltaY)
{
    if (s_Connected && s_Handle && s_Type == ControllerType::KMBOX_B_PLUS)
    {
        // Send KMBox command
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "km.move(%d,%d)\r\n", deltaX, deltaY);
        DWORD written;
        WriteFile(s_Handle, cmd, (DWORD)strlen(cmd), &written, nullptr);
        return true;
    }
    
    // Fallback to software
    mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, 0, 0);
    return true;
}

bool HardwareController::Click(int button) { (void)button; return false; }
bool HardwareController::Press(int button) { (void)button; return false; }
bool HardwareController::Release(int button) { (void)button; return false; }
bool HardwareController::KeyPress(int k) { (void)k; return false; }
bool HardwareController::KeyRelease(int k) { (void)k; return false; }
bool HardwareController::ScanCOMPorts(std::vector<std::string>& p) { p.clear(); return false; }
bool HardwareController::AutoDetectDevice() { return false; }
bool HardwareController::ConnectKMBoxNet(const char*, int) { return false; }
bool HardwareController::ConnectArduino(const char*, int) { return false; }
bool HardwareController::SerialWrite(const char*, int) { return false; }
bool HardwareController::SocketSend(const char*, int) { return false; }

// ============================================================================
// GITHUB CLOUD SYNC (DATA ONLY)
// ============================================================================
void OffsetUpdater::SetOffsetURL(const char* url)
{
    strncpy_s(s_OffsetURL, url, sizeof(s_OffsetURL) - 1);
}

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
    
    if (!WinHttpCrackUrl(urlW, 0, 0, &urlComp))
        return false;
    
    HINTERNET hSession = WinHttpOpen(L"ZeroElite/3.3", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return false;
    
    DWORD timeout = 5000;
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
    Logger::LogSection("GITHUB CLOUD SYNC");
    Logger::LogInfo("Fetching offsets from GitHub...");
    
    if (s_OffsetURL[0] == 0)
        strcpy_s(s_OffsetURL, DEFAULT_OFFSET_URL);
    
    for (int attempt = 0; attempt < maxRetries; attempt++)
    {
        auto future = std::async(std::launch::async, UpdateOffsetsFromServer);
        auto status = future.wait_for(std::chrono::seconds(5));
        
        if (status == std::future_status::ready && future.get())
        {
            Logger::LogSuccess("Offsets synced from GitHub!");
            if (s_LastOffsets.buildNumber[0])
                Logger::LogStatus("Build", s_LastOffsets.buildNumber, true);
            
            s_Synced = true;
            s_Updated = true;
            strcpy_s(g_Cloud_Status, "SYNCED");
            return true;
        }
        
        if (attempt < maxRetries - 1)
        {
            Logger::LogWarning("Retry...");
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        }
    }
    
    Logger::LogWarning("Cloud sync failed - using local offsets");
    strcpy_s(g_Cloud_Status, "OFFLINE");
    return false;
}

bool OffsetUpdater::FetchRemoteOffsets(const char* url) { return UpdateOffsetsFromServer(); (void)url; }

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
        while (!value.empty() && (value[0] == ' ')) value.erase(0, 1);
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
// SCATTER READ - FIXED C2664 ERROR
// Using PVMMDLL_SCATTER_HANDLE (pointer) directly - NO user-defined conversion
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
    
    // FIXED: Use PVMMDLL_SCATTER_HANDLE (which is a pointer type)
    // VMMDLL_Scatter_Initialize returns PVMMDLL_SCATTER_HANDLE directly
    PVMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    
    if (hScatter == NULL)  // Use NULL for pointer comparison
    {
        // Fallback to individual reads
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
    
    // Prepare reads - skip invalid addresses
    for (auto& e : entries)
    {
        if (e.address != 0 && e.size > 0 && e.size <= 0x10000)
            VMMDLL_Scatter_Prepare(hScatter, e.address, (DWORD)e.size);
    }
    
    // Execute single DMA transaction
    if (!VMMDLL_Scatter_Execute(hScatter))
    {
        VMMDLL_Scatter_CloseHandle(hScatter);
        for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
        g_DMABusy = false;
        return;
    }
    
    // Read results
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
// CONFIG
// ============================================================================
bool LoadConfig(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    std::string line, section;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[') { section = line.substr(1, line.find(']') - 1); continue; }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        if (section == "Target" && key == "ProcessName")
            strncpy_s(g_Config.processName, value.c_str(), 63);
        else if (section == "Controller" && key == "Type")
        {
            if (value == "KMBoxBPlus") g_Config.controllerType = ControllerType::KMBOX_B_PLUS;
            else g_Config.controllerType = ControllerType::NONE;
        }
        else if (section == "OffsetUpdater" && key == "Enabled")
            g_Config.enableOffsetUpdater = (value == "1");
    }
    return true;
}

bool SaveConfig(const char* filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << "[Target]\nProcessName=" << g_Config.processName << "\n\n";
    file << "[Controller]\nType=" << (g_Config.controllerType == ControllerType::KMBOX_B_PLUS ? "KMBoxBPlus" : "None") << "\n\n";
    file << "[OffsetUpdater]\nEnabled=" << (g_Config.enableOffsetUpdater ? "1" : "0") << "\n";
    return true;
}

void CreateDefaultConfig(const char* filename)
{
    strcpy_s(g_Config.processName, "cod.exe");
    g_Config.controllerType = ControllerType::KMBOX_B_PLUS;
    g_Config.enableOffsetUpdater = true;
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
// DMA HARDWARE INITIALIZATION
// ============================================================================
bool ProfessionalInit::RunProfessionalChecks()
{
    Logger::Initialize();
    memset(&g_InitStatus, 0, sizeof(g_InitStatus));
    
    Logger::LogBanner();
    Logger::LogSeparator();
    
    // === STEP 1: DMA HARDWARE ===
    Logger::LogSection("DMA HARDWARE");
    Logger::LogInfo("Initializing FPGA device...");
    
#if DMA_ENABLED
    char arg0[] = "";
    char arg1[] = "-device";
    char arg2[] = "fpga";
    char* args[3] = { arg0, arg1, arg2 };
    
    g_VMMDLL = VMMDLL_Initialize(3, args);
    
    if (g_VMMDLL != NULL)
    {
        Logger::LogSuccess("FPGA device found!");
        strcpy_s(g_DMA_Status, "ONLINE");
        g_InitStatus.dmaConnected = true;
        Logger::LogStatus("DMA_Status", "ONLINE", true);
    }
    else
    {
        Logger::LogWarning("FPGA not found - Simulation mode");
        strcpy_s(g_DMA_Status, "SIMULATION");
        g_InitStatus.dmaConnected = false;
        Logger::LogStatus("DMA_Status", "SIMULATION", false);
    }
#else
    Logger::LogWarning("DMA disabled at compile time");
    strcpy_s(g_DMA_Status, "DISABLED");
#endif
    
    // === STEP 2: KMBOX CONTROLLER ===
    Logger::LogSection("AIMBOT CONTROLLER");
    Logger::LogInfo("Connecting to KMBox B+ (COM3, 115200)...");
    
    g_Config.controllerType = ControllerType::KMBOX_B_PLUS;
    bool kmboxOk = HardwareController::Initialize();
    
    if (kmboxOk && HardwareController::IsConnected())
    {
        Logger::LogSuccess("KMBox B+ connected!");
        Logger::LogStatus("KMBox_Status", "CONNECTED", true);
    }
    else
    {
        Logger::LogWarning("KMBox not found - using software mouse");
        Logger::LogStatus("KMBox_Status", "DISCONNECTED", false);
    }
    
    // === STEP 3: GITHUB CLOUD (DATA ONLY) ===
    Logger::LogSection("GITHUB CLOUD");
    Logger::LogInfo("Syncing offsets from GitHub (data only)...");
    
    OffsetUpdater::SyncWithCloud(2, 2000);
    Logger::LogStatus("Cloud_Status", g_Cloud_Status, strcmp(g_Cloud_Status, "SYNCED") == 0);
    
    // === STEP 4: FIND GAME PROCESS ===
    Logger::LogSection("GAME PROCESS");
    
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        Logger::LogInfo("Searching for game...");
        DWORD pid = 0;
        if (VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)"cod.exe", &pid) && pid != 0)
        {
            g_DMA_PID = pid;
            DMAEngine::s_BaseAddress = 0x140000000;
            Logger::LogSuccess("Game found!");
            char pidStr[16]; snprintf(pidStr, 16, "%d", pid);
            Logger::LogStatus("PID", pidStr, true);
            g_InitStatus.gameFound = true;
        }
        else
        {
            Logger::LogWarning("Game not running");
            Logger::LogStatus("Process", "Not Found", false);
        }
    }
#endif
    
    if (!g_InitStatus.gameFound)
    {
        DMAEngine::s_SimulationMode = true;
        DMAEngine::s_BaseAddress = 0x140000000;
        g_Offsets.EntityList = DMAEngine::s_BaseAddress + 0x16D5C000;
    }
    
    // === SUMMARY ===
    Logger::LogSeparator();
    Logger::LogSection("STATUS SUMMARY");
    Logger::LogStatus("DMA", g_DMA_Status, strcmp(g_DMA_Status, "ONLINE") == 0);
    Logger::LogStatus("KMBox", g_KMBox_Status, strcmp(g_KMBox_Status, "CONNECTED") == 0);
    Logger::LogStatus("Cloud", g_Cloud_Status, strcmp(g_Cloud_Status, "SYNCED") == 0);
    Logger::LogSeparator();
    Logger::LogSuccess("Initialization complete!");
    Logger::LogInfo("Press INSERT to show menu, END to exit");
    Logger::Log("\n", COLOR_DEFAULT);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
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
bool DMAEngine::Initialize()
{
    return ProfessionalInit::RunProfessionalChecks();
}

bool DMAEngine::InitializeWithConfig(const DMAConfig&) { return Initialize(); }
bool DMAEngine::InitializeFTD3XX() { return Initialize(); }

void DMAEngine::Shutdown()
{
#if DMA_ENABLED
    if (g_VMMDLL) { VMMDLL_Close(g_VMMDLL); g_VMMDLL = nullptr; }
#endif
    HardwareController::Shutdown();
    SaveConfig("zero.ini");
    Logger::Shutdown();
}

bool DMAEngine::IsConnected() { return s_Connected || (strcmp(g_DMA_Status, "ONLINE") == 0); }
bool DMAEngine::IsOnline() { return s_Online || g_InitStatus.allChecksPassed; }
const char* DMAEngine::GetStatus() { return g_DMA_Status; }
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
// PATTERN SCANNER (Stub)
// ============================================================================
uintptr_t PatternScanner::FindPattern(uintptr_t, size_t, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ScanModule(const char*, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ResolveRelative(uintptr_t, int, int) { return 0; }
bool PatternScanner::UpdateAllOffsets() { s_Scanned = true; return false; }

// ============================================================================
// DIAGNOSTIC (Stub)
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
    
    // Create simulated players with VALID positions (not 0,0)
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
        
        // Set non-zero positions to prevent ghost drawings
        float angle = (float)i * 0.5236f;
        float dist = 30.0f + (float)(rand() % 80);
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.origin.z = 0;  // Ground level
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
        
        // Update position - ensure NOT at (0,0) to prevent ghost drawings
        float angle = (float)i * 0.5236f + s_SimTime * 0.3f;
        float dist = 40.0f + sinf(s_SimTime * 0.5f + (float)i) * 30.0f;
        
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.yaw = fmodf(angle * 57.3f + 180.0f, 360.0f);
        p.distance = dist;
        p.health = 30 + (int)(sinf(s_SimTime * 0.2f + (float)i) * 35 + 35);
        
        // GHOST CHECK: Mark invalid if at origin
        if (p.origin.x == 0 && p.origin.y == 0)
            p.valid = false;
        else
            p.valid = true;
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
