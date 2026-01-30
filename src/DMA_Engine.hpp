#ifndef DMA_ENGINE_HPP
#define DMA_ENGINE_HPP

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <cstring>

// Target process
constexpr const char* TARGET_PROCESS_NAME = "cod.exe";
constexpr DWORD TARGET_PID = 0;
constexpr ULONG64 TARGET_HANDLE = 0x021EE040;

// Offsets namespace for BO6
namespace BO6Offsets
{
    constexpr ULONG64 ClientInfo = 0x0;
    constexpr ULONG64 ClientInfoBase = 0x0;
    constexpr ULONG64 EntityList = 0x0;
    constexpr ULONG64 LocalPlayer = 0x0;
    constexpr ULONG64 ViewMatrix = 0x0;
    constexpr int MaxPlayers = 155;
}

// ============================================================================
// Vector structures
// ============================================================================
struct Vec2
{
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vec3
{
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    
    Vec3 operator-(const Vec3& o) const { return Vec3(x-o.x, y-o.y, z-o.z); }
    Vec3 operator+(const Vec3& o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
    Vec3 operator*(float f) const { return Vec3(x*f, y*f, z*f); }
    float Length() const { return sqrtf(x*x + y*y + z*z); }
    float Length2D() const { return sqrtf(x*x + y*y); }
};

// ============================================================================
// Player data structure
// ============================================================================
struct PlayerData
{
    bool valid;
    bool isEnemy;
    bool isVisible;
    bool onScreen;
    int health;
    int maxHealth;
    int team;
    Vec3 position;
    Vec3 headPosition;
    float yaw;
    float distance;
    char name[32];
    
    // Screen positions (for ESP)
    Vec2 screenPos;
    float screenHeight;
    
    PlayerData() : valid(false), isEnemy(false), isVisible(false), onScreen(false),
                   health(100), maxHealth(100), team(0), yaw(0), distance(0), screenHeight(0)
    {
        memset(name, 0, sizeof(name));
    }
};

// ============================================================================
// Radar data
// ============================================================================
struct RadarData
{
    Vec2 position;
    float size;
    float zoom;
    bool showEnemies;
    bool showTeammates;
    std::vector<PlayerData> players;
};

// ============================================================================
// Core functions
// ============================================================================
bool InitializeZeroDMA();
void ShutdownZeroDMA();
bool IsConnected();

// Memory
bool ReadMemory(ULONG64 address, void* buffer, SIZE_T size);
bool WriteMemory(ULONG64 address, void* buffer, SIZE_T size);
ULONG64 GetModuleBase(const char* moduleName);

// Player data
bool UpdatePlayerList();
const std::vector<PlayerData>& GetPlayerList();
Vec3 GetLocalPlayerPosition();
float GetLocalPlayerYaw();
int GetLocalPlayerTeam();

// Rendering
void RenderRadarOverlay();
void RenderESP();

// Templates
template<typename T>
T Read(ULONG64 address) {
    T value{};
    ReadMemory(address, &value, sizeof(T));
    return value;
}

template<typename T>
bool Write(ULONG64 address, const T& value) {
    return WriteMemory(address, (void*)&value, sizeof(T));
}

#endif // DMA_ENGINE_HPP
