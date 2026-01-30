// DMA_Engine.hpp - Final Hardware & Cloud Integration v3.3
#pragma once

// ============================================================================
// CONFIGURATION FLAGS
// ============================================================================
#define DMA_ENABLED 1          // Enable DMA hardware
#define USE_FTD3XX 0           // Not using FTD3XX for now
#define USE_LIBCURL 0          // Using WinHTTP instead

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>

// ============================================================================
// MATH TYPES
// ============================================================================
struct Vec2 { float x = 0, y = 0; Vec2() = default; Vec2(float _x, float _y) : x(_x), y(_y) {} };
struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    float Length() const { return sqrtf(x*x + y*y + z*z); }
};

struct Matrix4x4 { float m[4][4] = {}; };

// ============================================================================
// CONTROLLER TYPES
// ============================================================================
enum class ControllerType { NONE = 0, KMBOX_B_PLUS, KMBOX_NET, ARDUINO };
const char* ControllerTypeToString(ControllerType type);

struct ControllerConfig {
    ControllerType type = ControllerType::NONE;
    char comPort[16] = "COM3";
    char ipAddress[32] = "192.168.2.188";
    int port = 16896;
    int baudRate = 115200;
    bool autoDetect = false;
};

// ============================================================================
// SCATTER READ
// ============================================================================
enum class ScatterDataType {
    PLAYER_POSITION, PLAYER_HEALTH, PLAYER_TEAM, PLAYER_NAME,
    PLAYER_BONES, PLAYER_YAW, LOCAL_POSITION, LOCAL_ANGLES,
    LOCAL_YAW, VIEW_MATRIX, CUSTOM
};

struct ScatterEntry {
    uintptr_t address = 0;
    void* buffer = nullptr;
    size_t size = 0;
    ScatterDataType type = ScatterDataType::CUSTOM;
    int playerIndex = -1;
};

struct PlayerScatterBuffer {
    Vec3 position;
    int health = 0;
    int team = 0;
    float yaw = 0;
    char name[32] = {};
};

// ============================================================================
// REMOTE OFFSETS
// ============================================================================
struct RemoteOffsets {
    uintptr_t ClientInfo = 0;
    uintptr_t EntityList = 0;
    uintptr_t ViewMatrix = 0;
    uintptr_t Refdef = 0;
    uintptr_t BoneMatrix = 0;
    uintptr_t WeaponInfo = 0;
    char buildNumber[32] = {};
    char versionString[64] = {};
    bool valid = false;
};

// ============================================================================
// DMA CONFIG
// ============================================================================
struct DMAConfig {
    char devicePath[64] = "fpga";
    char processName[64] = "cod.exe";
    int updateRateHz = 60;
    ControllerType controllerType = ControllerType::KMBOX_B_PLUS;
    bool enableOffsetUpdater = true;
    char offsetURL[512] = "";
};

// ============================================================================
// PLAYER DATA
// ============================================================================
struct PlayerData {
    Vec3 origin;
    Vec3 velocity;
    Vec3 headPos;
    Vec3 feetPos;
    Vec2 screenPos;
    float yaw = 0;
    float distance = 0;
    int health = 0;
    int maxHealth = 100;
    int team = 0;
    int index = 0;
    int stance = 0;
    char name[32] = {};
    bool valid = false;
    bool isEnemy = false;
    bool isAlive = false;
    bool visible = false;
};

// ============================================================================
// STATUS
// ============================================================================
struct InitStatus {
    bool allChecksPassed = false;
    bool dmaConnected = false;
    bool gameFound = false;
    bool offsetsUpdated = false;
};

struct DiagnosticStatus {
    char statusText[64] = "";
    float readSpeedMBs = 0;
    int latencyUs = 0;
    bool passed = false;
};

enum class DiagnosticResult {
    SUCCESS, DEVICE_NOT_FOUND, FIRMWARE_FAIL, MEMORY_SPEED_FAIL,
    PROCESS_ACCESS_FAIL, CONTROLLER_FAIL, NETWORK_FAIL
};

// ============================================================================
// GAME OFFSETS
// ============================================================================
namespace GameOffsets {
    constexpr uintptr_t ClientInfo = 0x16D5B8D8;
    constexpr uintptr_t EntityList = 0x16D5C000;
    constexpr uintptr_t ViewMatrix = 0x16D60000;
    constexpr uintptr_t EntitySize = 0x568;
    constexpr uintptr_t EntityPos = 0x138;
    constexpr uintptr_t EntityYaw = 0x148;
    constexpr uintptr_t EntityHealth = 0x1C8;
    constexpr uintptr_t EntityTeam = 0x1D0;
    constexpr uintptr_t EntityName = 0x200;
    constexpr uintptr_t EntityValid = 0x100;
};

struct GameOffsetsStruct {
    uintptr_t ClientInfo = 0;
    uintptr_t EntityList = 0;
    uintptr_t ViewMatrix = 0;
};

// ============================================================================
// MAP INFO
// ============================================================================
struct MapInfo {
    char name[32] = {};
    char imagePath[256] = {};
    float minX = -4096, maxX = 4096;
    float minY = -4096, maxY = 4096;
    int width = 256, height = 256;
};

// ============================================================================
// CONSOLE COLORS
// ============================================================================
enum ConsoleColor {
    COLOR_DEFAULT = 7,
    COLOR_RED = 12,
    COLOR_GREEN = 10,
    COLOR_YELLOW = 14,
    COLOR_BLUE = 9,
    COLOR_CYAN = 11,
    COLOR_MAGENTA = 13,
    COLOR_WHITE = 15,
    COLOR_GRAY = 8
};

// ============================================================================
// LOGGER
// ============================================================================
class Logger {
public:
    static void* s_Console;
    static void Initialize();
    static void Shutdown();
    static void SetColor(ConsoleColor c);
    static void ResetColor();
    static void Log(const char* m, ConsoleColor c = COLOR_DEFAULT);
    static void LogSuccess(const char* m);
    static void LogError(const char* m);
    static void LogWarning(const char* m);
    static void LogInfo(const char* m);
    static void LogStatus(const char* label, const char* value, bool success);
    static void LogProgress(const char* m, int c, int t);
    static void LogBanner();
    static void LogSection(const char* t);
    static void LogSeparator();
    static void LogTimestamp();
    static void LogSpinner(const char* m, int f);
    static void ClearLine();
};

// ============================================================================
// HARDWARE CONTROLLER
// ============================================================================
class HardwareController {
public:
    static void* s_Handle;
    static void* s_Socket;
    static bool s_Connected;
    static ControllerType s_Type;
    static char s_DeviceName[64];

    static bool Initialize();
    static void Shutdown();
    static bool IsConnected();
    static const char* GetDeviceName();
    static ControllerType GetType();
    static bool MoveMouse(int dx, int dy);
    static bool Click(int button);
    static bool Press(int button);
    static bool Release(int button);
    static bool KeyPress(int k);
    static bool KeyRelease(int k);
    static bool ConnectKMBoxBPlus(const char* comPort, int baudRate);
    static bool ConnectKMBoxNet(const char* ip, int port);
    static bool ConnectArduino(const char* comPort, int baudRate);
    static bool ScanCOMPorts(std::vector<std::string>& ports);
    static bool AutoDetectDevice();
    static bool SerialWrite(const char* d, int len);
    static bool SocketSend(const char* d, int len);
};

// ============================================================================
// OFFSET UPDATER (GitHub Cloud)
// ============================================================================
class OffsetUpdater {
public:
    static RemoteOffsets s_LastOffsets;
    static bool s_Updated;
    static bool s_Synced;
    static char s_OffsetURL[512];
    static char s_LastError[256];
    
    static constexpr const char* DEFAULT_OFFSET_URL = 
        "https://raw.githubusercontent.com/losttown1/cap/refs/heads/cursor/zero-elite-project-setup-11fd/offsets.txt.txt";

    static void SetOffsetURL(const char* url);
    static bool FetchRemoteOffsets(const char* url = nullptr);
    static bool UpdateOffsetsFromServer();
    static bool SyncWithCloud(int maxRetries = 2, int retryDelayMs = 2000);
    static bool HttpGet(const char* url, std::string& response);
    static bool ParseOffsetsJSON(const char* jsonData);
    static bool ParseOffsetsINI(const char* iniData);
    static bool ParseOffsetsTXT(const char* txtData);
    static bool ApplyOffsets(const RemoteOffsets& o);
};

// ============================================================================
// SCATTER REGISTRY
// ============================================================================
class ScatterReadRegistry {
public:
    void RegisterPlayerData(int idx, uintptr_t base);
    void RegisterLocalPlayer(uintptr_t base);
    void RegisterViewMatrix(uintptr_t addr);
    void RegisterCustomRead(uintptr_t addr, void* buf, size_t size);
    void ExecuteAll();
    void Clear();
    int GetTotalBytes() const;
    const std::vector<ScatterEntry>& GetEntries() const { return m_Entries; }
    const Vec3& GetLocalPosition() const { return m_LocalPosition; }
    float GetLocalYaw() const { return m_LocalYaw; }
    const Matrix4x4& GetViewMatrix() const { return m_ViewMatrix; }
    int GetTransactionCount() const { return m_TransactionCount; }
private:
    std::vector<ScatterEntry> m_Entries;
    std::vector<PlayerScatterBuffer> m_PlayerBuffers;
    Vec3 m_LocalPosition;
    float m_LocalYaw = 0;
    Matrix4x4 m_ViewMatrix;
    int m_TransactionCount = 0;
};

// ============================================================================
// MAP TEXTURE MANAGER
// ============================================================================
class MapTextureManager {
public:
    static MapInfo s_CurrentMap;
    static std::unordered_map<std::string, MapInfo> s_MapDatabase;
    static void InitializeMapDatabase();
    static bool LoadMapConfig(const char* name);
    static bool LoadMapTexture(const char* path);
    static Vec2 GameToMapCoords(const Vec3& gamePos);
    static Vec2 GameToRadarCoords(const Vec3& gamePos, const Vec3& localPos, float localYaw,
                                   float centerX, float centerY, float radarSize, float zoom);
    static void SetMapBounds(const char* name, float minX, float minY, float maxX, float maxY);
};

// ============================================================================
// DMA ENGINE
// ============================================================================
class DMAEngine {
public:
    // PUBLIC static members (accessible from ProfessionalInit)
    static bool s_Connected;
    static bool s_Online;
    static bool s_SimulationMode;
    static bool s_UsingFTD3XX;
    static uintptr_t s_BaseAddress;
    static size_t s_ModuleSize;
    static char s_StatusText[64];
    static char s_DeviceInfo[128];
    static char s_DriverMode[32];

    static bool Initialize();
    static bool InitializeWithConfig(const DMAConfig& config);
    static bool InitializeFTD3XX();
    static void Shutdown();
    static bool IsConnected();
    static bool IsOnline();
    static const char* GetStatus();
    static const char* GetDeviceInfo();
    static const char* GetDriverMode();
    template<typename T> static T Read(uintptr_t address);
    static bool ReadBuffer(uintptr_t address, void* buffer, size_t size);
    static bool ReadString(uintptr_t address, char* buffer, size_t maxLen);
    static void ExecuteScatter(std::vector<ScatterEntry>& entries);
    static uintptr_t GetBaseAddress();
    static uintptr_t GetModuleBase(const wchar_t* moduleName);
    static size_t GetModuleSize();
};

// ============================================================================
// PATTERN SCANNER
// ============================================================================
class PatternScanner {
public:
    static bool s_Scanned;
    static int s_FoundCount;
    static uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask);
    static uintptr_t ScanModule(const char* module, const char* pattern, const char* mask);
    static uintptr_t ResolveRelative(uintptr_t address, int offset, int size);
    static bool UpdateAllOffsets();
};

// ============================================================================
// DIAGNOSTIC SYSTEM
// ============================================================================
class DiagnosticSystem {
public:
    static const char* GetErrorString(DiagnosticResult result);
    static bool RunAllDiagnostics();
    static DiagnosticResult CheckDeviceHandshake();
    static DiagnosticResult CheckFirmwareIntegrity();
    static DiagnosticResult CheckMemorySpeed();
    static DiagnosticResult CheckProcessAccess();
    static void SelfDestruct(const char* reason);
    static void ShowErrorPopup(const char* title, const char* msg);
    static bool IsGenericDeviceID(const char* deviceId);
    static bool IsLeakedDeviceID(const char* deviceId);
    static float MeasureReadSpeed(uintptr_t testAddress, size_t testSize, int iterations);
    static int MeasureLatency(uintptr_t testAddress, int samples);
};

// ============================================================================
// PROFESSIONAL INIT
// ============================================================================
class ProfessionalInit {
public:
    static bool RunProfessionalChecks();
    static void SimulateDelay(int ms);
    static bool CheckDMAConnection();
    static bool CheckMemoryMap();
    static void GetWindowsVersion(char* buffer, size_t size);
    static void GetKeyboardState(char* buffer, size_t size);
    static void GetMouseState(char* buffer, size_t size);
};

// ============================================================================
// AIMBOT
// ============================================================================
class Aimbot {
public:
    static bool s_Enabled;
    static int s_CurrentTarget;
    static Vec2 s_LastAimPos;
    static void Initialize();
    static void Update();
    static bool IsEnabled();
    static void SetEnabled(bool enabled);
    static int FindBestTarget(float maxFOV, float maxDist);
    static Vec2 GetTargetScreenPos(int targetIndex);
    static void AimAt(const Vec2& targetPos, float smoothness);
    static void MoveMouse(int deltaX, int deltaY);
};

// ============================================================================
// PLAYER MANAGER
// ============================================================================
class PlayerManager {
public:
    static std::vector<PlayerData> s_Players;
    static PlayerData s_LocalPlayer;
    static std::mutex s_Mutex;
    static bool s_Initialized;
    static float s_SimTime;
    static void Initialize();
    static void Update();
    static void UpdateWithScatterRegistry();
    static void SimulateUpdate();
    static void RealUpdate();
    static std::vector<PlayerData>& GetPlayers();
    static PlayerData& GetLocalPlayer();
    static int GetAliveCount();
    static int GetEnemyCount();
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
bool WorldToScreen(const Vec3& world, Vec2& screen, int screenW, int screenH);
bool WorldToRadar(const Vec3& world, const Vec3& local, float localYaw, Vec2& radar, float centerX, float centerY, float radarSize);
float GetFOVTo(const Vec2& center, const Vec2& target);
Vec3 CalcAngle(const Vec3& src, const Vec3& dst);
void SmoothAngle(Vec3& current, const Vec3& target, float smoothness);

// ============================================================================
// CONFIG
// ============================================================================
bool LoadConfig(const char* filename);
bool SaveConfig(const char* filename);
void CreateDefaultConfig(const char* filename);

void ExecuteScatterReads(std::vector<ScatterEntry>& entries);

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
extern DMAConfig g_Config;
extern GameOffsetsStruct g_Offsets;
extern ScatterReadRegistry g_ScatterRegistry;
extern DiagnosticStatus g_DiagStatus;
extern InitStatus g_InitStatus;
extern ControllerConfig g_ControllerConfig;
