// DMA_Engine.hpp - DMA Core Definitions
#pragma once

#include <cstdint>
#include <vector>
#include <string>

// ============================================================================
// CONFIGURATION
// ============================================================================
#define DMA_ENABLED 0  // Set to 1 when vmmdll.lib is available
#define TARGET_PROCESS_NAME "cod.exe"

// ============================================================================
// GAME OFFSETS (BO6 - Need real patterns)
// ============================================================================
namespace BO6Offsets {
    // These are placeholder values - real offsets need pattern scanning
    constexpr uintptr_t ClientInfo       = 0x17AA8E98;
    constexpr uintptr_t ClientInfoBase   = 0x99240;
    constexpr uintptr_t EntityList       = 0x16D5B8D8;
    constexpr uintptr_t ViewMatrix       = 0x171B46A0;
    constexpr uintptr_t RefDef           = 0x17210718;
    
    // Entity structure offsets
    constexpr uintptr_t EntityPos        = 0x138;
    constexpr uintptr_t EntityHealth     = 0x1C8;
    constexpr uintptr_t EntityTeam       = 0x1CC;
    constexpr uintptr_t EntityName       = 0x200;
    constexpr uintptr_t EntityYaw        = 0x148;
    
    // Bone offsets
    constexpr uintptr_t BoneBase         = 0x27F20;
    constexpr uintptr_t BoneIndex        = 0x37F30;
}

// ============================================================================
// STRUCTURES
// ============================================================================
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    float Length() const { return sqrtf(x*x + y*y + z*z); }
};

struct PlayerData {
    bool valid = false;
    bool isEnemy = true;
    bool isAlive = true;
    bool isVisible = false;
    bool onScreen = false;
    
    char name[32] = {0};
    int health = 100;
    int maxHealth = 100;
    int team = 0;
    
    Vec3 worldPos;
    Vec3 headPos;
    Vec2 screenPos;
    float screenHeight = 0;
    float distance = 0;
    float yaw = 0;
};

struct BonePos {
    Vec2 head, neck;
    Vec2 shoulderL, shoulderR;
    Vec2 elbowL, elbowR;
    Vec2 handL, handR;
    Vec2 spine, pelvis;
    Vec2 kneeL, kneeR;
    Vec2 footL, footR;
};

// ============================================================================
// SCATTER READ
// ============================================================================
struct ScatterReadRequest {
    uintptr_t address;
    void* buffer;
    size_t size;
};

class ScatterReader {
public:
    std::vector<ScatterReadRequest> requests;
    
    void AddRead(uintptr_t addr, void* buf, size_t size);
    void Execute();
    void Clear();
};

// ============================================================================
// DMA ENGINE
// ============================================================================
class DMAEngine {
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsConnected();
    
    // Single reads
    template<typename T>
    static T Read(uintptr_t address);
    
    static bool ReadBuffer(uintptr_t address, void* buffer, size_t size);
    
    // Scatter reads (batch multiple reads)
    static ScatterReader CreateScatter();
    
    // Process info
    static uintptr_t GetBaseAddress();
    static uint32_t GetProcessId();
    
private:
    static bool s_Connected;
    static uintptr_t s_BaseAddress;
    static uint32_t s_ProcessId;
};

// ============================================================================
// PATTERN SCANNER
// ============================================================================
class PatternScanner {
public:
    static uintptr_t Find(const char* module, const char* pattern, const char* mask);
    static bool UpdateAllOffsets();
    
    // Pattern definitions
    struct Pattern {
        const char* name;
        const char* pattern;
        const char* mask;
        int offset;
    };
};

// ============================================================================
// PLAYER MANAGER
// ============================================================================
class PlayerManager {
public:
    static void Update();
    static std::vector<PlayerData>& GetPlayers();
    static PlayerData& GetLocalPlayer();
    static int GetPlayerCount();
    
private:
    static std::vector<PlayerData> s_Players;
    static PlayerData s_LocalPlayer;
};
