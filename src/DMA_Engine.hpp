// DMA_Engine.hpp - Professional DMA Engine
// Features: Scatter Registry, Pattern Scanner, Config-driven Init, Map Textures

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <functional>

// ============================================================================
// DMA CONFIGURATION - LOADED FROM CONFIG FILE
// ============================================================================
#define DMA_ENABLED 1

// ============================================================================
// CONFIG STRUCTURE (Loaded from zero.ini)
// ============================================================================
struct DMAConfig {
    // Device settings (Hardware-ID Masking)
    char deviceType[32] = "fpga";
    char deviceArg[64] = "";
    char deviceAlgo[16] = "0";
    bool useCustomPCIe = false;
    char customPCIeID[32] = "";
    
    // Target process
    char processName[64] = "cod.exe";
    wchar_t processNameW[64] = L"cod.exe";
    
    // Performance
    int scatterBatchSize = 128;
    int updateRateHz = 120;
    bool useScatterRegistry = true;
    
    // Map settings
    char mapImagePath[256] = "";
    float mapScaleX = 1.0f;
    float mapScaleY = 1.0f;
    float mapOffsetX = 0.0f;
    float mapOffsetY = 0.0f;
    float mapRotation = 0.0f;
    
    // Debug
    bool debugMode = false;
    bool logReads = false;
};

extern DMAConfig g_Config;

// Config file functions
bool LoadConfig(const char* filename = "zero.ini");
bool SaveConfig(const char* filename = "zero.ini");
void CreateDefaultConfig(const char* filename = "zero.ini");

// ============================================================================
// PATTERN SIGNATURES (AOB - Array of Bytes)
// ============================================================================
namespace Patterns {
    // Player Base Pattern
    constexpr const char* PlayerBase = "\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x88\x00\x00\x00\x00\x48\x8B\x01";
    constexpr const char* PlayerBaseMask = "xxx????xxx????xxx";
    
    // Client Info Pattern
    constexpr const char* ClientInfo = "\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB\x74\x00\x48\x8B\x03";
    constexpr const char* ClientInfoMask = "xxx????xxxx?xxx";
    
    // Entity List Pattern
    constexpr const char* EntityList = "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x85\xC0\x74";
    constexpr const char* EntityListMask = "xxx????x????xxxx";
    
    // View Matrix Pattern
    constexpr const char* ViewMatrix = "\x48\x8B\x15\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\xE8";
    constexpr const char* ViewMatrixMask = "xxx????xxxx?x";
    
    // Refdef Pattern
    constexpr const char* Refdef = "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x84\xC0\x0F\x84";
    constexpr const char* RefdefMask = "xxx????x????xxxx";
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
    
    // Entity structure offsets
    static constexpr uintptr_t EntitySize = 0x568;
    static constexpr uintptr_t EntityPos = 0x138;
    static constexpr uintptr_t EntityHealth = 0x1C8;
    static constexpr uintptr_t EntityMaxHealth = 0x1CC;
    static constexpr uintptr_t EntityTeam = 0x1D0;
    static constexpr uintptr_t EntityYaw = 0x148;
    static constexpr uintptr_t EntityName = 0x200;
    static constexpr uintptr_t EntityValid = 0x100;
    static constexpr uintptr_t EntityStance = 0x1E0;
    
    // Bone offsets
    static constexpr uintptr_t BoneBase = 0x27F20;
    static constexpr uintptr_t BoneIndex = 0x150;
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
    Vec3 Normalize() const { float l = Length(); return l > 0 ? Vec3(x/l, y/l, z/l) : Vec3(); }
};

struct Matrix4x4 {
    float m[4][4] = {};
};

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
// SCATTER READ REGISTRY (Professional Performance)
// ============================================================================
enum class ScatterDataType {
    PLAYER_POSITION,
    PLAYER_HEALTH,
    PLAYER_NAME,
    PLAYER_TEAM,
    PLAYER_YAW,
    PLAYER_VALID,
    PLAYER_STANCE,
    LOCAL_POSITION,
    LOCAL_YAW,
    VIEW_MATRIX,
    CUSTOM
};

struct ScatterEntry {
    uintptr_t address;
    void* buffer;
    size_t size;
    ScatterDataType type;
    int playerIndex;  // -1 for non-player data
};

class ScatterReadRegistry {
public:
    // Register data to read every frame
    void RegisterPlayerData(int playerIndex, uintptr_t baseAddr);
    void RegisterLocalPlayer(uintptr_t baseAddr);
    void RegisterViewMatrix(uintptr_t addr);
    void RegisterCustomRead(uintptr_t addr, void* buf, size_t size);
    
    // Execute all registered reads in one transaction
    void ExecuteAll();
    
    // Clear registry
    void Clear();
    
    // Get statistics
    int GetReadCount() const { return (int)m_Entries.size(); }
    int GetTotalBytes() const;
    int GetTransactionCount() const { return m_TransactionCount; }
    
private:
    std::vector<ScatterEntry> m_Entries;
    int m_TransactionCount = 0;
    
    // Internal buffers for player data
    struct PlayerRawData {
        Vec3 position;
        int health;
        int maxHealth;
        int team;
        float yaw;
        uint8_t valid;
        uint8_t stance;
        char name[64];
    };
    std::vector<PlayerRawData> m_PlayerBuffers;
    
    // Local player buffer
    Vec3 m_LocalPosition;
    float m_LocalYaw;
    int m_LocalTeam;
    
    // View matrix buffer
    Matrix4x4 m_ViewMatrix;
};

extern ScatterReadRegistry g_ScatterRegistry;

// ============================================================================
// MAP TEXTURE SUPPORT
// ============================================================================
struct MapInfo {
    // Map bounds in game coordinates
    float minX = -5000.0f;
    float maxX = 5000.0f;
    float minY = -5000.0f;
    float maxY = 5000.0f;
    
    // Image dimensions
    int imageWidth = 0;
    int imageHeight = 0;
    
    // Calibration
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float rotation = 0.0f;  // Degrees
    
    // Map name
    char name[64] = "";
    char imagePath[256] = "";
    bool hasTexture = false;
};

class MapTextureManager {
public:
    // Load map configuration
    static bool LoadMapConfig(const char* mapName);
    static bool LoadMapTexture(const char* imagePath);
    
    // Coordinate conversion
    static Vec2 GameToMapCoords(const Vec3& gamePos);
    static Vec2 GameToRadarCoords(const Vec3& gamePos, const Vec3& localPos, float localYaw,
                                   float radarCX, float radarCY, float radarSize, float zoom);
    
    // Map info
    static MapInfo& GetCurrentMap() { return s_CurrentMap; }
    static bool HasMapTexture() { return s_CurrentMap.hasTexture; }
    
    // Predefined map configs (BO6 maps)
    static void SetMapBounds(const char* mapName, float minX, float maxX, float minY, float maxY);
    static void InitializeMapDatabase();
    
private:
    static MapInfo s_CurrentMap;
    static std::unordered_map<std::string, MapInfo> s_MapDatabase;
};

// ============================================================================
// DMA ENGINE
// ============================================================================
class DMAEngine {
public:
    // Initialization with config
    static bool Initialize();
    static bool InitializeWithConfig(const DMAConfig& config);
    static void Shutdown();
    static bool IsConnected();
    static const char* GetStatus();
    static const char* GetDeviceInfo();
    
    // Memory operations
    template<typename T>
    static T Read(uintptr_t address);
    
    static bool ReadBuffer(uintptr_t address, void* buffer, size_t size);
    static bool ReadString(uintptr_t address, char* buffer, size_t maxLen);
    
    // Scatter operations
    static void ExecuteScatter(std::vector<ScatterEntry>& entries);
    
    // Process info
    static uintptr_t GetBaseAddress();
    static uintptr_t GetModuleBase(const wchar_t* moduleName);
    static size_t GetModuleSize();
    
private:
    static bool s_Connected;
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
