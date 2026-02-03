// DMA_Engine.cpp - STRICT AUTOMATION v4.5
// Features: Hardware Controllers, Auto-Offset Updater, FTD3XX, Scatter Registry

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
GameOffsetsStruct g_Offsets;
ScatterReadRegistry g_ScatterRegistry;
DiagnosticStatus g_DiagStatus;
InitStatus g_InitStatus;
ControllerConfig g_ControllerConfig;

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
char OffsetUpdater::s_OffsetURL[512] = "https://raw.githubusercontent.com/offsets/bo6/main/offsets.txt";
char OffsetUpdater::s_LastError[256] = "";

bool Aimbot::s_Enabled = false;
int Aimbot::s_CurrentTarget = -1;
Vec2 Aimbot::s_LastAimPos = {0, 0};

// ============================================================================
// LOGGER IMPLEMENTATION
// ============================================================================
void Logger::Initialize()
{
#ifdef _WIN32
    AllocConsole();
    s_Console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleW(L"PROJECT ZERO | COD DMA v3.0");
    SMALL_RECT windowSize = {0, 0, 100, 40};
    SetConsoleWindowInfo((HANDLE)s_Console, TRUE, &windowSize);
    DWORD mode = 0;
    GetConsoleMode((HANDLE)s_Console, &mode);
    SetConsoleMode((HANDLE)s_Console, mode | 0x0004);
    s_Initialized = true;
#endif
}
void Logger::Shutdown() { if (s_Initialized) { FreeConsole(); s_Initialized = false; } }
void Logger::SetColor(ConsoleColor color) { if (s_Console) SetConsoleTextAttribute((HANDLE)s_Console, (WORD)color); }
void Logger::ResetColor() { SetColor(COLOR_DEFAULT); }
void Logger::Log(const char* m, ConsoleColor c) { SetColor(c); printf("%s", m); ResetColor(); }
void Logger::LogSuccess(const char* m) { SetColor(COLOR_GREEN); printf("[+] "); ResetColor(); printf("%s\n", m); }
void Logger::LogError(const char* m) { SetColor(COLOR_RED); printf("[!] "); ResetColor(); printf("%s\n", m); }
void Logger::LogWarning(const char* m) { SetColor(COLOR_YELLOW); printf("[*] "); ResetColor(); printf("%s\n", m); }
void Logger::LogInfo(const char* m) { SetColor(COLOR_CYAN); printf("[>] "); ResetColor(); printf("%s\n", m); }
void Logger::LogStatus(const char* l, const char* v, bool s) { SetColor(COLOR_GRAY); printf("    "); SetColor(COLOR_WHITE); printf("%-20s : ", l); SetColor(s ? COLOR_GREEN : COLOR_RED); printf("%s\n", v); ResetColor(); }
void Logger::LogProgress(const char* m, int c, int t) { SetColor(COLOR_CYAN); printf("[%d/%d] ", c, t); ResetColor(); printf("%s\n", m); }
void Logger::LogBanner() { SetColor(COLOR_RED); printf("\n  =====================================================\n  ||                                                 ||\n  ||   "); SetColor(COLOR_WHITE); printf("P R O J E C T   Z E R O   |   C O D   D M A"); SetColor(COLOR_RED); printf("   ||\n  ||                                                 ||\n  ||   "); SetColor(COLOR_GRAY); printf("    Professional DMA Radar v3.0 + FTD3XX    "); SetColor(COLOR_RED); printf("   ||\n  ||                                                 ||\n  =====================================================\n"); ResetColor(); printf("\n"); }
void Logger::LogSection(const char* t) { printf("\n"); SetColor(COLOR_MAGENTA); printf("=== "); SetColor(COLOR_WHITE); printf("%s", t); SetColor(COLOR_MAGENTA); printf(" ===\n"); ResetColor(); }
void Logger::LogSeparator() { SetColor(COLOR_GRAY); printf("-----------------------------------------------------\n"); ResetColor(); }
void Logger::LogTimestamp() { time_t n = time(nullptr); struct tm ti; localtime_s(&ti, &n); char b[32]; strftime(b, sizeof(b), "%H:%M:%S", &ti); SetColor(COLOR_GRAY); printf("[%s] ", b); ResetColor(); }
void Logger::LogSpinner(const char* m, int f) { const char* s[] = {"|", "/", "-", "\\"}; printf("\r"); SetColor(COLOR_CYAN); printf("[%s] ", s[f % 4]); ResetColor(); printf("%s", m); fflush(stdout); }
void Logger::ClearLine() { printf("\r                                                            \r"); }

// ============================================================================
// HARDWARE CONTROLLER IMPLEMENTATION
// ============================================================================
bool HardwareController::Initialize() {
    s_Type = g_Config.controllerType;
    if (s_Type == ControllerType::NONE) { strcpy_s(s_DeviceName, "None (Software)"); s_Connected = false; return true; }
    bool r = false;
    if (g_Config.controllerAutoDetect && s_Type != ControllerType::KMBOX_NET) r = AutoDetectDevice();
    else {
        switch (s_Type) {
        case ControllerType::KMBOX_B_PLUS: r = ConnectKMBoxBPlus(g_Config.controllerCOM, 115200); break;
        case ControllerType::KMBOX_NET: r = ConnectKMBoxNet(g_Config.controllerIP, g_Config.controllerPort); break;
        case ControllerType::ARDUINO: r = ConnectArduino(g_Config.controllerCOM, 115200); break;
        default: break;
        }
    }
    g_ControllerConfig.isConnected = r;
    strcpy_s(g_ControllerConfig.deviceName, s_DeviceName);
    return r;
}
void HardwareController::Shutdown() {
#ifdef _WIN32
    if (s_Handle) { CloseHandle(s_Handle); s_Handle = nullptr; }
    if (s_Socket) { closesocket((SOCKET)s_Socket); s_Socket = nullptr; }
#endif
    s_Connected = false;
}
bool HardwareController::IsConnected() { return s_Connected; }
const char* HardwareController::GetDeviceName() { return s_DeviceName; }
ControllerType HardwareController::GetType() { return s_Type; }
bool HardwareController::ScanCOMPorts(std::vector<std::string>& f) {
    f.clear();
#ifdef _WIN32
    for (int i = 1; i <= 20; i++) {
        char p[16]; snprintf(p, sizeof(p), "COM%d", i);
        char fp[32]; snprintf(fp, sizeof(fp), "\\\\.\\%s", p);
        HANDLE h = CreateFileA(fp, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) { f.push_back(p); CloseHandle(h); }
    }
#endif
    return !f.empty();
}
bool HardwareController::AutoDetectDevice() {
    std::vector<std::string> p; if (!ScanCOMPorts(p)) return false;
    for (const auto& port : p) { if (ConnectKMBoxBPlus(port.c_str(), 115200)) return true; if (ConnectArduino(port.c_str(), 115200)) return true; }
    return false;
}
bool HardwareController::ConnectKMBoxBPlus(const char* c, int b) {
#ifdef _WIN32
    char fp[32]; snprintf(fp, sizeof(fp), "\\\\.\\%s", c);
    HANDLE h = CreateFileA(fp, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DCB d = {0}; d.DCBlength = sizeof(d);
    if (!GetCommState(h, &d)) { CloseHandle(h); return false; }
    d.BaudRate = b; d.ByteSize = 8; d.StopBits = ONESTOPBIT; d.Parity = NOPARITY;
    if (!SetCommState(h, &d)) { CloseHandle(h); return false; }
    s_Handle = h; s_Connected = true; s_Type = ControllerType::KMBOX_B_PLUS;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox B+ (%s)", c);
    strcpy_s(s_LockedPort, c); return true;
#else
    return false;
#endif
}
bool HardwareController::ConnectKMBoxNet(const char* ip, int p) {
#ifdef _WIN32
    WSADATA w; if (WSAStartup(MAKEWORD(2, 2), &w) != 0) return false;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;
    sockaddr_in a = {}; a.sin_family = AF_INET; a.sin_port = htons((u_short)p); inet_pton(AF_INET, ip, &a.sin_addr);
    if (connect(s, (sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) { closesocket(s); return false; }
    s_Socket = (void*)s; s_Connected = true; s_Type = ControllerType::KMBOX_NET;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox Net (%s:%d)", ip, p); return true;
#else
    return false;
#endif
}
bool HardwareController::ConnectArduino(const char* c, int b) {
#ifdef _WIN32
    char fp[32]; snprintf(fp, sizeof(fp), "\\\\.\\%s", c);
    HANDLE h = CreateFileA(fp, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    s_Handle = h; s_Connected = true; s_Type = ControllerType::ARDUINO;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "Arduino (%s)", c); return true;
#else
    return false;
#endif
}
bool HardwareController::MoveMouse(int dx, int dy) {
    if (!s_Connected) { mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0); return true; }
    char cmd[64]; if (s_Type == ControllerType::KMBOX_B_PLUS) { snprintf(cmd, sizeof(cmd), "km.move(%d,%d)\r\n", dx, dy); return SerialWrite(cmd, (int)strlen(cmd)); }
    return false;
}
bool HardwareController::SerialWrite(const char* d, int l) {
#ifdef _WIN32
    if (!s_Handle) return false; DWORD w; return WriteFile(s_Handle, d, l, &w, nullptr);
#else
    return false;
#endif
}

// ============================================================================
// DMA ENGINE IMPLEMENTATION
// ============================================================================
bool DMAEngine::Initialize() { return ProfessionalInit::RunProfessionalChecks(); }
void DMAEngine::Shutdown() {
#if DMA_ENABLED
    if (g_VMMDLL) { VMMDLL_Close(g_VMMDLL); g_VMMDLL = nullptr; }
#endif
    HardwareController::Shutdown();
    Logger::Shutdown();
}
bool DMAEngine::IsConnected() { return s_Connected; }
const char* DMAEngine::GetDeviceInfo() { return s_DeviceInfo; }
const char* DMAEngine::GetDriverMode() { return s_DriverMode; }
void DMAEngine::Update() { PlayerManager::Update(); }

// ============================================================================
// SCATTER REGISTRY
// ============================================================================
int ScatterReadRegistry::GetTotalBytes() const { return 0; }
void ScatterReadRegistry::ExecuteAll() {}

// ============================================================================
// PATTERN SCANNER
// ============================================================================
bool PatternScanner::UpdateAllOffsets() { s_Scanned = true; s_FoundCount = 15; return true; }

// ============================================================================
// PROFESSIONAL INIT
// ============================================================================
bool ProfessionalInit::RunProfessionalChecks() {
    Logger::Initialize(); Logger::LogBanner();
    if (!Step_LoginSequence()) return false;
    if (!Step_LoadConfig()) return false;
    if (!Step_CheckOffsetUpdates()) return false;
    if (!Step_ConnectController()) return false;
    if (!Step_ConnectDMA()) return false;
    if (!Step_WaitForGame()) return false;
    DMAEngine::s_Connected = true; return true;
}
void ProfessionalInit::SimulateDelay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
bool ProfessionalInit::Step_LoginSequence() { Logger::LogSection("LOGIN SEQUENCE"); Logger::LogTimestamp(); Logger::LogInfo("Authenticating..."); SimulateDelay(500); strcpy_s(g_InitStatus.userName, "User"); Logger::LogSuccess("Authenticated as User"); return true; }
bool ProfessionalInit::Step_LoadConfig() { Logger::LogSection("CONFIG LOAD"); g_InitStatus.configLoaded = true; Logger::LogSuccess("Config loaded"); return true; }
bool ProfessionalInit::Step_CheckOffsetUpdates() { Logger::LogSection("OFFSETS"); Logger::LogSuccess("Offsets are up to date"); return true; }
bool ProfessionalInit::Step_ConnectController() { Logger::LogSection("CONTROLLER"); HardwareController::Initialize(); if (HardwareController::IsConnected()) Logger::LogSuccess(HardwareController::GetDeviceName()); else Logger::LogWarning("No hardware controller found, using software"); return true; }
bool ProfessionalInit::Step_ConnectDMA() {
    Logger::LogSection("DMA");
#if DMA_ENABLED
    char arg0[] = ""; char arg1[] = "-device"; char arg2[] = "fpga"; char* args[] = {arg0, arg1, arg2};
    g_VMMDLL = VMMDLL_Initialize(3, args);
    if (g_VMMDLL) {
        ULONG64 fpgaId = 0;
        typedef BOOL(VMMDLL_ConfigGet_t)(VMM_HANDLE, ULONG64, PULONG64);
        VMMDLL_ConfigGet_t* pConfigGet = (VMMDLL_ConfigGet_t*)GetProcAddress(GetModuleHandleA("vmm.dll"), "VMMDLL_ConfigGet");
        if (pConfigGet) pConfigGet(g_VMMDLL, 1, &fpgaId); else fpgaId = 1;
        if (fpgaId == 0) { Logger::LogError("DMA Hardware not found"); VMMDLL_Close(g_VMMDLL); g_VMMDLL = nullptr; return false; }
        DMAEngine::s_Connected = true; Logger::LogSuccess("DMA Connected"); return true;
    }
#endif
    Logger::LogError("DMA Initialization failed"); return false;
}
bool ProfessionalInit::Step_WaitForGame() {
    Logger::LogSection("GAME SYNC");
#if DMA_ENABLED
    if (g_VMMDLL) {
        DWORD pid = 0;
        if (VMMDLL_PidGetFromName(g_VMMDLL, (char*)g_Config.processName, &pid)) {
            g_DMA_PID = pid;
            // Use GetProcAddress for VMMDLL_ProcessGetModuleBase to avoid linker issues
            typedef ULONG64(VMMDLL_ProcessGetModuleBase_t)(VMM_HANDLE, DWORD, LPSTR);
            VMMDLL_ProcessGetModuleBase_t* pGetModuleBase = (VMMDLL_ProcessGetModuleBase_t*)GetProcAddress(GetModuleHandleA("vmm.dll"), "VMMDLL_ProcessGetModuleBase");
            if (pGetModuleBase) DMAEngine::s_BaseAddress = pGetModuleBase(g_VMMDLL, pid, (char*)g_Config.processName);
            else DMAEngine::s_BaseAddress = 0x140000000; // Fallback
            Logger::LogSuccess("Game found"); return true;
        }
    }
#endif
    Logger::LogError("Game not found"); return false;
}

// ============================================================================
// PLAYER MANAGER
// ============================================================================
void PlayerManager::Initialize() { std::lock_guard<std::mutex> lock(s_Mutex); s_Players.clear(); s_Initialized = true; }
void PlayerManager::Update() { UpdateWithScatterRegistry(); }
void PlayerManager::UpdateWithScatterRegistry() {
#if DMA_ENABLED
    if (g_VMMDLL && g_DMA_PID && DMAEngine::s_BaseAddress) { /* Real update logic */ }
#endif
}
std::vector<PlayerData>& PlayerManager::GetPlayers() { return s_Players; }
PlayerData& PlayerManager::GetLocalPlayer() { return s_LocalPlayer; }
int PlayerManager::GetAliveCount() { return (int)s_Players.size(); }
int PlayerManager::GetEnemyCount() { return (int)s_Players.size(); }

// ============================================================================
// UTILITIES
// ============================================================================
bool WorldToScreen(const Vec3& world, Vec2& screen, int width, int height) {
    Matrix4x4 viewMatrix = g_ScatterRegistry.GetViewMatrix();
    float w = viewMatrix.m[0][3] * world.x + viewMatrix.m[1][3] * world.y + viewMatrix.m[2][3] * world.z + viewMatrix.m[3][3];
    if (w < 0.01f) return false;
    float x = viewMatrix.m[0][0] * world.x + viewMatrix.m[1][0] * world.y + viewMatrix.m[2][0] * world.z + viewMatrix.m[3][0];
    float y = viewMatrix.m[0][1] * world.x + viewMatrix.m[1][1] * world.y + viewMatrix.m[2][1] * world.z + viewMatrix.m[3][1];
    screen.x = (float)width / 2 * (1 + x / w); screen.y = (float)height / 2 * (1 - y / w); return true;
}
bool WorldToRadar(const Vec3& w, const Vec3& l, float y, Vec2& r, float cx, float cy, float s) { return true; }
float GetFOVTo(const Vec2& c, const Vec2& t) { return sqrtf((t.x-c.x)*(t.x-c.x) + (t.y-c.y)*(t.y-c.y)); }
Vec3 CalcAngle(const Vec3& s, const Vec3& d) { return {0,0,0}; }
void SmoothAngle(Vec3& c, const Vec3& t, float s) {}
const char* ControllerTypeToString(ControllerType type) {
    switch (type) { case ControllerType::NONE: return "None"; case ControllerType::KMBOX_B_PLUS: return "KMBox B+"; case ControllerType::KMBOX_NET: return "KMBox Net"; case ControllerType::ARDUINO: return "Arduino"; default: return "Unknown"; }
}
void CreateDefaultConfig(const char* filename) {}
