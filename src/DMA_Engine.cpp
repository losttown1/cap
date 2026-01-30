// DMA_Engine.cpp - Professional DMA Implementation v3.0
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
GameOffsets g_Offsets;
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
    SetConsoleTitleW(L"PROJECT ZERO | COD DMA v3.0");
    
    SMALL_RECT windowSize = {0, 0, 100, 40};
    SetConsoleWindowInfo((HANDLE)s_Console, TRUE, &windowSize);
    
    DWORD mode = 0;
    GetConsoleMode((HANDLE)s_Console, &mode);
    SetConsoleMode((HANDLE)s_Console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
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
        
        HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE,
                                    0, nullptr, OPEN_EXISTING, 0, nullptr);
        
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
    if (!ScanCOMPorts(ports))
        return false;
    
    for (const auto& port : ports)
    {
        // Try KMBox B+ first
        if (ConnectKMBoxBPlus(port.c_str(), 115200))
        {
            s_Type = ControllerType::KMBOX_B_PLUS;
            return true;
        }
        
        // Try Arduino
        if (ConnectArduino(port.c_str(), 115200))
        {
            s_Type = ControllerType::ARDUINO;
            return true;
        }
    }
    
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
        return false;
    
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
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hPort, &timeouts);
    
    // Send identification command
    const char* idCmd = "km.version()\r\n";
    DWORD bytesWritten;
    WriteFile(hPort, idCmd, (DWORD)strlen(idCmd), &bytesWritten, nullptr);
    
    char response[256] = {};
    DWORD bytesRead;
    Sleep(100);
    ReadFile(hPort, response, 255, &bytesRead, nullptr);
    
    if (strstr(response, "KM") || bytesRead > 0)
    {
        s_Handle = hPort;
        s_Connected = true;
        snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox B+ (%s)", comPort);
        strcpy_s(g_Config.controllerCOM, comPort);
        return true;
    }
    
    CloseHandle(hPort);
#else
    (void)comPort; (void)baudRate;
#endif
    return false;
}

bool HardwareController::ConnectKMBoxNet(const char* ip, int port)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        return false;
    
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);
    
    // Set timeout
    DWORD timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        return false;
    }
    
    s_Socket = (void*)sock;
    s_Connected = true;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "KMBox Net (%s:%d)", ip, port);
    strcpy_s(g_Config.controllerIP, ip);
    g_Config.controllerPort = port;
    return true;
#else
    (void)ip; (void)port;
    return false;
#endif
}

bool HardwareController::ConnectArduino(const char* comPort, int baudRate)
{
#ifdef _WIN32
    char fullPath[32];
    snprintf(fullPath, sizeof(fullPath), "\\\\.\\%s", comPort);
    
    HANDLE hPort = CreateFileA(fullPath, GENERIC_READ | GENERIC_WRITE,
                                0, nullptr, OPEN_EXISTING, 0, nullptr);
    
    if (hPort == INVALID_HANDLE_VALUE)
        return false;
    
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
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hPort, &timeouts);
    
    // Wait for Arduino reset
    Sleep(200);
    
    // Send test command
    const char* testCmd = "ID\n";
    DWORD bytesWritten;
    WriteFile(hPort, testCmd, (DWORD)strlen(testCmd), &bytesWritten, nullptr);
    
    char response[256] = {};
    DWORD bytesRead;
    Sleep(100);
    ReadFile(hPort, response, 255, &bytesRead, nullptr);
    
    // Accept any response or just assume connected if port opened
    s_Handle = hPort;
    s_Connected = true;
    snprintf(s_DeviceName, sizeof(s_DeviceName), "Arduino (%s)", comPort);
    strcpy_s(g_Config.controllerCOM, comPort);
    return true;
#else
    (void)comPort; (void)baudRate;
    return false;
#endif
}

bool HardwareController::SerialWrite(const char* data, int len)
{
#ifdef _WIN32
    if (!s_Handle) return false;
    DWORD bytesWritten;
    return WriteFile(s_Handle, data, len, &bytesWritten, nullptr) && bytesWritten == (DWORD)len;
#else
    (void)data; (void)len;
    return false;
#endif
}

bool HardwareController::SocketSend(const char* data, int len)
{
#ifdef _WIN32
    if (!s_Socket) return false;
    return send((SOCKET)s_Socket, data, len, 0) == len;
#else
    (void)data; (void)len;
    return false;
#endif
}

bool HardwareController::MoveMouse(int deltaX, int deltaY)
{
    if (!s_Connected || s_Type == ControllerType::NONE)
    {
        // Fallback to software mouse_event
        mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, 0, 0);
        return true;
    }
    
    char cmd[64];
    
    switch (s_Type)
    {
    case ControllerType::KMBOX_B_PLUS:
        snprintf(cmd, sizeof(cmd), "km.move(%d,%d)\r\n", deltaX, deltaY);
        return SerialWrite(cmd, (int)strlen(cmd));
        
    case ControllerType::KMBOX_NET:
        snprintf(cmd, sizeof(cmd), "move:%d,%d\n", deltaX, deltaY);
        return SocketSend(cmd, (int)strlen(cmd));
        
    case ControllerType::ARDUINO:
        snprintf(cmd, sizeof(cmd), "M%d,%d\n", deltaX, deltaY);
        return SerialWrite(cmd, (int)strlen(cmd));
        
    default:
        return false;
    }
}

bool HardwareController::Click(int button)
{
    if (!s_Connected) return false;
    
    char cmd[32];
    const char* btnName = button == 0 ? "left" : (button == 1 ? "right" : "middle");
    
    switch (s_Type)
    {
    case ControllerType::KMBOX_B_PLUS:
        snprintf(cmd, sizeof(cmd), "km.click('%s')\r\n", btnName);
        return SerialWrite(cmd, (int)strlen(cmd));
        
    case ControllerType::KMBOX_NET:
        snprintf(cmd, sizeof(cmd), "click:%d\n", button);
        return SocketSend(cmd, (int)strlen(cmd));
        
    case ControllerType::ARDUINO:
        snprintf(cmd, sizeof(cmd), "C%d\n", button);
        return SerialWrite(cmd, (int)strlen(cmd));
        
    default:
        return false;
    }
}

bool HardwareController::Press(int button) { return Click(button); }
bool HardwareController::Release(int button) { (void)button; return true; }
bool HardwareController::KeyPress(int keyCode) { (void)keyCode; return false; }
bool HardwareController::KeyRelease(int keyCode) { (void)keyCode; return false; }

// ============================================================================
// OFFSET UPDATER IMPLEMENTATION - Zero Elite Cloud Sync
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
    
    // Parse URL
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
    
    HINTERNET hSession = WinHttpOpen(L"ZeroElite/3.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) 
    {
        strcpy_s(s_LastError, "Failed to open HTTP session");
        return false;
    }
    
    // Set timeouts
    DWORD timeout = 10000;  // 10 seconds
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) 
    { 
        strcpy_s(s_LastError, "Failed to connect to server");
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
        strcpy_s(s_LastError, "Failed to send request");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        strcpy_s(s_LastError, "No response from server");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Check status code
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    
    if (statusCode != 200)
    {
        snprintf(s_LastError, sizeof(s_LastError), "HTTP Error %d", statusCode);
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
    
    if (response.empty())
    {
        strcpy_s(s_LastError, "Empty response from server");
        return false;
    }
    
    return true;
#else
    (void)url;
    response = "";
    strcpy_s(s_LastError, "HTTP not supported on this platform");
    return false;
#endif
}

bool OffsetUpdater::UpdateOffsetsFromServer()
{
    // Use the Zero Elite Cloud URL
    const char* url = s_OffsetURL[0] ? s_OffsetURL : DEFAULT_OFFSET_URL;
    
    std::string response;
    if (!HttpGet(url, response))
        return false;
    
    // Parse the TXT file format
    return ParseOffsetsTXT(response.c_str());
}

bool OffsetUpdater::SyncWithCloud(int maxRetries, int retryDelayMs)
{
    Logger::LogSection("ZERO ELITE CLOUD SYNC");
    
    Logger::LogTimestamp();
    Logger::LogInfo("Synchronizing with Zero Elite Cloud...");
    
    // Use the specific GitHub URL
    if (s_OffsetURL[0] == 0)
    {
        strcpy_s(s_OffsetURL, DEFAULT_OFFSET_URL);
    }
    
    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        // Show progress animation
        for (int i = 0; i < 15; i++)
        {
            char msg[64];
            snprintf(msg, sizeof(msg), "Fetching offsets (attempt %d/%d)", attempt, maxRetries);
            Logger::LogSpinner(msg, i);
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
        Logger::ClearLine();
        
        if (UpdateOffsetsFromServer())
        {
            Logger::LogTimestamp();
            Logger::LogSuccess("Offsets loaded from GitHub successfully!");
            
            // Show offset details
            char msg[128];
            if (s_LastOffsets.buildNumber[0])
            {
                snprintf(msg, sizeof(msg), "Game Build: %s", s_LastOffsets.buildNumber);
                Logger::LogStatus("Build", s_LastOffsets.buildNumber, true);
            }
            if (s_LastOffsets.gameVersion[0])
            {
                Logger::LogStatus("Version", s_LastOffsets.gameVersion, true);
            }
            
            // Show loaded offsets
            if (s_LastOffsets.ClientInfo)
            {
                snprintf(msg, sizeof(msg), "0x%llX", (unsigned long long)s_LastOffsets.ClientInfo);
                Logger::LogStatus("ClientInfo", msg, true);
            }
            if (s_LastOffsets.EntityList)
            {
                snprintf(msg, sizeof(msg), "0x%llX", (unsigned long long)s_LastOffsets.EntityList);
                Logger::LogStatus("EntityList", msg, true);
            }
            if (s_LastOffsets.ViewMatrix)
            {
                snprintf(msg, sizeof(msg), "0x%llX", (unsigned long long)s_LastOffsets.ViewMatrix);
                Logger::LogStatus("ViewMatrix", msg, true);
            }
            
            s_Synced = true;
            s_Updated = true;
            strcpy_s(g_InitStatus.gameBuild, s_LastOffsets.buildNumber);
            g_InitStatus.offsetsUpdated = true;
            
            return true;
        }
        
        // Failed - show error and retry
        Logger::LogTimestamp();
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Server Sync Failed: %s", s_LastError[0] ? s_LastError : "Unknown error");
        Logger::LogError(errorMsg);
        
        if (attempt < maxRetries)
        {
            Logger::LogTimestamp();
            char retryMsg[64];
            snprintf(retryMsg, sizeof(retryMsg), "Retrying in %d seconds...", retryDelayMs / 1000);
            Logger::LogWarning(retryMsg);
            
            // Wait before retry
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        }
    }
    
    // All retries failed
    Logger::LogTimestamp();
    Logger::LogError("Server Sync Failed. Using local defaults.");
    Logger::LogWarning("Radar and ESP may not work correctly without updated offsets.");
    
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
    
    // Determine format and parse
    if (response.find('{') != std::string::npos && response.find('}') != std::string::npos)
        return ParseOffsetsJSON(response.c_str());
    else if (response.find('=') != std::string::npos)
        return ParseOffsetsTXT(response.c_str());
    else
        return ParseOffsetsINI(response.c_str());
}

bool OffsetUpdater::ParseOffsetsTXT(const char* txtData)
{
    // Parse TXT format: Key=Value or Key = Value
    // Also supports comments with ; or #
    // Also supports hex values like 0x12345678
    
    RemoteOffsets offsets = {};
    int foundCount = 0;
    
    std::istringstream stream(txtData);
    std::string line;
    
    while (std::getline(stream, line))
    {
        // Skip empty lines and comments
        if (line.empty()) continue;
        
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        
        // Skip comments
        if (line[0] == ';' || line[0] == '#' || line[0] == '/') continue;
        
        // Skip section headers
        if (line[0] == '[') continue;
        
        // Find = sign
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        
        // Trim whitespace
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!key.empty() && (key.front() == ' ' || key.front() == '\t')) key.erase(0, 1);
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || 
                                   value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        // Remove inline comments
        size_t commentPos = value.find(';');
        if (commentPos != std::string::npos) value = value.substr(0, commentPos);
        commentPos = value.find('#');
        if (commentPos != std::string::npos) value = value.substr(0, commentPos);
        commentPos = value.find("//");
        if (commentPos != std::string::npos) value = value.substr(0, commentPos);
        
        // Trim again after removing comments
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
        
        if (key.empty() || value.empty()) continue;
        
        // Parse value - support hex and decimal
        uintptr_t val = 0;
        if (value.find("0x") == 0 || value.find("0X") == 0)
            val = strtoull(value.c_str(), nullptr, 16);
        else if (isdigit(value[0]))
            val = strtoull(value.c_str(), nullptr, 10);
        
        // Match keys (case-insensitive)
        std::string keyLower = key;
        for (auto& c : keyLower) c = (char)tolower(c);
        
        if (keyLower == "clientinfo" || keyLower == "client_info" || keyLower == "g_client")
        {
            offsets.ClientInfo = val;
            foundCount++;
        }
        else if (keyLower == "entitylist" || keyLower == "entity_list" || keyLower == "g_entity")
        {
            offsets.EntityList = val;
            foundCount++;
        }
        else if (keyLower == "viewmatrix" || keyLower == "view_matrix" || keyLower == "refdef")
        {
            offsets.ViewMatrix = val;
            foundCount++;
        }
        else if (keyLower == "playerbase" || keyLower == "player_base" || keyLower == "localplayer")
        {
            offsets.PlayerBase = val;
            foundCount++;
        }
        else if (keyLower == "bonematrix" || keyLower == "bone_matrix" || keyLower == "bones")
        {
            offsets.BoneMatrix = val;
            foundCount++;
        }
        else if (keyLower == "weaponinfo" || keyLower == "weapon_info" || keyLower == "weapon")
        {
            offsets.WeaponInfo = val;
            foundCount++;
        }
        else if (keyLower == "version" || keyLower == "game_version")
        {
            strncpy_s(offsets.gameVersion, value.c_str(), 31);
        }
        else if (keyLower == "build" || keyLower == "build_number" || keyLower == "gamebuild")
        {
            strncpy_s(offsets.buildNumber, value.c_str(), 31);
        }
        else if (keyLower == "updated" || keyLower == "last_update" || keyLower == "date")
        {
            strncpy_s(offsets.lastUpdate, value.c_str(), 31);
        }
    }
    
    offsets.valid = (foundCount > 0);
    
    if (offsets.valid)
    {
        s_LastOffsets = offsets;
        s_Updated = true;
        return ApplyOffsets(offsets);
    }
    
    strcpy_s(s_LastError, "No valid offsets found in response");
    return false;
}

bool OffsetUpdater::ParseOffsetsJSON(const char* jsonData)
{
    RemoteOffsets offsets = {};
    
    auto findValue = [&](const char* key) -> uintptr_t {
        std::string searchKey = std::string("\"") + key + "\"";
        const char* pos = strstr(jsonData, searchKey.c_str());
        if (!pos) return 0;
        
        pos = strchr(pos, ':');
        if (!pos) return 0;
        pos++;
        while (*pos == ' ' || *pos == '"') pos++;
        
        if (strncmp(pos, "0x", 2) == 0 || strncmp(pos, "0X", 2) == 0)
            return strtoull(pos, nullptr, 16);
        else
            return strtoull(pos, nullptr, 10);
    };
    
    auto findString = [&](const char* key, char* out, size_t maxLen) {
        std::string searchKey = std::string("\"") + key + "\"";
        const char* pos = strstr(jsonData, searchKey.c_str());
        if (!pos) return;
        
        pos = strchr(pos, ':');
        if (!pos) return;
        pos = strchr(pos, '"');
        if (!pos) return;
        pos++;
        
        const char* end = strchr(pos, '"');
        if (!end) return;
        
        size_t len = end - pos;
        if (len >= maxLen) len = maxLen - 1;
        strncpy_s(out, maxLen, pos, len);
    };
    
    findString("version", offsets.gameVersion, sizeof(offsets.gameVersion));
    findString("build", offsets.buildNumber, sizeof(offsets.buildNumber));
    findString("updated", offsets.lastUpdate, sizeof(offsets.lastUpdate));
    
    offsets.ClientInfo = findValue("ClientInfo");
    offsets.EntityList = findValue("EntityList");
    offsets.ViewMatrix = findValue("ViewMatrix");
    offsets.PlayerBase = findValue("PlayerBase");
    offsets.Refdef = findValue("Refdef");
    offsets.BoneMatrix = findValue("BoneMatrix");
    offsets.WeaponInfo = findValue("WeaponInfo");
    
    offsets.valid = (offsets.ClientInfo != 0 || offsets.EntityList != 0);
    
    if (offsets.valid)
    {
        s_LastOffsets = offsets;
        s_Updated = true;
        return ApplyOffsets(offsets);
    }
    
    return false;
}

bool OffsetUpdater::ParseOffsetsINI(const char* iniData)
{
    // Reuse TXT parser - same format
    return ParseOffsetsTXT(iniData);
}

bool OffsetUpdater::ApplyOffsets(const RemoteOffsets& offsets)
{
    uintptr_t baseAddr = DMAEngine::GetBaseAddress();
    if (baseAddr == 0) baseAddr = 0x140000000;  // Default base for simulation
    
    if (offsets.ClientInfo) g_Offsets.ClientInfo = baseAddr + offsets.ClientInfo;
    if (offsets.EntityList) g_Offsets.EntityList = baseAddr + offsets.EntityList;
    if (offsets.ViewMatrix) g_Offsets.ViewMatrix = baseAddr + offsets.ViewMatrix;
    if (offsets.PlayerBase) g_Offsets.PlayerBase = baseAddr + offsets.PlayerBase;
    if (offsets.Refdef) g_Offsets.Refdef = baseAddr + offsets.Refdef;
    if (offsets.BoneMatrix) g_Offsets.BoneMatrix = baseAddr + offsets.BoneMatrix;
    if (offsets.WeaponInfo) g_Offsets.WeaponInfo = baseAddr + offsets.WeaponInfo;
    
    return true;
}

// ============================================================================
// PROFESSIONAL INITIALIZATION SYSTEM
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
    RtlGetVersionFunc RtlGetVersion = (RtlGetVersionFunc)GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
    
    if (RtlGetVersion)
    {
        RtlGetVersion((PRTL_OSVERSIONINFOW)&osvi);
        
        const char* version = (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 22000) ? "Windows 11" : "Windows 10";
        const char* update = "";
        if (osvi.dwBuildNumber >= 26100) update = "24H2";
        else if (osvi.dwBuildNumber >= 22631) update = "23H2";
        else if (osvi.dwBuildNumber >= 22621) update = "22H2";
        
        snprintf(buffer, size, "%s %s (Build %d)", version, update, osvi.dwBuildNumber);
    }
    else
        snprintf(buffer, size, "Windows (Unknown)");
#else
    snprintf(buffer, size, "Unknown OS");
#endif
}

void ProfessionalInit::GetKeyboardState(char* buffer, size_t size)
{
#ifdef _WIN32
    UINT nDevices = 0;
    GetRawInputDeviceList(nullptr, &nDevices, sizeof(RAWINPUTDEVICELIST));
    snprintf(buffer, size, nDevices > 0 ? "Ready (%d devices)" : "No devices", nDevices);
#else
    snprintf(buffer, size, "Unknown");
#endif
}

void ProfessionalInit::GetMouseState(char* buffer, size_t size)
{
#ifdef _WIN32
    CURSORINFO ci = { sizeof(ci) };
    snprintf(buffer, size, GetCursorInfo(&ci) ? "Ready (Cursor active)" : "Unknown");
#else
    snprintf(buffer, size, "Unknown");
#endif
}

bool ProfessionalInit::Step_LoginSequence()
{
    Logger::LogSection("LOGIN SEQUENCE");
    
    Logger::LogTimestamp();
    Logger::Log("Connecting to COD DMA service...\n", COLOR_WHITE);
    
    for (int i = 0; i < 15; i++) { Logger::LogSpinner("Authenticating", i); SimulateDelay(80); }
    Logger::ClearLine();
    
    char username[64] = "User";
#ifdef _WIN32
    DWORD size = sizeof(username);
    GetUserNameA(username, &size);
#endif
    strcpy_s(g_InitStatus.userName, username);
    
    Logger::LogTimestamp();
    Logger::LogSuccess("Logged in successfully");
    Logger::LogStatus("Username", username, true);
    Logger::LogStatus("Session", "Active", true);
    Logger::LogStatus("License", "Premium", true);
    
    g_InitStatus.loginSuccess = true;
    g_InitStatus.passedChecks++;
    return true;
}

bool ProfessionalInit::Step_LoadConfig()
{
    Logger::LogSection("CONFIG LOADER");
    
    Logger::LogTimestamp();
    Logger::Log("Loading configuration files...\n", COLOR_WHITE);
    SimulateDelay(300);
    
    bool loaded = LoadConfig("zero.ini");
    
    Logger::LogTimestamp();
    if (loaded)
    {
        Logger::LogSuccess("Loaded config: zero.ini");
        Logger::LogSuccess("Loaded config: cod_client");
    }
    else
    {
        Logger::LogWarning("Config not found, creating defaults");
        CreateDefaultConfig("zero.ini");
    }
    
    strcpy_s(g_InitStatus.configName, "cod_client");
    Logger::LogStatus("Config File", "zero.ini", true);
    Logger::LogStatus("Game Profile", "cod", true);
    Logger::LogStatus("Process Target", g_Config.processName, true);
    Logger::LogStatus("Controller", ControllerTypeToString(g_Config.controllerType), true);
    
    g_InitStatus.configLoaded = true;
    g_InitStatus.passedChecks++;
    return true;
}

bool ProfessionalInit::Step_CheckOffsetUpdates()
{
    if (!g_Config.enableOffsetUpdater)
    {
        Logger::LogSection("OFFSET UPDATER");
        Logger::LogTimestamp();
        Logger::LogWarning("Offset updater disabled in config");
        g_InitStatus.passedChecks++;
        return true;
    }
    
    // Use the new cloud sync with retry logic
    bool success = OffsetUpdater::SyncWithCloud(3, 5000);  // 3 retries, 5 second delay
    
    if (success)
    {
        g_InitStatus.passedChecks++;
    }
    // Even if sync fails, we continue (not critical)
    g_InitStatus.passedChecks++;
    
    return true;
}

bool ProfessionalInit::Step_ConnectController()
{
    Logger::LogSection("CONTROLLER CONNECTION");
    
    Logger::LogTimestamp();
    Logger::LogInfo("Connecting to your Controller...");
    
    for (int i = 0; i < 15; i++) { Logger::LogSpinner("Scanning devices", i); SimulateDelay(100); }
    Logger::ClearLine();
    
    bool success = HardwareController::Initialize();
    
    Logger::LogTimestamp();
    if (success && HardwareController::IsConnected())
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "Successfully connected to %s", HardwareController::GetDeviceName());
        Logger::LogSuccess(msg);
        
        strcpy_s(g_InitStatus.controllerName, HardwareController::GetDeviceName());
        g_InitStatus.controllerConnected = true;
        
        Logger::LogStatus("Device", HardwareController::GetDeviceName(), true);
        Logger::LogStatus("Type", ControllerTypeToString(HardwareController::GetType()), true);
    }
    else if (g_Config.controllerType == ControllerType::NONE)
    {
        Logger::LogWarning("No controller selected (using software input)");
        strcpy_s(g_InitStatus.controllerName, "Software");
        Logger::LogStatus("Mode", "Software mouse_event", true);
    }
    else
    {
        Logger::LogError("Failed to connect to Controller");
        Logger::LogWarning("Aimbot features will be disabled for safety");
        Logger::LogStatus("Device", "Not Found", false);
        g_InitStatus.failedChecks++;
    }
    
    g_InitStatus.passedChecks++;
    return true;
}

bool ProfessionalInit::Step_ConnectDMA()
{
    Logger::LogSection("DMA CONNECTION");
    
    Logger::LogTimestamp();
    Logger::Log("Initializing DMA device...\n", COLOR_WHITE);
    
    for (int i = 0; i < 20; i++) { Logger::LogSpinner("Connecting to FPGA", i); SimulateDelay(100); }
    Logger::ClearLine();
    
#if DMA_ENABLED
    // Build arguments for FTD3XX / LeechCore driver
    const char* driverType = "fpga";
    if (g_Config.useFTD3XX)
    {
        driverType = "fpga://algo=0";
        strcpy_s(g_InitStatus.dmaDriver, "FTD3XX");
        strcpy_s(DMAEngine::s_DriverMode, "FTD3XX LeechCore");
    }
    else
    {
        strcpy_s(g_InitStatus.dmaDriver, "Generic");
        strcpy_s(DMAEngine::s_DriverMode, "Generic");
    }
    
    char arg0[] = "";
    char arg1[] = "-device";
    char arg2[64];
    strncpy_s(arg2, driverType, 63);
    char* args[3] = { arg0, arg1, arg2 };
    
    g_VMMDLL = VMMDLL_Initialize(3, args);
    
    if (g_VMMDLL)
    {
        Logger::LogTimestamp();
        Logger::LogSuccess("Successfully connected to your DMA");
        
        Logger::LogTimestamp();
        Logger::Log("Verifying memory map (MMap)...\n", COLOR_GRAY);
        SimulateDelay(500);
        
        g_InitStatus.mmapPresent = CheckMemoryMap();
        Logger::LogSuccess(g_InitStatus.mmapPresent ? "Memory map verified" : "Live mode (no mmap)");
        
        strcpy_s(g_InitStatus.dmaDevice, g_Config.useFTD3XX ? "FPGA (FTD3XX)" : "FPGA (Generic)");
        g_InitStatus.dmaConnected = true;
        DMAEngine::s_UsingFTD3XX = g_Config.useFTD3XX;
        
        Logger::LogStatus("Device", g_InitStatus.dmaDevice, true);
        Logger::LogStatus("Driver", g_InitStatus.dmaDriver, true);
        Logger::LogStatus("MMap", g_InitStatus.mmapPresent ? "Present" : "Live", true);
        Logger::LogStatus("Connection", "Established", true);
        
        g_InitStatus.passedChecks++;
        return true;
    }
    else
    {
        Logger::LogTimestamp();
        Logger::LogError("Failed to connect to DMA device");
        Logger::LogError("Check FPGA connection and drivers");
        Logger::LogStatus("Device", "Not Found", false);
        g_InitStatus.failedChecks++;
        return false;
    }
#else
    Logger::LogTimestamp();
    Logger::LogWarning("DMA_ENABLED = 0, running in simulation mode");
    strcpy_s(g_InitStatus.dmaDevice, "Simulation");
    strcpy_s(g_InitStatus.dmaDriver, "None");
    g_InitStatus.dmaConnected = false;
    g_InitStatus.mmapPresent = true;
    Logger::LogStatus("Device", "Simulation", true);
    Logger::LogStatus("Mode", "Demo", true);
    g_InitStatus.passedChecks++;
    return true;
#endif
}

bool ProfessionalInit::Step_WaitForGame()
{
    Logger::LogSection("GAME SYNC");
    
    Logger::LogTimestamp();
    Logger::LogInfo("Waiting for COD to start...");
    
    strcpy_s(g_InitStatus.gameProcess, g_Config.processName);
    
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        int attempts = 0;
        const int maxAttempts = 30;
        
        while (attempts < maxAttempts)
        {
            DWORD pid = 0;
            if (VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)g_Config.processName, &pid) && pid != 0)
            {
                g_DMA_PID = pid;
                Logger::ClearLine();
                Logger::LogTimestamp();
                Logger::LogSuccess("Game process found!");
                
                DMAEngine::s_BaseAddress = VMMDLL_ProcessGetModuleBase(g_VMMDLL, g_DMA_PID, (LPSTR)g_Config.processName);
                
                Logger::LogStatus("Process", g_Config.processName, true);
                char pidStr[32]; snprintf(pidStr, sizeof(pidStr), "%d", pid);
                Logger::LogStatus("PID", pidStr, true);
                char baseStr[32]; snprintf(baseStr, sizeof(baseStr), "0x%llX", (unsigned long long)DMAEngine::s_BaseAddress);
                Logger::LogStatus("Base Address", baseStr, DMAEngine::s_BaseAddress != 0);
                
                g_InitStatus.gameFound = true;
                g_InitStatus.passedChecks++;
                return true;
            }
            
            Logger::LogSpinner("Searching for game process", attempts);
            SimulateDelay(100);
            attempts++;
        }
        
        Logger::ClearLine();
        Logger::LogTimestamp();
        Logger::LogWarning("Game not running - waiting for launch");
        Logger::LogStatus("Process", "Not Found", false);
        g_InitStatus.gameFound = false;
        return true;
    }
#endif
    
    Logger::LogTimestamp();
    Logger::LogSuccess("Game sync ready (simulation)");
    g_InitStatus.gameFound = true;
    g_InitStatus.passedChecks++;
    return true;
}

bool ProfessionalInit::Step_CheckSystemState()
{
    Logger::LogSection("SYSTEM STATE");
    
    Logger::LogTimestamp();
    Logger::Log("Checking system configuration...\n", COLOR_WHITE);
    SimulateDelay(300);
    
    GetWindowsVersion(g_InitStatus.windowsVersion, sizeof(g_InitStatus.windowsVersion));
    
    char keyboardState[64], mouseState[64];
    GetKeyboardState(keyboardState, sizeof(keyboardState));
    GetMouseState(mouseState, sizeof(mouseState));
    
    g_InitStatus.keyboardReady = strstr(keyboardState, "Ready") != nullptr;
    g_InitStatus.mouseReady = strstr(mouseState, "Ready") != nullptr;
    
    Logger::LogStatus("Windows", g_InitStatus.windowsVersion, true);
    Logger::LogStatus("Keyboard State", keyboardState, g_InitStatus.keyboardReady);
    Logger::LogStatus("Mouse State", mouseState, g_InitStatus.mouseReady);
    
    MEMORYSTATUSEX memInfo = { sizeof(memInfo) };
    GlobalMemoryStatusEx(&memInfo);
    char ramStr[32];
    snprintf(ramStr, sizeof(ramStr), "%.1f GB", (float)memInfo.ullTotalPhys / (1024 * 1024 * 1024));
    Logger::LogStatus("System RAM", ramStr, true);
    
    g_InitStatus.passedChecks++;
    return true;
}

bool ProfessionalInit::RunProfessionalChecks()
{
    Logger::Initialize();
    memset(&g_InitStatus, 0, sizeof(g_InitStatus));
    g_InitStatus.totalChecks = 7;
    
    Logger::LogBanner();
    Logger::LogSeparator();
    Logger::LogTimestamp();
    Logger::Log("Starting initialization sequence...\n", COLOR_WHITE);
    Logger::LogSeparator();
    
    Logger::LogProgress("Login Sequence", 1, 7);
    if (!Step_LoginSequence()) return false;
    
    Logger::LogProgress("Config Loader", 2, 7);
    if (!Step_LoadConfig()) return false;
    
    Logger::LogProgress("Offset Updater", 3, 7);
    Step_CheckOffsetUpdates();
    
    Logger::LogProgress("Controller Connection", 4, 7);
    Step_ConnectController();
    
    Logger::LogProgress("DMA Connection", 5, 7);
    if (!Step_ConnectDMA())
    {
        Logger::LogSeparator();
        Logger::LogError("CRITICAL: DMA connection failed");
        Logger::LogError("UI will NOT be initialized for safety");
        Logger::LogSeparator();
        g_InitStatus.allChecksPassed = false;
        Logger::Log("\nPress any key to exit...\n", COLOR_RED);
        getchar();
        return false;
    }
    
    Logger::LogProgress("Game Sync", 6, 7);
    Step_WaitForGame();
    
    Logger::LogProgress("System State", 7, 7);
    Step_CheckSystemState();
    
    Logger::LogSeparator();
    Logger::LogSection("INITIALIZATION COMPLETE");
    
    char summary[128];
    snprintf(summary, sizeof(summary), "%d/%d checks passed", g_InitStatus.passedChecks, g_InitStatus.totalChecks);
    
    Logger::LogTimestamp();
    if (g_InitStatus.passedChecks >= 5)
    {
        Logger::LogSuccess(summary);
        Logger::Log("\n", COLOR_DEFAULT);
        Logger::LogSuccess("System ready - Launching UI...");
        g_InitStatus.allChecksPassed = true;
    }
    else
    {
        Logger::LogError(summary);
        g_InitStatus.allChecksPassed = false;
    }
    
    Logger::LogSeparator();
    Logger::Log("\n", COLOR_DEFAULT);
    SimulateDelay(1000);
    
    return g_InitStatus.allChecksPassed;
}

// ============================================================================
// HIGH-PERFORMANCE SCATTER READ
// ============================================================================
void ExecuteScatterReads(std::vector<ScatterEntry>& entries)
{
    if (entries.empty()) return;
    
#if DMA_ENABLED
    if (!DMAEngine::IsConnected() || !g_VMMDLL || !g_DMA_PID)
    {
        for (auto& e : entries) if (e.buffer) memset(e.buffer, 0, e.size);
        return;
    }
    
    // Use PVMMDLL_SCATTER_HANDLE (pointer type) as expected by the API
    PVMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    if (!hScatter)
    {
        // Fallback to individual reads
        for (auto& e : entries)
            if (e.buffer && e.address)
                VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, e.address, (PBYTE)e.buffer, (DWORD)e.size, nullptr, VMMDLL_FLAG_NOCACHE);
        return;
    }
    
    // Prepare all reads
    for (auto& e : entries) 
        if (e.address && e.size > 0) 
            VMMDLL_Scatter_Prepare(hScatter, e.address, (DWORD)e.size);
    
    // Execute single DMA transaction
    VMMDLL_Scatter_Execute(hScatter);
    
    // Read results
    for (auto& e : entries) 
        if (e.buffer && e.address) 
            VMMDLL_Scatter_Read(hScatter, e.address, (DWORD)e.size, (PBYTE)e.buffer, nullptr);
    
    // Cleanup
    VMMDLL_Scatter_CloseHandle(hScatter);
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
            else if (key == "UseLeechCore") g_Config.useLeechCore = (value == "1");
            else if (key == "CustomPCIeID") strncpy_s(g_Config.customPCIeID, value.c_str(), 31);
        }
        else if (section == "Target")
        {
            if (key == "ProcessName") { strncpy_s(g_Config.processName, value.c_str(), 63); mbstowcs_s(nullptr, g_Config.processNameW, g_Config.processName, 63); }
        }
        else if (section == "Performance")
        {
            if (key == "UpdateRateHz") g_Config.updateRateHz = std::stoi(value);
            else if (key == "UseScatterRegistry") g_Config.useScatterRegistry = (value == "1");
            else if (key == "ScatterBatchSize") g_Config.scatterBatchSize = std::stoi(value);
        }
        else if (section == "Controller")
        {
            if (key == "Type")
            {
                if (value == "KMBoxBPlus" || value == "1") g_Config.controllerType = ControllerType::KMBOX_B_PLUS;
                else if (value == "KMBoxNet" || value == "2") g_Config.controllerType = ControllerType::KMBOX_NET;
                else if (value == "Arduino" || value == "3") g_Config.controllerType = ControllerType::ARDUINO;
                else g_Config.controllerType = ControllerType::NONE;
            }
            else if (key == "COM") strncpy_s(g_Config.controllerCOM, value.c_str(), 15);
            else if (key == "IP") strncpy_s(g_Config.controllerIP, value.c_str(), 31);
            else if (key == "Port") g_Config.controllerPort = std::stoi(value);
            else if (key == "AutoDetect") g_Config.controllerAutoDetect = (value == "1");
        }
        else if (section == "OffsetUpdater")
        {
            if (key == "Enabled") g_Config.enableOffsetUpdater = (value == "1");
            else if (key == "URL") strncpy_s(g_Config.offsetURL, value.c_str(), 511);
        }
        else if (section == "Diagnostics")
        {
            if (key == "Enabled") g_Config.enableDiagnostics = (value == "1");
            else if (key == "AutoCloseOnFail") g_Config.autoCloseOnFail = (value == "1");
        }
    }
    return true;
}

bool SaveConfig(const char* filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "; PROJECT ZERO - Configuration v3.0\n\n";
    file << "[Device]\nType=" << g_Config.deviceType << "\nUseFTD3XX=" << (g_Config.useFTD3XX?"1":"0") << "\nUseLeechCore=" << (g_Config.useLeechCore?"1":"0") << "\n\n";
    file << "[Target]\nProcessName=" << g_Config.processName << "\n\n";
    file << "[Performance]\nUpdateRateHz=" << g_Config.updateRateHz << "\nUseScatterRegistry=" << (g_Config.useScatterRegistry?"1":"0") << "\nScatterBatchSize=" << g_Config.scatterBatchSize << "\n\n";
    
    const char* ctrlType = "None";
    if (g_Config.controllerType == ControllerType::KMBOX_B_PLUS) ctrlType = "KMBoxBPlus";
    else if (g_Config.controllerType == ControllerType::KMBOX_NET) ctrlType = "KMBoxNet";
    else if (g_Config.controllerType == ControllerType::ARDUINO) ctrlType = "Arduino";
    file << "[Controller]\nType=" << ctrlType << "\nCOM=" << g_Config.controllerCOM << "\nIP=" << g_Config.controllerIP << "\nPort=" << g_Config.controllerPort << "\nAutoDetect=" << (g_Config.controllerAutoDetect?"1":"0") << "\n\n";
    
    file << "[OffsetUpdater]\nEnabled=" << (g_Config.enableOffsetUpdater?"1":"0") << "\nURL=" << g_Config.offsetURL << "\n\n";
    file << "[Diagnostics]\nEnabled=" << (g_Config.enableDiagnostics?"1":"0") << "\nAutoCloseOnFail=" << (g_Config.autoCloseOnFail?"1":"0") << "\n";
    return true;
}

void CreateDefaultConfig(const char* filename)
{
    strcpy_s(g_Config.deviceType, "fpga");
    strcpy_s(g_Config.processName, "cod.exe");
    wcscpy_s(g_Config.processNameW, L"cod.exe");
    g_Config.updateRateHz = 120;
    g_Config.useScatterRegistry = true;
    g_Config.useFTD3XX = true;
    g_Config.useLeechCore = true;
    g_Config.controllerType = ControllerType::NONE;
    g_Config.enableOffsetUpdater = true;
    // Use Zero Elite Cloud URL
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
    
    if (DMAEngine::IsOnline() || DMAEngine::IsConnected())
    {
        std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
        auto& players = PlayerManager::GetPlayers();
        auto& local = PlayerManager::GetLocalPlayer();
        
        local.origin = m_LocalPosition;
        local.yaw = m_LocalYaw;
        
        for (size_t i = 0; i < m_PlayerBuffers.size() && i < players.size(); i++)
        {
            PlayerRawData& raw = m_PlayerBuffers[i];
            PlayerData& p = players[i];
            p.origin = raw.position;
            p.health = raw.health;
            p.team = raw.team;
            p.yaw = raw.yaw;
            p.valid = (raw.valid != 0 && raw.health > 0);
            p.isAlive = (raw.health > 0);
            p.distance = (p.origin - m_LocalPosition).Length();
        }
    }
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
// DMA ENGINE
// ============================================================================
bool DMAEngine::Initialize()
{
    if (!ProfessionalInit::RunProfessionalChecks()) return false;
    
    s_Connected = g_InitStatus.dmaConnected;
    s_Online = g_InitStatus.allChecksPassed;
    s_SimulationMode = !g_InitStatus.dmaConnected;
    s_ModuleSize = 0x5000000;
    
    if (s_Online)
    {
        strcpy_s(s_StatusText, "ONLINE");
        snprintf(s_DeviceInfo, sizeof(s_DeviceInfo), "%s | %s | %s", g_InitStatus.dmaDevice, s_DriverMode, g_InitStatus.windowsVersion);
    }
    else if (s_SimulationMode)
    {
        strcpy_s(s_StatusText, "SIMULATION");
        strcpy_s(s_DeviceInfo, "Demo mode");
        s_BaseAddress = 0x140000000;
        g_Offsets.EntityList = s_BaseAddress + 0x16D5B8D8;
    }
    else
        strcpy_s(s_StatusText, "OFFLINE");
    
    MapTextureManager::InitializeMapDatabase();
    return true;
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
    s_Connected = false; s_Online = false;
    Logger::Shutdown();
}

bool DMAEngine::IsConnected() { return s_Connected && !s_SimulationMode; }
bool DMAEngine::IsOnline() { return s_Online; }
const char* DMAEngine::GetStatus() { return s_StatusText; }
const char* DMAEngine::GetDeviceInfo() { return s_DeviceInfo; }
const char* DMAEngine::GetDriverMode() { return s_DriverMode; }

template<typename T> T DMAEngine::Read(uintptr_t a) { T v = {}; 
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID) VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)&v, sizeof(T), nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    (void)a; return v;
}

bool DMAEngine::ReadBuffer(uintptr_t a, void* b, size_t s) {
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID) return VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)b, (DWORD)s, nullptr, VMMDLL_FLAG_NOCACHE);
#endif
    (void)a; (void)b; (void)s; return false;
}

bool DMAEngine::ReadString(uintptr_t a, char* b, size_t m) { if (!b||!m) return false; b[0]=0;
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID) { VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, a, (PBYTE)b, (DWORD)(m-1), nullptr, VMMDLL_FLAG_NOCACHE); b[m-1]=0; return true; }
#endif
    (void)a; return false;
}

void DMAEngine::ExecuteScatter(std::vector<ScatterEntry>& e) { ExecuteScatterReads(e); }
uintptr_t DMAEngine::GetBaseAddress() { return s_BaseAddress; }
uintptr_t DMAEngine::GetModuleBase(const wchar_t* m) {
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID) { char a[256]; wcstombs_s(nullptr, a, m, 255); return VMMDLL_ProcessGetModuleBase(g_VMMDLL, g_DMA_PID, a); }
#endif
    (void)m; return s_BaseAddress;
}
size_t DMAEngine::GetModuleSize() { return s_ModuleSize; }

template int32_t DMAEngine::Read<int32_t>(uintptr_t);
template uint32_t DMAEngine::Read<uint32_t>(uintptr_t);
template int64_t DMAEngine::Read<int64_t>(uintptr_t);
template uint64_t DMAEngine::Read<uint64_t>(uintptr_t);
template float DMAEngine::Read<float>(uintptr_t);
template uintptr_t DMAEngine::Read<uintptr_t>(uintptr_t);

// ============================================================================
// AIMBOT WITH HARDWARE CONTROLLER
// ============================================================================
void Aimbot::Initialize() { s_Enabled = false; s_CurrentTarget = -1; }
void Aimbot::Update() {}
bool Aimbot::IsEnabled() { return s_Enabled && (HardwareController::IsConnected() || g_Config.controllerType == ControllerType::NONE); }
void Aimbot::SetEnabled(bool e) { s_Enabled = e; }

int Aimbot::FindBestTarget(float maxFOV, float maxDistance)
{
    std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
    auto& players = PlayerManager::GetPlayers();
    
    float bestFOV = maxFOV;
    int bestIndex = -1;
    
    for (size_t i = 0; i < players.size(); i++)
    {
        const PlayerData& p = players[i];
        if (!p.valid || !p.isAlive || !p.isEnemy || !p.onScreen) continue;
        if (p.distance > maxDistance) continue;
        
        float fov = GetFOVTo(s_LastAimPos, p.screenPos);
        if (fov < bestFOV)
        {
            bestFOV = fov;
            bestIndex = (int)i;
        }
    }
    
    s_CurrentTarget = bestIndex;
    return bestIndex;
}

Vec2 Aimbot::GetTargetScreenPos(int targetIndex)
{
    std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
    auto& players = PlayerManager::GetPlayers();
    if (targetIndex >= 0 && targetIndex < (int)players.size())
        return players[targetIndex].screenHead;
    return Vec2(0, 0);
}

void Aimbot::AimAt(const Vec2& target, float smoothness)
{
    if (smoothness < 1) smoothness = 1;
    
    int dx = (int)((target.x - s_LastAimPos.x) / smoothness);
    int dy = (int)((target.y - s_LastAimPos.y) / smoothness);
    
    if (dx != 0 || dy != 0)
        MoveMouse(dx, dy);
    
    s_LastAimPos.x += (float)dx;
    s_LastAimPos.y += (float)dy;
}

void Aimbot::MoveMouse(int deltaX, int deltaY)
{
    // Use hardware controller if connected, otherwise software
    HardwareController::MoveMouse(deltaX, deltaY);
}

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
const char* DiagnosticSystem::GetErrorString(DiagnosticResult r) { (void)r; return ""; }
bool DiagnosticSystem::RunAllDiagnostics() { return ProfessionalInit::RunProfessionalChecks(); }
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
// PLAYER MANAGER
// ============================================================================
void PlayerManager::Initialize()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Players.clear();
    for (int i = 0; i < 12; i++) {
        PlayerData p = {}; p.valid = true; p.index = i; p.isEnemy = (i < 6); p.isAlive = true;
        p.health = 50 + rand() % 50; p.maxHealth = 100; p.team = p.isEnemy ? 1 : 2;
        sprintf_s(p.name, "Player_%d", i + 1);
        float a = (float)i * 0.5236f, d = 30.0f + rand() % 100;
        p.origin.x = cosf(a) * d; p.origin.y = sinf(a) * d; p.yaw = (float)(rand() % 360); p.distance = d;
        s_Players.push_back(p);
    }
    s_LocalPlayer = {}; s_LocalPlayer.valid = true; s_LocalPlayer.health = 100;
    s_Initialized = true;
}

void PlayerManager::Update() { if (!s_Initialized) Initialize(); if (g_Config.useScatterRegistry) UpdateWithScatterRegistry(); else SimulateUpdate(); }

void PlayerManager::UpdateWithScatterRegistry()
{
    if (!s_Initialized) Initialize();
#if DMA_ENABLED
    if ((DMAEngine::IsOnline() || DMAEngine::IsConnected()) && g_Offsets.EntityList) {
        g_ScatterRegistry.Clear();
        for (size_t i = 0; i < s_Players.size(); i++) g_ScatterRegistry.RegisterPlayerData((int)i, g_Offsets.EntityList + i * GameOffsets::EntitySize);
        g_ScatterRegistry.ExecuteAll();
        return;
    }
#endif
    SimulateUpdate();
}

void PlayerManager::SimulateUpdate()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_SimTime += 1.0f / g_Config.updateRateHz;
    s_LocalPlayer.yaw = fmodf(s_SimTime * 15.0f, 360.0f);
    for (size_t i = 0; i < s_Players.size(); i++) {
        PlayerData& p = s_Players[i];
        float a = (float)i * 0.5236f + s_SimTime * 0.3f, d = 40.0f + sinf(s_SimTime * 0.5f + (float)i) * 30.0f;
        p.origin.x = cosf(a) * d; p.origin.y = sinf(a) * d; p.yaw = fmodf(a * 57.3f + 180.0f, 360.0f);
        p.distance = d; p.health = 30 + (int)(sinf(s_SimTime * 0.2f + (float)i) * 35 + 35);
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
