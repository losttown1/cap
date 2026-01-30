// DMA_Engine.hpp - Professional DMA Engine with Pattern Scanner & Scatter Reads
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <mutex>

// ============================================================================
// DMA CONFIGURATION - SET TO 1 FOR REAL HARDWARE
// ============================================================================
#define DMA_ENABLED 1

// Target process
#define TARGET_PROCESS_NAME "cod.exe"
#define TARGET_PROCESS_WIDE L"cod.exe"

// ============================================================================
// PATTERN SIGNATURES (AOB - Array of Bytes)
// ============================================================================
namespace Patterns {
    // Player Base Pattern: 48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 8B 01
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
// GAME OFFSETS (Found by Pattern Scanner)
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
    int stance = 0;  // 0=standing, 1=crouching, 2=prone
    
    Vec3 origin;
    Vec3 headPos;
    Vec2 screenPos;
    Vec2 screenHead;
    float screenHeight = 0;
    float screenWidth = 0;
    float distance = 0;
    float yaw = 0;
    
    // Bone positions (screen space)
    Vec2 bones[20];
    bool bonesValid = false;
};

// ============================================================================
// SCATTER READ REQUEST
// ============================================================================
struct ScatterEntry {
    uintptr_t address;
    void* buffer;
    size_t size;
};

class ScatterReader {
public:
    void Add(uintptr_t addr, void* buf, size_t size);
    void Execute();
    void Clear();
    size_t Count() const { return m_Entries.size(); }
    
private:
    std::vector<ScatterEntry> m_Entries;
};

// ============================================================================
// PATTERN SCANNER
// ============================================================================
class PatternScanner {
public:
    // Scan for pattern in module
    static uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask);
    
    // Scan module by name
    static uintptr_t ScanModule(const char* moduleName, const char* pattern, const char* mask);
    
    // Read relative address (for instructions like MOV RAX, [RIP+offset])
    static uintptr_t ResolveRelative(uintptr_t instructionAddr, int offset, int instructionSize);
    
    // Update all game offsets
    static bool UpdateAllOffsets();
    
    // Get scan status
    static bool IsScanned() { return s_Scanned; }
    static int GetFoundCount() { return s_FoundCount; }
    
private:
    static bool s_Scanned;
    static int s_FoundCount;
};

// ============================================================================
// DMA ENGINE
// ============================================================================
class DMAEngine {
public:
    // Initialization
    static bool Initialize();
    static void Shutdown();
    static bool IsConnected();
    static const char* GetStatus();
    
    // Memory operations
    template<typename T>
    static T Read(uintptr_t address);
    
    static bool ReadBuffer(uintptr_t address, void* buffer, size_t size);
    static bool ReadString(uintptr_t address, char* buffer, size_t maxLen);
    
    // Scatter read
    static ScatterReader CreateScatter();
    static void ExecuteScatter(ScatterReader& scatter);
    
    // Process info
    static uintptr_t GetBaseAddress();
    static uintptr_t GetModuleBase(const wchar_t* moduleName);
    static size_t GetModuleSize(const wchar_t* moduleName);
    
private:
    static bool s_Connected;
    static bool s_SimulationMode;
    static uintptr_t s_BaseAddress;
    static size_t s_ModuleSize;
    static char s_StatusText[64];
};

// ============================================================================
// PLAYER MANAGER
// ============================================================================
class PlayerManager {
public:
    static void Initialize();
    static void Update();
    static void UpdateWithScatter();
    
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
// WORLD TO SCREEN
// ============================================================================
bool WorldToScreen(const Vec3& worldPos, Vec2& screenPos, int screenW, int screenH);
bool WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float localYaw, 
                  Vec2& radarPos, float radarCX, float radarCY, float radarScale);

// ============================================================================
// AIMBOT HELPERS
// ============================================================================
float GetFOVTo(const Vec2& screenCenter, const Vec2& targetScreen);
Vec3 CalcAngle(const Vec3& src, const Vec3& dst);
void SmoothAngle(Vec3& currentAngle, const Vec3& targetAngle, float smoothness);
