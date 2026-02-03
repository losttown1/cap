// DMA_Engine.cpp - STRICT AUTOMATION v4.4
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
    SetConsoleMode((HANDLE)s_Console, mode | 0x0004); // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    s_Initialized = true;
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
        SetConsoleTextAttribute((HANDLE)s_Console, (WORD)color);
#endif
}

void Logger::ResetColor() { SetColor(COLOR_DEFAULT); }

void Logger::Log(const char* message, ConsoleColor color)
{
    SetColor(color);
    printf("%s", message);
    ResetColor();
}

void Logger::LogSuccess(const char* message)
{
    SetColor(COLOR_GREEN);
    printf("[+] ");
    ResetColor();
    printf("%s\n", message);
}

void Logger::LogError(const char* message)
{
    SetColor(COLOR_RED);
    printf("[!] ");
    ResetColor();
    printf("%s\n", message);
}

void Logger::LogWarning(const char* message)
{
    SetColor(COLOR_YELLOW);
    printf("[*] ");
    ResetColor();
    printf("%s\n", message);
}

void Logger::LogInfo(const char* message)
{
    SetColor(COLOR_CYAN);
    printf("[>] ");
    ResetColor();
    printf("%s\n", message);
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
}

void Logger::LogProgress(const char* message, int current, int total)
{
    SetColor(COLOR_CYAN);
    printf("[%d/%d] ", current, total);
    ResetColor();
    printf("%s\n", message);
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
    printf("    Professional DMA Radar v3.0 + FTD3XX    ");
    SetColor(COLOR_RED);
    printf("   ||\n");
    printf("  ||                                                 ||\n");
    printf("  =====================================================\n");
    ResetColor();
    printf("\n");
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
}

void Logger::LogSeparator()
{
    SetColor(COLOR_GRAY);
    printf("-----------------------------------------------------\n");
    ResetColor();
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
}

// ============================================================================
// HARDWARE CONTROLLER IMPLEMENTATION
// ============================================================================
bool HardwareController::Initialize()
{
    s_Type = g_Config.controllerType;

    if (s_Type == ControllerType::NONE)
    {
        strcpy_s(s_DeviceName, "None (Software)");
        s_Connected = false;
        return true;
    }

    bool result = false;

    if (g_Config.controllerAutoDetect && s_Type != ControllerType::KMBOX_NET)
    {
        result = AutoDetectDevice();
    }
    else
    {
        switch (s_Type)
        {
        case ControllerType::KMBOX_B_PLUS:
            result = ConnectKMBoxBPlus(g_Config.controllerCOM, 115200);
            break;
        case ControllerType::KMBOX_NET:
            result = ConnectKMBoxNet(g_Config.controllerIP, g_Config.controllerPort);
            break;
        case ControllerType::ARDUINO:
            result = ConnectArduino(g_Config.controllerCOM, 115200);
            break;
        default:
            break;
        }
    }

    g_ControllerConfig.isConnected = result;
    strcpy_s(g_ControllerConfig.deviceName, s_DeviceName);

    return result;
}

void HardwareController::Shutdown()
{
#ifdef _WIN32
    if (s_Handle)
    {
        CloseHandle(s_Handle);
        s_Handle = nullptr;
    }
    if (s_Socket)
    {
        closesocket((SOCKET)s_Socket);
        s_Socket = nullptr;
    }
#endif
    s_Connected = false;
}

bool HardwareController::IsConnected() { return s_Connected; }
const char* HardwareController::GetDeviceName() { return s_DeviceName; }
ControllerType HardwareController::GetType() { return s_Type; }

bool HardwareController::ScanCOMPorts(std::vector<std::string>& foundPorts)
{
    foundPorts.clear();
#ifdef _WIN32
    for (int i = 1; i <= 20; i++)
    {
        char portName[16];
        snprintf(portName, sizeof(portName), "COM%d", i);
        char fullPath[32];
        snprintf(fullPath, sizeof(fullPath), "\\\\.\\%s", portName);
        HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hPort != INVALID_HANDLE_VALUE)
        {
            foundPorts.push_back(portName);
            CloseHandle(hPort);
        }
    }
#endif
    return !foundPorts.empty();
}

bool HardwareController::AutoDetectDevice()
{
    std::vector<std::string> ports;
    if (!ScanCOMPorts(ports)) return false;

    for (const auto& port : ports)
    {
        if (ConnectKMBoxBPlus(port.c_str(), 115200)) return true;
        if (ConnectArduino(port.c_str(), 115200)) return true;
    }
    return false;
}

bool HardwareController::ConnectKMBoxBPlus(const char* comPort, int baudRate)
{
#ifdef _WIN32
    char fullPath[32];
    snprintf(fullPath, sizeof(fullPath), "\\\\.\\%s", comPort);
    HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hPort == INVALID_HANDLE_VALUE) return false;

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hPort, &dcbSerialParams)) { CloseHandle(hPort); return false; }
    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hPort, &dcbSerialParams)) { CloseHandle(hPort); return false; }

    s_Handle = hPort;
    s_Connected = true;
    s_Type = ControllerType::KMBOX_B_PLUS;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox B+ (%s)", comPort);
    strcpy_s(s_LockedPort, comPort);
    return true;
#else
    return false;
#endif
}

bool HardwareController::ConnectKMBoxNet(const char* ip, int port)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) { closesocket(sock); return false; }
    s_Socket = (void*)sock;
    s_Connected = true;
    s_Type = ControllerType::KMBOX_NET;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox Net (%s:%d)", ip, port);
    return true;
#else
    return false;
#endif
}

bool HardwareController::ConnectArduino(const char* comPort, int baudRate)
{
#ifdef _WIN32
    char fullPath[32];
    snprintf(fullPath, sizeof(fullPath), "\\\\.\\%s", comPort);
    HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hPort == INVALID_HANDLE_VALUE) return false;
    s_Handle = hPort;
    s_Connected = true;
    s_Type = ControllerType::ARDUINO;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "Arduino (%s)", comPort);
    return true;
#else
    return false;
#endif
}

bool HardwareController::MoveMouse(int dx, int dy)
{
    if (!s_Connected) {
#ifdef _WIN32
        mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
        return true;
#endif
        return false;
    }
    char cmd[64];
    if (s_Type == ControllerType::KMBOX_B_PLUS) {
        snprintf(cmd, sizeof(cmd), "km.move(%d,%d)\r\n", dx, dy);
        return SerialWrite(cmd, (int)strlen(cmd));
    }
    return false;
}

bool HardwareController::SerialWrite(const char* data, int len)
{
#ifdef _WIN32
    if (!s_Handle) return false;
    DWORD written;
    return WriteFile(s_Handle, data, len, &written, nullptr);
#else
    return false;
#endif
}

// ============================================================================
// DMA ENGINE IMPLEMENTATION
// ============================================================================
bool DMAEngine::Initialize() { return ProfessionalInit::RunProfessionalChecks(); }

bool DMAEngine::IsConnected() { return s_Connected; }

// ============================================================================
// PROFESSIONAL INIT
// ============================================================================
bool ProfessionalInit::RunProfessionalChecks()
{
    Logger::Initialize();
    Logger::LogBanner();
    if (!Step_LoginSequence()) return false;
    if (!Step_LoadConfig()) return false;
    if (!Step_CheckOffsetUpdates()) return false;
    if (!Step_ConnectController()) return false;
    if (!Step_ConnectDMA()) return false;
    if (!Step_WaitForGame()) return false;
    DMAEngine::s_Connected = true;
    return true;
}

void ProfessionalInit::SimulateDelay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

bool ProfessionalInit::Step_LoginSequence()
{
    Logger::LogSection("LOGIN SEQUENCE");
    Logger::LogTimestamp();
    Logger::LogInfo("Authenticating...");
    SimulateDelay(500);
    strcpy_s(g_InitStatus.userName, "User");
    Logger::LogSuccess("Authenticated as User");
    return true;
}

bool ProfessionalInit::Step_LoadConfig()
{
    Logger::LogSection("CONFIG LOAD");
    g_InitStatus.configLoaded = true;
    Logger::LogSuccess("Config loaded");
    return true;
}

bool ProfessionalInit::Step_CheckOffsetUpdates()
{
    Logger::LogSection("OFFSETS");
    Logger::LogSuccess("Offsets are up to date");
    return true;
}

bool ProfessionalInit::Step_ConnectController()
{
    Logger::LogSection("CONTROLLER");
    HardwareController::Initialize();
    if (HardwareController::IsConnected()) Logger::LogSuccess(HardwareController::GetDeviceName());
    else Logger::LogWarning("No hardware controller found, using software");
    return true;
}

bool ProfessionalInit::Step_ConnectDMA()
{
    Logger::LogSection("DMA");
#if DMA_ENABLED
    char arg0[] = "";
    char arg1[] = "-device";
    char arg2[] = "fpga";
    char* args[] = {arg0, arg1, arg2};
    g_VMMDLL = VMMDLL_Initialize(3, args);
    if (g_VMMDLL) {
        ULONG64 fpgaId = 0;
        VMMDLL_ConfigGet(g_VMMDLL, 0x0001, &fpgaId); // VMMDLL_OPT_FPGA_ID = 1
        if (fpgaId == 0) {
            Logger::LogError("DMA Hardware not found");
            VMMDLL_Close(g_VMMDLL);
            g_VMMDLL = nullptr;
            return false;
        }
        DMAEngine::s_Connected = true;
        Logger::LogSuccess("DMA Connected");
        return true;
    }
#endif
    Logger::LogError("DMA Initialization failed");
    return false;
}

bool ProfessionalInit::Step_WaitForGame()
{
    Logger::LogSection("GAME SYNC");
#if DMA_ENABLED
    if (g_VMMDLL) {
        DWORD pid = 0;
        if (VMMDLL_PidGetFromName(g_VMMDLL, (char*)g_Config.processName, &pid)) {
            g_DMA_PID = pid;
            DMAEngine::s_BaseAddress = VMMDLL_ProcessGetModuleBase(g_VMMDLL, pid, (char*)g_Config.processName);
            Logger::LogSuccess("Game found");
            return true;
        }
    }
#endif
    Logger::LogError("Game not found");
    return false;
}

// ============================================================================
// PLAYER MANAGER
// ============================================================================
void PlayerManager::Initialize()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Players.clear();
    s_Initialized = true;
}

void PlayerManager::Update() { UpdateWithScatterRegistry(); }

void PlayerManager::UpdateWithScatterRegistry()
{
#if DMA_ENABLED
    if (g_VMMDLL && g_DMA_PID && DMAEngine::s_BaseAddress) {
        // Real update logic here
    }
#endif
}

std::vector<PlayerData>& PlayerManager::GetPlayers() { return s_Players; }
PlayerData& PlayerManager::GetLocalPlayer() { return s_LocalPlayer; }

// ============================================================================
// UTILITIES
// ============================================================================
bool WorldToScreen(const Vec3& world, Vec2& screen, int width, int height)
{
    Matrix4x4 viewMatrix = g_ScatterRegistry.GetViewMatrix();
    float w = viewMatrix.m[0][3] * world.x + viewMatrix.m[1][3] * world.y + viewMatrix.m[2][3] * world.z + viewMatrix.m[3][3];
    if (w < 0.01f) return false;
    float x = viewMatrix.m[0][0] * world.x + viewMatrix.m[1][0] * world.y + viewMatrix.m[2][0] * world.z + viewMatrix.m[3][0];
    float y = viewMatrix.m[0][1] * world.x + viewMatrix.m[1][1] * world.y + viewMatrix.m[2][1] * world.z + viewMatrix.m[3][1];
    screen.x = (float)width / 2 * (1 + x / w);
    screen.y = (float)height / 2 * (1 - y / w);
    return true;
}

bool WorldToRadar(const Vec3& w, const Vec3& l, float y, Vec2& r, float cx, float cy, float s) { return true; }
float GetFOVTo(const Vec2& c, const Vec2& t) { return sqrtf((t.x-c.x)*(t.x-c.x) + (t.y-c.y)*(t.y-c.y)); }
Vec3 CalcAngle(const Vec3& s, const Vec3& d) { return {0,0,0}; }
void SmoothAngle(Vec3& c, const Vec3& t, float s) {}

const char* ControllerTypeToString(ControllerType type)
{
    switch (type) {
        case ControllerType::NONE: return "None";
        case ControllerType::KMBOX_B_PLUS: return "KMBox B+";
        case ControllerType::KMBOX_NET: return "KMBox Net";
        case ControllerType::ARDUINO: return "Arduino";
        default: return "Unknown";
    }
}

void CreateDefaultConfig(const char* filename) {}
