// DMA_Engine.hpp - STRICT AUTOMATION v4.3
// Zero Elite - Black Stage Architecture

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <functional>

#define DMA_ENABLED 1
#define KMBOX_ENABLED 1
#define DEFAULT_OFFSET_URL "https://raw.githubusercontent.com/offsets/bo6/main/offsets.txt"

// ============================================================================
// MATH TYPES
// ============================================================================
struct Vec2 { 
    float x = 0, y = 0; 
    Vec2() = default; 
    Vec2(float _x, float _y) : x(_x), y(_y) {} 
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    float Length() const { return sqrtf(x*x + y*y); }
};

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float Length() const { return sqrtf(x*x + y*y + z*z); }
    float Length2D() const { return sqrtf(x*x + y*y); }
};

struct Matrix4x4 { float m[4][4] = {}; };

// ============================================================================
// CONFIGURATION
// ============================================================================
enum class ControllerType { NONE, KMBOX_B_PLUS, KMBOX_NET, ARDUINO };

struct DMAConfig {
    char deviceName[64] = "fpga";
    char processName[64] = "cod.exe";
    wchar_t processNameW[64] = L"cod.exe";
    uint32_t pid = 0;
    uintptr_t handle = 0;
    bool useFTD3XX = true;
    bool useScatterReads = true;
    bool useScatterRegistry = true;
    bool enableOffsetUpdater = true;
    bool enableDiagnostics = true;
    bool autoCloseOnFail = false;
    int maxRetries = 3;
    int readTimeout = 100;
    int updateRateHz = 120;
    
    // Controller settings
    ControllerType controllerType = ControllerType::NONE;
    bool controllerAutoDetect = true;
    char controllerCOM[16] = "COM3";
    char controllerIP[32] = "192.168.2.188";
    int controllerPort = 8888;
    char offsetURL[512] = DEFAULT_OFFSET_URL;
};

struct GameOffsetsStruct {
    uintptr_t ClientInfo = 0;
    uintptr_t EntityList = 0;
    uintptr_t ViewMatrix = 0;
    uintptr_t LocalPlayer = 0;
    uintptr_t PlayerBase = 0;
    uintptr_t Refdef = 0;
    uintptr_t BoneMatrix = 0;
    uintptr_t WeaponInfo = 0;

    static constexpr uintptr_t EntitySize = 0x568;
    static constexpr uintptr_t EntityPos = 0x138;
    static constexpr uintptr_t EntityHealth = 0x1C8;
    static constexpr uintptr_t EntityMaxHealth = 0x1CC;
    static constexpr uintptr_t EntityTeam = 0x1D0;
    static constexpr uintptr_t EntityYaw = 0x148;
    static constexpr uintptr_t EntityName = 0x200;
    static constexpr uintptr_t EntityValid = 0x100;
    static constexpr uintptr_t EntityStance = 0x1E0;
};

extern DMAConfig g_Config;
extern GameOffsetsStruct g_Offsets;

// ============================================================================
// PLAYER DATA
// ============================================================================
struct PlayerData {
    Vec3 origin;
    Vec3 headPos;
    Vec2 screenPos;
    float screenHeight = 0;
    float screenWidth = 0;
    bool onScreen = false;
    bool isVisible = false;
    float yaw = 0;
    int health = 100;
    int maxHealth = 100;
    int team = 0;
    int index = 0;
    bool valid = false;
    bool isAlive = true;
    bool isEnemy = false;
    float distance = 0;
    char name[32] = {};
};

// ============================================================================
// SCATTER READ
// ============================================================================
enum class ScatterDataType { PLAYER_POSITION, PLAYER_HEALTH, PLAYER_TEAM, PLAYER_YAW, PLAYER_VALID, LOCAL_POSITION, LOCAL_YAW, VIEW_MATRIX, CUSTOM };

struct ScatterEntry {
    uintptr_t address = 0;
    void* buffer = nullptr;
    size_t size = 0;
    ScatterDataType type = ScatterDataType::CUSTOM;
    int playerIndex = -1;
};

struct PlayerRawData {
    Vec3 position;
    int health = 0;
    int team = 0;
    float yaw = 0;
    uint8_t valid = 0;
};

class ScatterReadRegistry {
public:
    void RegisterPlayerData(int idx, uintptr_t base);
    void RegisterLocalPlayer(uintptr_t base);
    void RegisterViewMatrix(uintptr_t addr);
    void RegisterCustomRead(uintptr_t addr, void* buf, size_t size);
    void ExecuteAll();
    void Clear();
    int GetTotalBytes() const;
    int GetTransactionCount() const { return m_TransactionCount; }
    std::vector<PlayerRawData>& GetPlayerBuffers() { return m_PlayerBuffers; }
    Vec3 GetLocalPosition() const { return m_LocalPosition; }
    float GetLocalYaw() const { return m_LocalYaw; }
    Matrix4x4 GetViewMatrix() const { return m_ViewMatrix; }
    
private:
    std::vector<ScatterEntry> m_Entries;
    std::vector<PlayerRawData> m_PlayerBuffers;
    Vec3 m_LocalPosition;
    float m_LocalYaw = 0;
    Matrix4x4 m_ViewMatrix;
    int m_TransactionCount = 0;
};

extern ScatterReadRegistry g_ScatterRegistry;

// ============================================================================
// LOGGING
// ============================================================================
enum ConsoleColor {
    COLOR_DEFAULT = 7,
    COLOR_RED = 12,
    COLOR_GREEN = 10,
    COLOR_YELLOW = 14,
    COLOR_CYAN = 11,
    COLOR_WHITE = 15,
    COLOR_GRAY = 8,
    COLOR_MAGENTA = 13
};

class Logger {
public:
    static void Initialize();
    static void Shutdown();
    static void ShowConsole();
    static void HideConsole();
    static void SetColor(ConsoleColor);
    static void ResetColor();
    static void Log(const char*, ConsoleColor = COLOR_DEFAULT);
    static void LogSuccess(const char*);
    static void LogError(const char*);
    static void LogWarning(const char*);
    static void LogInfo(const char*);
    static void LogStatus(const char*, const char*, bool);
    static void LogProgress(const char*, int, int);
    static void LogBanner();
    static void LogSection(const char*);
    static void LogSeparator();
    static void LogTimestamp();
    static void LogSpinner(const char*, int);
    static void ClearLine();
private:
    static void* s_Console;
    static bool s_Initialized;
};

// ============================================================================
// INIT STATUS
// ============================================================================
struct InitStatus {
    bool deviceFound = false;
    bool dmaConnected = false;
    bool mMapPresent = false;
    bool gameFound = false;
    bool offsetsValid = false;
    bool controllerConnected = false;
    bool allChecksPassed = false;
    bool configLoaded = false;
    
    char dmaDriver[32] = {0};
    char dmaDevice[64] = {0};
    char controllerName[64] = {0};
    char userName[64] = {0};
    char configName[64] = {0};
    char gameProcess[64] = {0};
    int passedChecks = 0;
    int totalChecks = 0;
    int failedChecks = 0;
};

extern InitStatus g_InitStatus;

// ============================================================================
// DIAGNOSTICS
// ============================================================================
enum class DiagnosticResult { SUCCESS, DEVICE_NOT_FOUND, FIRMWARE_MISMATCH, MEMORY_ERROR, PROCESS_NOT_FOUND, ACCESS_DENIED };

struct DiagnosticStatus {
    DiagnosticResult deviceCheck = DiagnosticResult::SUCCESS;
    DiagnosticResult firmwareCheck = DiagnosticResult::SUCCESS;
    DiagnosticResult memoryCheck = DiagnosticResult::SUCCESS;
    DiagnosticResult processCheck = DiagnosticResult::SUCCESS;
    float readSpeed = 0;
    int latency = 0;
    bool allPassed = true;
};

extern DiagnosticStatus g_DiagStatus;

class DiagnosticSystem {
public:
    static const char* GetErrorString(DiagnosticResult);
    static bool RunAllDiagnostics();
    static DiagnosticResult CheckDeviceHandshake();
    static DiagnosticResult CheckFirmwareIntegrity();
    static DiagnosticResult CheckMemorySpeed();
    static DiagnosticResult CheckProcessAccess();
    static void SelfDestruct(const char*);
    static void ShowErrorPopup(const char*, const char*);
    static bool IsGenericDeviceID(const char*);
    static bool IsLeakedDeviceID(const char*);
    static float MeasureReadSpeed(uintptr_t, size_t, int);
    static int MeasureLatency(uintptr_t, int);
};

// ============================================================================
// DMA ENGINE
// ============================================================================
class DMAEngine {
public:
    static bool Initialize();
    static bool InitializeWithConfig(const DMAConfig&);
    static bool InitializeFTD3XX();
    static void Shutdown();
    static bool IsConnected();
    static bool IsOnline();
    static const char* GetStatus();
    static const char* GetDeviceInfo();
    static const char* GetDriverMode();
    static void Update();
    
    template<typename T> static T Read(uintptr_t);
    static bool ReadBuffer(uintptr_t, void*, size_t);
    static bool ReadString(uintptr_t, char*, size_t);
    static void ExecuteScatter(std::vector<ScatterEntry>&);
    static uintptr_t GetBaseAddress();
    static uintptr_t GetModuleBase(const wchar_t*);
    static size_t GetModuleSize();
    
    static bool s_Connected;
    static bool s_Online;
    static bool s_SimulationMode;
    static bool s_UsingFTD3XX;
    static uintptr_t s_BaseAddress;
    static size_t s_ModuleSize;
    static char s_StatusText[64];
    static char s_DeviceInfo[128];
    static char s_DriverMode[32];
};

// ============================================================================
// HARDWARE CONTROLLER
// ============================================================================
struct ControllerConfig {
    ControllerType type = ControllerType::NONE;
    char comPort[16] = "COM3";
    char ipAddress[32] = "192.168.2.188";
    int port = 16896;
    int baudRate = 115200;
    bool isConnected = false;
    char deviceName[64] = "None";
};

extern ControllerConfig g_ControllerConfig;

class HardwareController {
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsConnected();
    static const char* GetDeviceName();
    static ControllerType GetType();
    static const char* GetLockedPort();
    
    static bool MoveMouse(int dx, int dy);
    static bool Click(int button);
    static bool Press(int button);
    static bool Release(int button);
    static bool KeyPress(int key);
    static bool KeyRelease(int key);
    
    static bool ScanCOMPorts(std::vector<std::string>&);
    static bool AutoDetectDevice();
    static bool ConnectKMBoxBPlus(const char*, int);
    static bool ConnectKMBoxNet(const char*, int);
    static bool ConnectArduino(const char*, int);
    
    static void* s_Handle;
    static void* s_Socket;
    static bool s_Connected;
    static ControllerType s_Type;
    static char s_DeviceName[64];
    static char s_LockedPort[16];
    
private:
    static bool SerialWrite(const char*, int);
    static bool SocketSend(const char*, int);
};

// ============================================================================
// OFFSET UPDATER
// ============================================================================
struct RemoteOffsets {
    uintptr_t ClientInfo = 0;
    uintptr_t EntityList = 0;
    uintptr_t ViewMatrix = 0;
    char buildNumber[32] = {};
    bool valid = false;
};

class OffsetUpdater {
public:
    static void SetOffsetURL(const char*);
    static bool FetchRemoteOffsets(const char*);
    static bool UpdateOffsetsFromServer();
    static bool SyncWithCloud(int = 3, int = 1000);
    static bool ParseOffsetsJSON(const char*);
    static bool ParseOffsetsINI(const char*);
    static bool ParseOffsetsTXT(const char*);
    static bool ApplyOffsets(const RemoteOffsets&);
    
    static RemoteOffsets s_LastOffsets;
    static bool s_Updated;
    static bool s_Synced;
    static char s_OffsetURL[512];
    static char s_LastError[256];
    static constexpr const char* DEFAULT_OFFSET_URL = "https://raw.githubusercontent.com/offsets/bo6/main/offsets.txt";
    
private:
    static bool HttpGet(const char*, std::string&);
};

// ============================================================================
// MAP TEXTURE
// ============================================================================
struct MapInfo {
    char name[64] = {};
    char imagePath[256] = {};
    float minX = -5000, maxX = 5000;
    float minY = -5000, maxY = 5000;
    int width = 512, height = 512;
};

class MapTextureManager {
public:
    static void InitializeMapDatabase();
    static bool LoadMapConfig(const char*);
    static bool LoadMapTexture(const char*);
    static Vec2 GameToMapCoords(const Vec3&);
    static Vec2 GameToRadarCoords(const Vec3&, const Vec3&, float, float, float, float, float);
    static void SetMapBounds(const char*, float, float, float, float);
    
    static MapInfo s_CurrentMap;
    static std::unordered_map<std::string, MapInfo> s_MapDatabase;
};

// ============================================================================
// PATTERN SCANNER
// ============================================================================
class PatternScanner {
public:
    static uintptr_t FindPattern(uintptr_t, size_t, const char*, const char*);
    static uintptr_t ScanModule(const char*, const char*, const char*);
    static uintptr_t ResolveRelative(uintptr_t, int, int);
    static bool UpdateAllOffsets();
    static bool s_Scanned;
    static int s_FoundCount;
};

// ============================================================================
// PROFESSIONAL INIT
// ============================================================================
class ProfessionalInit {
public:
    static bool RunProfessionalChecks();
    static void SimulateDelay(int);
    static bool CheckDMAConnection();
    static bool CheckMemoryMap();
    static void GetWindowsVersion(char*, size_t);
    static void GetKeyboardState(char*, size_t);
    static void GetMouseState(char*, size_t);
    
    static bool Step_LoginSequence();
    static bool Step_LoadConfig();
    static bool Step_CheckOffsetUpdates();
    static bool Step_ConnectController();
    static bool Step_ConnectDMA();
    static bool Step_WaitForGame();
    static bool Step_CheckSystemState();
};

// ============================================================================
// PLAYER MANAGER
// ============================================================================
class PlayerManager {
public:
    static void Initialize();
    static void Update();
    static void UpdateWithScatterRegistry();
    static void SimulateUpdate();
    static void RealUpdate();
    static std::vector<PlayerData>& GetPlayers();
    static PlayerData& GetLocalPlayer();
    static int GetAliveCount();
    static int GetEnemyCount();
    static std::mutex& GetMutex() { return s_Mutex; }
    
    static std::vector<PlayerData> s_Players;
    static PlayerData s_LocalPlayer;
    static std::mutex s_Mutex;
    static bool s_Initialized;
    static float s_SimTime;
};

// ============================================================================
// AIMBOT
// ============================================================================
class Aimbot {
public:
    static void Initialize();
    static void Update();
    static bool IsEnabled();
    static void SetEnabled(bool);
    
    static bool s_Enabled;
    static int s_CurrentTarget;
    static Vec2 s_LastAimPos;
};

// ============================================================================
// UTILITIES
// ============================================================================
bool WorldToScreen(const Vec3& world, Vec2& screen, int width, int height);
bool WorldToRadar(const Vec3& world, const Vec3& local, float yaw, Vec2& radar, float cx, float cy, float size);
float GetFOVTo(const Vec2& center, const Vec2& target);
Vec3 CalcAngle(const Vec3& source, const Vec3& destination);
void SmoothAngle(Vec3& current, const Vec3& target, float smooth);
const char* ControllerTypeToString(ControllerType type);
void CreateDefaultConfig(const char* filename);
