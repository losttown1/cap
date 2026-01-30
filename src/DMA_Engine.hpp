// DMA_Engine.hpp - Professional DMA Engine
// Features: Professional Logging, Pre-Launch Diagnostics, Scatter Registry

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <functional>

// ============================================================================
// DMA CONFIGURATION
// ============================================================================
#define DMA_ENABLED 1

// ============================================================================
// CONSOLE COLORS (Windows)
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

// ============================================================================
// INITIALIZATION STATUS
// ============================================================================
struct InitStatus {
    bool loginSuccess = false;
    bool configLoaded = false;
    bool hardwareConnected = false;
    bool arduinoConnected = false;
    bool kmboxConnected = false;
    bool dmaConnected = false;
    bool mmapPresent = false;
    bool gameFound = false;
    bool keyboardReady = false;
    bool mouseReady = false;
    bool allChecksPassed = false;
    
    char windowsVersion[64] = {0};
    char userName[64] = {0};
    char configName[64] = {0};
    char dmaDevice[64] = {0};
    char gameProcess[64] = {0};
    
    int totalChecks = 0;
    int passedChecks = 0;
    int failedChecks = 0;
};

extern InitStatus g_InitStatus;

// ============================================================================
// DIAGNOSTIC RESULT CODES
// ============================================================================
enum class DiagnosticResult {
    SUCCESS = 0,
    DEVICE_NOT_FOUND,
    DEVICE_BUSY,
    FIRMWARE_INVALID,
    FIRMWARE_GENERIC,
    FIRMWARE_LEAKED,
    SPEED_TOO_SLOW,
    PROCESS_NOT_FOUND,
    BASE_ADDRESS_FAIL,
    MEMORY_READ_FAIL,
    UNKNOWN_ERROR
};

// ============================================================================
// DIAGNOSTIC STATUS
// ============================================================================
struct DiagnosticStatus {
    bool deviceHandshake = false;
    bool firmwareCheck = false;
    bool speedTest = false;
    bool processFound = false;
    bool memoryAccess = false;
    
    DiagnosticResult lastError = DiagnosticResult::SUCCESS;
    char errorMessage[256] = {0};
    
    char deviceName[64] = {0};
    char firmwareVersion[32] = {0};
    char deviceID[32] = {0};
    bool isGenericID = false;
    bool isLeakedID = false;
    
    float readSpeedMBps = 0;
    float writeSpeedMBps = 0;
    int latencyUs = 0;
    bool speedSufficient = false;
    
    bool allChecksPassed = false;
};

extern DiagnosticStatus g_DiagStatus;

// ============================================================================
// CONFIG STRUCTURE
// ============================================================================
struct DMAConfig {
    char deviceType[32] = "fpga";
    char deviceArg[64] = "";
    char deviceAlgo[16] = "0";
    bool useCustomPCIe = false;
    char customPCIeID[32] = "";
    
    char processName[64] = "cod.exe";
    wchar_t processNameW[64] = L"cod.exe";
    
    int scatterBatchSize = 128;
    int updateRateHz = 120;
    bool useScatterRegistry = true;
    
    bool enableDiagnostics = true;
    bool autoCloseOnFail = true;
    float minSpeedMBps = 50.0f;
    int maxLatencyUs = 5000;
    bool warnGenericID = true;
    bool warnLeakedID = true;
    
    char mapImagePath[256] = "";
    float mapScaleX = 1.0f;
    float mapScaleY = 1.0f;
    float mapOffsetX = 0.0f;
    float mapOffsetY = 0.0f;
    float mapRotation = 0.0f;
    
    bool debugMode = false;
    bool logReads = false;
};

extern DMAConfig g_Config;

bool LoadConfig(const char* filename = "zero.ini");
bool SaveConfig(const char* filename = "zero.ini");
void CreateDefaultConfig(const char* filename = "zero.ini");

// ============================================================================
// PROFESSIONAL LOGGING SYSTEM
// ============================================================================
class Logger {
public:
    static void Initialize();
    static void Shutdown();
    
    // Logging with colors and timestamps
    static void Log(const char* message, ConsoleColor color = COLOR_DEFAULT);
    static void LogSuccess(const char* message);
    static void LogError(const char* message);
    static void LogWarning(const char* message);
    static void LogInfo(const char* message);
    static void LogStatus(const char* label, const char* value, bool success);
    static void LogProgress(const char* message, int current, int total);
    
    // Special formatted logs
    static void LogBanner();
    static void LogSection(const char* title);
    static void LogSeparator();
    static void LogTimestamp();
    
    // Animation
    static void LogSpinner(const char* message, int frame);
    static void ClearLine();
    
private:
    static void SetColor(ConsoleColor color);
    static void ResetColor();
    static void* s_Console;
};

// ============================================================================
// PROFESSIONAL INITIALIZATION SYSTEM
// ============================================================================
class ProfessionalInit {
public:
    // Main initialization function - returns true if all checks pass
    static bool RunProfessionalChecks();
    
    // Individual check steps
    static bool Step_LoginSequence();
    static bool Step_LoadConfig();
    static bool Step_HardwareHandshake();
    static bool Step_ConnectDMA();
    static bool Step_WaitForGame();
    static bool Step_CheckSystemState();
    
    // Get initialization status
    static InitStatus& GetStatus() { return g_InitStatus; }
    static bool IsReady() { return g_InitStatus.allChecksPassed; }
    
private:
    static void SimulateDelay(int ms);
    static bool CheckArduinoConnection();
    static bool CheckKMBoxConnection();
    static bool CheckDMAConnection();
    static bool CheckMemoryMap();
    static void GetWindowsVersion(char* buffer, size_t size);
    static void GetKeyboardState(char* buffer, size_t size);
    static void GetMouseState(char* buffer, size_t size);
};

// ============================================================================
// PATTERN SIGNATURES
// ============================================================================
namespace Patterns {
    constexpr const char* PlayerBase = "\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x88\x00\x00\x00\x00\x48\x8B\x01";
    constexpr const char* PlayerBaseMask = "xxx????xxx????xxx";
    
    constexpr const char* ClientInfo = "\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB\x74\x00\x48\x8B\x03";
    constexpr const char* ClientInfoMask = "xxx????xxxx?xxx";
    
    constexpr const char* EntityList = "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x85\xC0\x74";
    constexpr const char* EntityListMask = "xxx????x????xxxx";
    
    constexpr const char* ViewMatrix = "\x48\x8B\x15\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\xE8";
    constexpr const char* ViewMatrixMask = "xxx????xxxx?x";
}

// ============================================================================
// GAME OFFSETS
// ============================================================================
struct GameOffsets {
    uintptr_t PlayerBase = 0;
    uintptr_t ClientInfo = 0;
    uintptr_t EntityList = 0;
    uintptr_t ViewMatrix = 0;
    uintptr_t Refdef = 0;
    
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

extern GameOffsets g_Offsets;

// ============================================================================
// MATH STRUCTURES
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
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float Length() const { return sqrtf(x*x + y*y + z*z); }
    float Length2D() const { return sqrtf(x*x + y*y); }
};

struct Matrix4x4 { float m[4][4] = {}; };

// ============================================================================
// PLAYER DATA
// ============================================================================
struct PlayerData {
    bool valid = false;
    bool isEnemy = true;
    bool isAlive = true;
    bool isVisible = false;
    bool onScreen = false;
    
    int index = 0;
    char name[64] = {0};
    int health = 100;
    int maxHealth = 100;
    int team = 0;
    int stance = 0;
    
    Vec3 origin;
    Vec3 headPos;
    Vec2 screenPos;
    Vec2 screenHead;
    float screenHeight = 0;
    float screenWidth = 0;
    float distance = 0;
    float yaw = 0;
    
    Vec2 bones[20];
    bool bonesValid = false;
};

// ============================================================================
// SCATTER READ REGISTRY
// ============================================================================
enum class ScatterDataType {
    PLAYER_POSITION, PLAYER_HEALTH, PLAYER_NAME, PLAYER_TEAM,
    PLAYER_YAW, PLAYER_VALID, PLAYER_STANCE, LOCAL_POSITION,
    LOCAL_YAW, VIEW_MATRIX, CUSTOM
};

struct ScatterEntry {
    uintptr_t address;
    void* buffer;
    size_t size;
    ScatterDataType type;
    int playerIndex;
};

class ScatterReadRegistry {
public:
    void RegisterPlayerData(int playerIndex, uintptr_t baseAddr);
    void RegisterLocalPlayer(uintptr_t baseAddr);
    void RegisterViewMatrix(uintptr_t addr);
    void RegisterCustomRead(uintptr_t addr, void* buf, size_t size);
    void ExecuteAll();
    void Clear();
    
    int GetReadCount() const { return (int)m_Entries.size(); }
    int GetTotalBytes() const;
    int GetTransactionCount() const { return m_TransactionCount; }
    
private:
    std::vector<ScatterEntry> m_Entries;
    int m_TransactionCount = 0;
    
    struct PlayerRawData {
        Vec3 position;
        int health, maxHealth, team;
        float yaw;
        uint8_t valid, stance;
        char name[64];
    };
    std::vector<PlayerRawData> m_PlayerBuffers;
    Vec3 m_LocalPosition;
    float m_LocalYaw;
    int m_LocalTeam;
    Matrix4x4 m_ViewMatrix;
};

extern ScatterReadRegistry g_ScatterRegistry;

// ============================================================================
// MAP TEXTURE SUPPORT
// ============================================================================
struct MapInfo {
    float minX = -5000.0f, maxX = 5000.0f;
    float minY = -5000.0f, maxY = 5000.0f;
    int imageWidth = 0, imageHeight = 0;
    float scaleX = 1.0f, scaleY = 1.0f;
    float offsetX = 0.0f, offsetY = 0.0f;
    float rotation = 0.0f;
    char name[64] = "";
    char imagePath[256] = "";
    bool hasTexture = false;
};

class MapTextureManager {
public:
    static bool LoadMapConfig(const char* mapName);
    static bool LoadMapTexture(const char* imagePath);
    static Vec2 GameToMapCoords(const Vec3& gamePos);
    static Vec2 GameToRadarCoords(const Vec3& gamePos, const Vec3& localPos, float localYaw,
                                   float radarCX, float radarCY, float radarSize, float zoom);
    static MapInfo& GetCurrentMap() { return s_CurrentMap; }
    static bool HasMapTexture() { return s_CurrentMap.hasTexture; }
    static void SetMapBounds(const char* mapName, float minX, float maxX, float minY, float maxY);
    static void InitializeMapDatabase();
    
private:
    static MapInfo s_CurrentMap;
    static std::unordered_map<std::string, MapInfo> s_MapDatabase;
};

// ============================================================================
// DIAGNOSTIC SYSTEM
// ============================================================================
class DiagnosticSystem {
public:
    static bool RunAllDiagnostics();
    static DiagnosticResult CheckDeviceHandshake();
    static DiagnosticResult CheckFirmwareIntegrity();
    static DiagnosticResult CheckMemorySpeed();
    static DiagnosticResult CheckProcessAccess();
    static DiagnosticStatus& GetStatus() { return g_DiagStatus; }
    static const char* GetErrorString(DiagnosticResult result);
    static void SelfDestruct(const char* reason);
    static void ShowErrorPopup(const char* title, const char* message);
    
private:
    static bool IsGenericDeviceID(const char* deviceID);
    static bool IsLeakedDeviceID(const char* deviceID);
    static float MeasureReadSpeed(uintptr_t testAddr, size_t size, int iterations);
    static int MeasureLatency(uintptr_t testAddr, int iterations);
};

// ============================================================================
// DMA ENGINE
// ============================================================================
class DMAEngine {
public:
    static bool Initialize();
    static bool InitializeWithConfig(const DMAConfig& config);
    static void Shutdown();
    static bool IsConnected();
    static bool IsOnline();
    static const char* GetStatus();
    static const char* GetDeviceInfo();
    
    template<typename T>
    static T Read(uintptr_t address);
    
    static bool ReadBuffer(uintptr_t address, void* buffer, size_t size);
    static bool ReadString(uintptr_t address, char* buffer, size_t maxLen);
    static void ExecuteScatter(std::vector<ScatterEntry>& entries);
    
    static uintptr_t GetBaseAddress();
    static uintptr_t GetModuleBase(const wchar_t* moduleName);
    static size_t GetModuleSize();
    
private:
    static bool s_Connected;
    static bool s_Online;
    static bool s_SimulationMode;
    static uintptr_t s_BaseAddress;
    static size_t s_ModuleSize;
    static char s_StatusText[64];
    static char s_DeviceInfo[128];
};

// ============================================================================
// PATTERN SCANNER
// ============================================================================
class PatternScanner {
public:
    static uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask);
    static uintptr_t ScanModule(const char* moduleName, const char* pattern, const char* mask);
    static uintptr_t ResolveRelative(uintptr_t instructionAddr, int offset, int instructionSize);
    static bool UpdateAllOffsets();
    static bool IsScanned() { return s_Scanned; }
    static int GetFoundCount() { return s_FoundCount; }
    
private:
    static bool s_Scanned;
    static int s_FoundCount;
};

// ============================================================================
// PLAYER MANAGER
// ============================================================================
class PlayerManager {
public:
    static void Initialize();
    static void Update();
    static void UpdateWithScatterRegistry();
    static std::vector<PlayerData>& GetPlayers();
    static PlayerData& GetLocalPlayer();
    static int GetAliveCount();
    static int GetEnemyCount();
    static std::mutex& GetMutex() { return s_Mutex; }
    
private:
    static void SimulateUpdate();
    static void RealUpdate();
    static std::vector<PlayerData> s_Players;
    static PlayerData s_LocalPlayer;
    static std::mutex s_Mutex;
    static bool s_Initialized;
    static float s_SimTime;
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
bool WorldToScreen(const Vec3& worldPos, Vec2& screenPos, int screenW, int screenH);
bool WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float localYaw, 
                  Vec2& radarPos, float radarCX, float radarCY, float radarScale);
float GetFOVTo(const Vec2& screenCenter, const Vec2& targetScreen);
Vec3 CalcAngle(const Vec3& src, const Vec3& dst);
void SmoothAngle(Vec3& currentAngle, const Vec3& targetAngle, float smoothness);
