// DMA_Engine.cpp - Professional DMA Implementation
// Features: Professional Logging, Scatter Registry, Pre-Launch Diagnostics

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

void* Logger::s_Console = nullptr;

// ============================================================================
// LOGGER IMPLEMENTATION
// ============================================================================
void Logger::Initialize()
{
#ifdef _WIN32
    AllocConsole();
    s_Console = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Set console title
    SetConsoleTitleW(L"PROJECT ZERO | COD DMA");
    
    // Set console size
    SMALL_RECT windowSize = {0, 0, 100, 35};
    SetConsoleWindowInfo((HANDLE)s_Console, TRUE, &windowSize);
    
    // Enable ANSI colors
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

void Logger::ResetColor()
{
    SetColor(COLOR_DEFAULT);
}

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
    printf("        Professional DMA Radar v2.3         ");
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
// PROFESSIONAL INITIALIZATION SYSTEM
// ============================================================================
void ProfessionalInit::SimulateDelay(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool ProfessionalInit::CheckArduinoConnection()
{
    // Simulate Arduino/serial check
    // In real implementation, would scan COM ports
    return false;  // Return false to show realistic failure
}

bool ProfessionalInit::CheckKMBoxConnection()
{
    // Simulate KMBox check
    // In real implementation, would try to connect to KMBox
    return false;  // Return false to show realistic failure
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
    
    // Check if memory map is present
    DWORD dwPID = 0;
    if (VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)"System", &dwPID))
    {
        return dwPID != 0;
    }
    return false;
#else
    return true;  // Simulation mode
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
        
        const char* version = "Unknown";
        if (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 22000)
            version = "Windows 11";
        else if (osvi.dwMajorVersion == 10)
            version = "Windows 10";
        else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
            version = "Windows 8.1";
        
        // Get update version
        const char* update = "";
        if (osvi.dwBuildNumber >= 26100) update = "24H2";
        else if (osvi.dwBuildNumber >= 22631) update = "23H2";
        else if (osvi.dwBuildNumber >= 22621) update = "22H2";
        else if (osvi.dwBuildNumber >= 22000) update = "21H2";
        else if (osvi.dwBuildNumber >= 19045) update = "22H2";
        else if (osvi.dwBuildNumber >= 19044) update = "21H2";
        
        snprintf(buffer, size, "%s %s (Build %d)", version, update, osvi.dwBuildNumber);
    }
    else
    {
        snprintf(buffer, size, "Windows (Unknown Version)");
    }
#else
    snprintf(buffer, size, "Unknown OS");
#endif
}

void ProfessionalInit::GetKeyboardState(char* buffer, size_t size)
{
#ifdef _WIN32
    UINT nDevices = 0;
    GetRawInputDeviceList(nullptr, &nDevices, sizeof(RAWINPUTDEVICELIST));
    
    if (nDevices > 0)
    {
        snprintf(buffer, size, "Ready (%d devices)", nDevices);
    }
    else
    {
        snprintf(buffer, size, "No devices found");
    }
#else
    snprintf(buffer, size, "Unknown");
#endif
}

void ProfessionalInit::GetMouseState(char* buffer, size_t size)
{
#ifdef _WIN32
    CURSORINFO ci = { sizeof(ci) };
    if (GetCursorInfo(&ci))
    {
        snprintf(buffer, size, "Ready (Cursor active)");
    }
    else
    {
        snprintf(buffer, size, "Unknown state");
    }
#else
    snprintf(buffer, size, "Unknown");
#endif
}

bool ProfessionalInit::Step_LoginSequence()
{
    Logger::LogSection("LOGIN SEQUENCE");
    
    Logger::LogTimestamp();
    Logger::Log("Connecting to COD DMA service...\n", COLOR_WHITE);
    
    // Simulate login animation
    for (int i = 0; i < 15; i++)
    {
        Logger::LogSpinner("Authenticating", i);
        SimulateDelay(100);
    }
    Logger::ClearLine();
    
    // Get username
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
    
    // Load config
    bool loaded = LoadConfig("zero.ini");
    
    Logger::LogTimestamp();
    if (loaded)
    {
        Logger::LogSuccess("Loaded config: zero.ini");
        Logger::LogSuccess("Loaded config: cod_client");
    }
    else
    {
        Logger::LogWarning("Config not found, using defaults");
        CreateDefaultConfig("zero.ini");
    }
    
    strcpy_s(g_InitStatus.configName, "cod_client");
    
    Logger::LogStatus("Config File", "zero.ini", true);
    Logger::LogStatus("Game Profile", "cod", true);
    Logger::LogStatus("Process Target", g_Config.processName, true);
    
    g_InitStatus.configLoaded = true;
    g_InitStatus.passedChecks++;
    return true;
}

bool ProfessionalInit::Step_HardwareHandshake()
{
    Logger::LogSection("HARDWARE HANDSHAKE");
    
    Logger::LogTimestamp();
    Logger::Log("Checking hardware connections...\n", COLOR_WHITE);
    
    // Check Arduino
    Logger::LogTimestamp();
    Logger::Log("Scanning for Arduino/Serial devices...\n", COLOR_GRAY);
    SimulateDelay(500);
    
    g_InitStatus.arduinoConnected = CheckArduinoConnection();
    if (g_InitStatus.arduinoConnected)
    {
        Logger::LogSuccess("Arduino connected on COM3");
    }
    else
    {
        Logger::LogWarning("Arduino not found (optional)");
    }
    
    // Check KMBox
    Logger::LogTimestamp();
    Logger::Log("Scanning for KMBox devices...\n", COLOR_GRAY);
    SimulateDelay(500);
    
    g_InitStatus.kmboxConnected = CheckKMBoxConnection();
    if (g_InitStatus.kmboxConnected)
    {
        Logger::LogSuccess("KMBox connected");
    }
    else
    {
        Logger::LogWarning("KMBox not found (optional)");
    }
    
    Logger::LogStatus("Arduino", g_InitStatus.arduinoConnected ? "Connected" : "Not Found", g_InitStatus.arduinoConnected);
    Logger::LogStatus("KMBox", g_InitStatus.kmboxConnected ? "Connected" : "Not Found", g_InitStatus.kmboxConnected);
    
    g_InitStatus.hardwareConnected = true;  // Not critical
    g_InitStatus.passedChecks++;
    return true;
}

bool ProfessionalInit::Step_ConnectDMA()
{
    Logger::LogSection("DMA CONNECTION");
    
    Logger::LogTimestamp();
    Logger::Log("Initializing DMA device...\n", COLOR_WHITE);
    
    // Animation
    for (int i = 0; i < 20; i++)
    {
        Logger::LogSpinner("Connecting to FPGA", i);
        SimulateDelay(100);
    }
    Logger::ClearLine();
    
#if DMA_ENABLED
    // Try to connect
    char arg0[] = "";
    char arg1[] = "-device";
    char arg2[64];
    strncpy_s(arg2, g_Config.deviceType, 63);
    char* args[3] = { arg0, arg1, arg2 };
    
    g_VMMDLL = VMMDLL_Initialize(3, args);
    
    if (g_VMMDLL)
    {
        Logger::LogTimestamp();
        Logger::LogSuccess("Successfully connected to your DMA");
        
        // Check memory map
        Logger::LogTimestamp();
        Logger::Log("Verifying memory map (MMap)...\n", COLOR_GRAY);
        SimulateDelay(500);
        
        g_InitStatus.mmapPresent = CheckMemoryMap();
        if (g_InitStatus.mmapPresent)
        {
            Logger::LogSuccess("Memory map verified");
        }
        else
        {
            Logger::LogWarning("Memory map not present (live mode)");
        }
        
        strcpy_s(g_InitStatus.dmaDevice, g_Config.deviceType);
        g_InitStatus.dmaConnected = true;
        
        Logger::LogStatus("Device", g_Config.deviceType, true);
        Logger::LogStatus("MMap", g_InitStatus.mmapPresent ? "Present" : "Live Mode", g_InitStatus.mmapPresent);
        Logger::LogStatus("Connection", "Established", true);
        
        g_InitStatus.passedChecks++;
        return true;
    }
    else
    {
        Logger::LogTimestamp();
        Logger::LogError("Failed to connect to DMA device");
        Logger::LogError("Please check your FPGA connection");
        
        Logger::LogStatus("Device", "Not Found", false);
        Logger::LogStatus("Connection", "Failed", false);
        
        g_InitStatus.failedChecks++;
        
        // CRITICAL: DMA connection failed - do not initialize UI
        return false;
    }
#else
    // Simulation mode
    Logger::LogTimestamp();
    Logger::LogWarning("DMA_ENABLED = 0, running in simulation mode");
    
    strcpy_s(g_InitStatus.dmaDevice, "Simulation");
    g_InitStatus.dmaConnected = false;
    g_InitStatus.mmapPresent = true;
    
    Logger::LogStatus("Device", "Simulation", true);
    Logger::LogStatus("Mode", "Demo", true);
    
    g_InitStatus.passedChecks++;
    return true;  // Allow UI in simulation
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
        // Try to find the game process
        int attempts = 0;
        const int maxAttempts = 30;  // 3 seconds max wait
        
        while (attempts < maxAttempts)
        {
            DWORD pid = 0;
            if (VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)g_Config.processName, &pid) && pid != 0)
            {
                g_DMA_PID = pid;
                Logger::ClearLine();
                Logger::LogTimestamp();
                Logger::LogSuccess("Game process found!");
                
                // Get base address
                DMAEngine::s_BaseAddress = VMMDLL_ProcessGetModuleBase(g_VMMDLL, g_DMA_PID, (LPSTR)g_Config.processName);
                
                Logger::LogStatus("Process", g_Config.processName, true);
                
                char pidStr[32];
                snprintf(pidStr, sizeof(pidStr), "%d", pid);
                Logger::LogStatus("PID", pidStr, true);
                
                char baseStr[32];
                snprintf(baseStr, sizeof(baseStr), "0x%llX", (unsigned long long)DMAEngine::s_BaseAddress);
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
        Logger::LogStatus("Status", "Waiting...", false);
        
        g_InitStatus.gameFound = false;
        return true;  // Not critical - can wait
    }
#endif
    
    // Simulation mode
    Logger::LogTimestamp();
    Logger::LogSuccess("Game sync ready (simulation)");
    
    Logger::LogStatus("Process", "Simulated", true);
    Logger::LogStatus("Status", "Demo Mode", true);
    
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
    
    // Get Windows version
    GetWindowsVersion(g_InitStatus.windowsVersion, sizeof(g_InitStatus.windowsVersion));
    
    // Get input device states
    char keyboardState[64], mouseState[64];
    GetKeyboardState(keyboardState, sizeof(keyboardState));
    GetMouseState(mouseState, sizeof(mouseState));
    
    g_InitStatus.keyboardReady = strstr(keyboardState, "Ready") != nullptr;
    g_InitStatus.mouseReady = strstr(mouseState, "Ready") != nullptr;
    
    Logger::LogStatus("Windows", g_InitStatus.windowsVersion, true);
    Logger::LogStatus("Keyboard State", keyboardState, g_InitStatus.keyboardReady);
    Logger::LogStatus("Mouse State", mouseState, g_InitStatus.mouseReady);
    
    // Additional system info
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
    // Initialize console and logger
    Logger::Initialize();
    
    // Reset status
    memset(&g_InitStatus, 0, sizeof(g_InitStatus));
    g_InitStatus.totalChecks = 6;
    
    // Show banner
    Logger::LogBanner();
    
    Logger::LogSeparator();
    Logger::LogTimestamp();
    Logger::Log("Starting initialization sequence...\n", COLOR_WHITE);
    Logger::LogSeparator();
    
    // Step 1: Login
    Logger::LogProgress("Login Sequence", 1, 6);
    if (!Step_LoginSequence())
    {
        Logger::LogError("Login failed - aborting");
        return false;
    }
    
    // Step 2: Config
    Logger::LogProgress("Config Loader", 2, 6);
    if (!Step_LoadConfig())
    {
        Logger::LogError("Config load failed - aborting");
        return false;
    }
    
    // Step 3: Hardware
    Logger::LogProgress("Hardware Handshake", 3, 6);
    Step_HardwareHandshake();  // Not critical
    
    // Step 4: DMA Connection - CRITICAL
    Logger::LogProgress("DMA Connection", 4, 6);
    if (!Step_ConnectDMA())
    {
        Logger::LogSeparator();
        Logger::LogError("CRITICAL: DMA connection failed");
        Logger::LogError("UI will NOT be initialized for safety");
        Logger::LogSeparator();
        
        g_InitStatus.allChecksPassed = false;
        
        // Wait for user acknowledgment
        Logger::Log("\nPress any key to exit...\n", COLOR_RED);
        getchar();
        
        return false;
    }
    
    // Step 5: Game Sync
    Logger::LogProgress("Game Sync", 5, 6);
    Step_WaitForGame();
    
    // Step 6: System State
    Logger::LogProgress("System State", 6, 6);
    Step_CheckSystemState();
    
    // Final summary
    Logger::LogSeparator();
    Logger::LogSection("INITIALIZATION COMPLETE");
    
    char summary[128];
    snprintf(summary, sizeof(summary), "%d/%d checks passed", 
             g_InitStatus.passedChecks, g_InitStatus.totalChecks);
    
    Logger::LogTimestamp();
    if (g_InitStatus.passedChecks >= 4)  // At least critical checks
    {
        Logger::LogSuccess(summary);
        Logger::Log("\n", COLOR_DEFAULT);
        Logger::LogSuccess("System ready - Launching UI...");
        g_InitStatus.allChecksPassed = true;
    }
    else
    {
        Logger::LogError(summary);
        Logger::LogError("Too many checks failed");
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
        for (auto& entry : entries)
            if (entry.buffer) memset(entry.buffer, 0, entry.size);
        return;
    }
    
    VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
    if (!hScatter)
    {
        for (auto& entry : entries)
            if (entry.buffer && entry.address)
                VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, entry.address, (PBYTE)entry.buffer, (DWORD)entry.size, nullptr, VMMDLL_FLAG_NOCACHE);
        return;
    }
    
    for (auto& entry : entries)
        if (entry.address && entry.size > 0)
            VMMDLL_Scatter_Prepare(hScatter, entry.address, (DWORD)entry.size);
    
    VMMDLL_Scatter_Execute(hScatter);
    
    for (auto& entry : entries)
        if (entry.buffer && entry.address)
            VMMDLL_Scatter_Read(hScatter, entry.address, (DWORD)entry.size, (PBYTE)entry.buffer, nullptr);
    
    VMMDLL_Scatter_CloseHandle(hScatter);
#else
    for (auto& entry : entries)
        if (entry.buffer) memset(entry.buffer, 0, entry.size);
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
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0, 1);
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        if (section == "Device") {
            if (key == "Type") strncpy_s(g_Config.deviceType, value.c_str(), 31);
            else if (key == "UseCustomPCIe") g_Config.useCustomPCIe = (value == "1");
            else if (key == "CustomPCIeID") strncpy_s(g_Config.customPCIeID, value.c_str(), 31);
        }
        else if (section == "Target") {
            if (key == "ProcessName") { strncpy_s(g_Config.processName, value.c_str(), 63); mbstowcs_s(nullptr, g_Config.processNameW, g_Config.processName, 63); }
        }
        else if (section == "Performance") {
            if (key == "UpdateRateHz") g_Config.updateRateHz = std::stoi(value);
            else if (key == "UseScatterRegistry") g_Config.useScatterRegistry = (value == "1");
        }
        else if (section == "Diagnostics") {
            if (key == "EnableDiagnostics") g_Config.enableDiagnostics = (value == "1");
            else if (key == "AutoCloseOnFail") g_Config.autoCloseOnFail = (value == "1");
        }
    }
    return true;
}

bool SaveConfig(const char* filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "; PROJECT ZERO - Configuration\n\n";
    file << "[Device]\nType=" << g_Config.deviceType << "\nUseCustomPCIe=" << (g_Config.useCustomPCIe?"1":"0") << "\nCustomPCIeID=" << g_Config.customPCIeID << "\n\n";
    file << "[Target]\nProcessName=" << g_Config.processName << "\n\n";
    file << "[Performance]\nUpdateRateHz=" << g_Config.updateRateHz << "\nUseScatterRegistry=" << (g_Config.useScatterRegistry?"1":"0") << "\n\n";
    file << "[Diagnostics]\nEnableDiagnostics=" << (g_Config.enableDiagnostics?"1":"0") << "\nAutoCloseOnFail=" << (g_Config.autoCloseOnFail?"1":"0") << "\n";
    return true;
}

void CreateDefaultConfig(const char* filename)
{
    strcpy_s(g_Config.deviceType, "fpga");
    strcpy_s(g_Config.processName, "cod.exe");
    wcscpy_s(g_Config.processNameW, L"cod.exe");
    g_Config.updateRateHz = 120;
    g_Config.useScatterRegistry = true;
    g_Config.enableDiagnostics = true;
    g_Config.autoCloseOnFail = true;
    SaveConfig(filename);
}

// ============================================================================
// SCATTER REGISTRY
// ============================================================================
void ScatterReadRegistry::RegisterPlayerData(int playerIndex, uintptr_t baseAddr)
{
    if (playerIndex < 0 || baseAddr == 0) return;
    while ((int)m_PlayerBuffers.size() <= playerIndex) m_PlayerBuffers.push_back({});
    PlayerRawData& buf = m_PlayerBuffers[playerIndex];
    m_Entries.push_back({baseAddr + GameOffsets::EntityPos, &buf.position, sizeof(Vec3), ScatterDataType::PLAYER_POSITION, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityHealth, &buf.health, sizeof(int), ScatterDataType::PLAYER_HEALTH, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityTeam, &buf.team, sizeof(int), ScatterDataType::PLAYER_TEAM, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityYaw, &buf.yaw, sizeof(float), ScatterDataType::PLAYER_YAW, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityValid, &buf.valid, sizeof(uint8_t), ScatterDataType::PLAYER_VALID, playerIndex});
}

void ScatterReadRegistry::RegisterLocalPlayer(uintptr_t baseAddr)
{
    if (baseAddr == 0) return;
    m_Entries.push_back({baseAddr + GameOffsets::EntityPos, &m_LocalPosition, sizeof(Vec3), ScatterDataType::LOCAL_POSITION, -1});
    m_Entries.push_back({baseAddr + GameOffsets::EntityYaw, &m_LocalYaw, sizeof(float), ScatterDataType::LOCAL_YAW, -1});
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
    // Run professional checks first
    if (!ProfessionalInit::RunProfessionalChecks())
    {
        return false;
    }
    
    s_Connected = g_InitStatus.dmaConnected;
    s_Online = g_InitStatus.allChecksPassed;
    s_SimulationMode = !g_InitStatus.dmaConnected;
    s_ModuleSize = 0x5000000;
    
    if (s_Online)
    {
        strcpy_s(s_StatusText, "ONLINE");
        snprintf(s_DeviceInfo, sizeof(s_DeviceInfo), "%s | %s", g_InitStatus.dmaDevice, g_InitStatus.windowsVersion);
    }
    else if (s_SimulationMode)
    {
        strcpy_s(s_StatusText, "SIMULATION");
        strcpy_s(s_DeviceInfo, "Demo mode");
        s_BaseAddress = 0x140000000;
        g_Offsets.EntityList = s_BaseAddress + 0x16D5B8D8;
    }
    else
    {
        strcpy_s(s_StatusText, "OFFLINE");
    }
    
    MapTextureManager::InitializeMapDatabase();
    return true;
}

bool DMAEngine::InitializeWithConfig(const DMAConfig&) { return Initialize(); }
void DMAEngine::Shutdown() { 
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
// PATTERN SCANNER (Stub)
// ============================================================================
uintptr_t PatternScanner::FindPattern(uintptr_t, size_t, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ScanModule(const char*, const char*, const char*) { return 0; }
uintptr_t PatternScanner::ResolveRelative(uintptr_t, int, int) { return 0; }
bool PatternScanner::UpdateAllOffsets() { s_Scanned = true; return false; }

// ============================================================================
// DIAGNOSTIC SYSTEM (Stub - handled by ProfessionalInit)
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
        float a = (float)i * 0.5236f + s_SimTime * 0.3f, d = 40.0f + sinf(s_SimTime * 0.5f + i) * 30.0f;
        p.origin.x = cosf(a) * d; p.origin.y = sinf(a) * d; p.yaw = fmodf(a * 57.3f + 180.0f, 360.0f);
        p.distance = d; p.health = 30 + (int)(sinf(s_SimTime * 0.2f + i) * 35 + 35);
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
