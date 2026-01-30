#ifndef DMA_ENGINE_HPP
#define DMA_ENGINE_HPP

#include <Windows.h>
#include <cstdint>
#include <vector>

//=============================================================================
// DMA Configuration - Black Ops 6 (BO6)
//=============================================================================

// Target process
constexpr const char* TARGET_PROCESS_NAME = "cod.exe";
constexpr DWORD TARGET_PID = 0;  // Will be found dynamically
constexpr ULONG64 TARGET_HANDLE = 0x021EE040;

// BO6 Offsets (Update these based on current game version)
namespace BO6Offsets
{
    // Base addresses
    constexpr ULONG64 ClientInfo         = 0x0;        // Needs updating
    constexpr ULONG64 ClientInfoBase     = 0x0;        // Needs updating
    constexpr ULONG64 EntityList         = 0x0;        // Needs updating
    constexpr ULONG64 LocalPlayer        = 0x0;        // Needs updating
    constexpr ULONG64 ViewMatrix         = 0x0;        // Needs updating
    
    // Player structure offsets
    constexpr ULONG64 PlayerHealth       = 0x0;
    constexpr ULONG64 PlayerTeam         = 0x0;
    constexpr ULONG64 PlayerPos          = 0x0;        // Vec3 position
    constexpr ULONG64 PlayerYaw          = 0x0;
    constexpr ULONG64 PlayerName         = 0x0;
    constexpr ULONG64 PlayerValid        = 0x0;
    
    // Entity list
    constexpr int MaxPlayers             = 155;        // BO6 Warzone max
    constexpr ULONG64 EntitySize         = 0x0;       // Size of each entity
}

//=============================================================================
// Data Structures
//=============================================================================

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
    
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator*(float f) const { return Vec3(x * f, y * f, z * f); }
    float Length() const { return sqrtf(x*x + y*y + z*z); }
    float Length2D() const { return sqrtf(x*x + y*y); }
};

struct PlayerData
{
    bool valid;
    bool isEnemy;
    bool isVisible;
    int health;
    int team;
    Vec3 position;
    Vec3 headPosition;
    float yaw;
    char name[32];
    float distance;
    
    PlayerData() : valid(false), isEnemy(false), isVisible(false), 
                   health(0), team(0), yaw(0.0f), distance(0.0f) 
    {
        memset(name, 0, sizeof(name));
    }
};

struct RadarData
{
    Vec2 position;      // Radar position on screen
    float size;         // Radar size
    float zoom;         // Radar zoom level
    bool showEnemies;
    bool showTeammates;
    std::vector<PlayerData> players;
};

//=============================================================================
// DMA Engine Functions
//=============================================================================

// Core functions
bool InitializeZeroDMA();
void ShutdownZeroDMA();
bool IsConnected();

// Memory functions
bool ReadMemory(ULONG64 address, void* buffer, SIZE_T size);
bool WriteMemory(ULONG64 address, void* buffer, SIZE_T size);
ULONG64 GetModuleBase(const char* moduleName);

// BO6-specific functions
bool UpdatePlayerList();
const std::vector<PlayerData>& GetPlayerList();
Vec3 GetLocalPlayerPosition();
float GetLocalPlayerYaw();
int GetLocalPlayerTeam();

// Radar functions
void UpdateRadar();
void RenderRadarOverlay();

// Utility
Vec2 WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float localYaw, 
                   const Vec2& radarCenter, float radarSize, float zoom);

// Template read functions
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
