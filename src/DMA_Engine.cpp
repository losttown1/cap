// DMA_Engine.cpp - PROJECT ZERO DMA Memory Engine
// Black Ops 6 (BO6) Radar Support with FPGA DMA
// Includes 2D Radar overlay and player tracking

#include "DMA_Engine.hpp"
#include "ZeroUI.hpp"
#include "../include/imgui.h"
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <mutex>

//=============================================================================
// DMA Configuration
//=============================================================================

// Set to 1 for real DMA hardware, 0 for simulation/testing mode
// NOTE: To use real DMA, you need:
//   1. vmmdll.lib in the libs folder (x64 version)
//   2. vmm.dll, leechcore.dll, FTD3XX.dll in the output folder
//   3. FPGA hardware connected and drivers installed
#define DMA_ENABLED 0

#if DMA_ENABLED
    // Real DMA mode - requires VMMDLL library
    #include "../include/vmmdll.h"
    #pragma comment(lib, "../libs/vmmdll.lib")
    static VMM_HANDLE g_hVMM = nullptr;
#else
    // Simulation mode - no hardware required
    typedef void* VMM_HANDLE;
    static VMM_HANDLE g_hVMM = nullptr;
    
    // Stub functions for simulation
    #define VMMDLL_Initialize(argc, argv) ((VMM_HANDLE)1)
    #define VMMDLL_Close(h) ((void)0)
    #define VMMDLL_MemRead(h, pid, addr, buf, size) (true)
    #define VMMDLL_MemWrite(h, pid, addr, buf, size) (true)
    #define VMMDLL_ProcessGetModuleBase(h, pid, name) ((ULONG64)0x140000000)
    #define VMMDLL_PidGetFromName(h, name, pid) (*(pid) = 12345, true)
#endif

//=============================================================================
// Global State
//=============================================================================

static DWORD g_dwTargetPID = 0;
static bool g_bConnected = false;
static ULONG64 g_ModuleBase = 0;
static std::vector<PlayerData> g_Players;
static std::mutex g_PlayersMutex;

// Local player cache
static Vec3 g_LocalPosition;
static float g_LocalYaw = 0.0f;
static int g_LocalTeam = 0;

// Radar settings
static RadarData g_Radar = {
    Vec2(150.0f, 150.0f),  // Position (top-left)
    200.0f,                 // Size
    1.0f,                   // Zoom
    true,                   // Show enemies
    false                   // Show teammates
};

//=============================================================================
// DMA Initialization
//=============================================================================

bool InitializeZeroDMA() 
{
    if (g_bConnected) {
        std::cout << "[DMA Engine] Already initialized" << std::endl;
        return true;
    }

    std::cout << "[DMA Engine] Initializing for Black Ops 6..." << std::endl;
    std::cout << "[DMA Engine] Target Process: " << TARGET_PROCESS_NAME << std::endl;

#if DMA_ENABLED
    // Real DMA mode - connect to FPGA hardware
    std::cout << "[DMA Engine] Mode: HARDWARE DMA" << std::endl;
    std::cout << "[DMA Engine] Connecting to FPGA device..." << std::endl;
    
    // Try to initialize VMMDLL with FPGA
    LPCSTR args[] = { 
        "", 
        "-device", "fpga",
        "-v"
    };
    
    g_hVMM = VMMDLL_Initialize(4, (LPSTR*)args);
    
    if (!g_hVMM) {
        std::cerr << "[DMA Engine] Failed to initialize VMMDLL" << std::endl;
        std::cerr << "[DMA Engine] Possible causes:" << std::endl;
        std::cerr << "[DMA Engine]   - FPGA device not connected" << std::endl;
        std::cerr << "[DMA Engine]   - Driver not installed (FTD3XX)" << std::endl;
        std::cerr << "[DMA Engine]   - Device busy - close other DMA tools" << std::endl;
        std::cerr << "[DMA Engine]   - Missing DLLs (vmm.dll, leechcore.dll)" << std::endl;
        std::cerr << "[DMA Engine] Falling back to simulation mode..." << std::endl;
        goto simulation_mode;
    }

    std::cout << "[DMA Engine] VMMDLL initialized successfully" << std::endl;

    // Find the target process (cod.exe)
    {
        DWORD dwPID = 0;
        if (VMMDLL_PidGetFromName(g_hVMM, (LPSTR)TARGET_PROCESS_NAME, &dwPID)) {
            g_dwTargetPID = dwPID;
            std::cout << "[DMA Engine] Found " << TARGET_PROCESS_NAME << " (PID: " << dwPID << ")" << std::endl;
        } else {
            std::cerr << "[DMA Engine] Process not found: " << TARGET_PROCESS_NAME << std::endl;
            std::cerr << "[DMA Engine] Make sure Black Ops 6 is running" << std::endl;
            std::cerr << "[DMA Engine] Continuing anyway for overlay testing..." << std::endl;
            g_dwTargetPID = 0;
        }
    }

    // Get module base if process was found
    if (g_dwTargetPID > 0) {
        g_ModuleBase = VMMDLL_ProcessGetModuleBase(g_hVMM, g_dwTargetPID, (LPSTR)TARGET_PROCESS_NAME);
        if (g_ModuleBase) {
            std::cout << "[DMA Engine] Module base: 0x" << std::hex << g_ModuleBase << std::dec << std::endl;
        } else {
            std::cout << "[DMA Engine] Warning: Could not get module base" << std::endl;
        }
    }

    g_bConnected = true;
    std::cout << "[DMA Engine] DMA Connection established!" << std::endl;
    return true;
    
simulation_mode:
#endif
    // Simulation mode - no hardware required
    std::cout << "[DMA Engine] Mode: SIMULATION (No Hardware)" << std::endl;
    std::cout << "[DMA Engine] Using fake player data for testing overlay" << std::endl;
    
    g_hVMM = (VMM_HANDLE)1;  // Fake handle
    g_bConnected = true;
    g_dwTargetPID = 12345;
    g_ModuleBase = 0x140000000;
    
    // Generate fake players for testing the radar
    g_Players.clear();
    srand((unsigned int)time(nullptr));
    
    for (int i = 0; i < 12; i++) {
        PlayerData player;
        player.valid = true;
        player.isEnemy = (i < 6);  // First 6 are enemies
        player.health = 100 - (i * 5);
        player.team = player.isEnemy ? 2 : 1;
        
        // Random positions around the local player
        float angle = (float)i * (360.0f / 12.0f) * 3.14159f / 180.0f;
        float dist = 200.0f + (float)(rand() % 400);
        player.position = Vec3(
            cosf(angle) * dist,
            sinf(angle) * dist,
            0.0f
        );
        player.yaw = (float)(rand() % 360);
        player.distance = dist;
        snprintf(player.name, sizeof(player.name), "%s%d", player.isEnemy ? "Enemy" : "Team", i + 1);
        g_Players.push_back(player);
    }
    
    g_LocalPosition = Vec3(0, 0, 0);
    g_LocalYaw = 0.0f;
    g_LocalTeam = 1;
    
    std::cout << "[DMA Engine] Simulation initialized with " << g_Players.size() << " fake players" << std::endl;
    
    return true;
}

void ShutdownZeroDMA() 
{
#if DMA_ENABLED
    if (g_hVMM) {
        VMMDLL_Close(g_hVMM);
        g_hVMM = nullptr;
        std::cout << "[DMA Engine] VMMDLL closed" << std::endl;
    }
#endif
    
    g_bConnected = false;
    g_dwTargetPID = 0;
    g_ModuleBase = 0;
    g_Players.clear();
    
    std::cout << "[DMA Engine] Shutdown complete" << std::endl;
}

bool IsConnected() 
{
    return g_bConnected;
}

//=============================================================================
// Memory Operations
//=============================================================================

bool ReadMemory(ULONG64 address, void* buffer, SIZE_T size) 
{
    if (!g_bConnected || !buffer || size == 0) {
        return false;
    }

#if DMA_ENABLED
    if (!g_hVMM) return false;
    return VMMDLL_MemRead(g_hVMM, g_dwTargetPID, address, 
                          reinterpret_cast<PBYTE>(buffer), 
                          static_cast<DWORD>(size));
#else
    memset(buffer, 0, size);
    return true;
#endif
}

bool WriteMemory(ULONG64 address, void* buffer, SIZE_T size) 
{
    if (!g_bConnected || !buffer || size == 0) {
        return false;
    }

#if DMA_ENABLED
    if (!g_hVMM) return false;
    return VMMDLL_MemWrite(g_hVMM, g_dwTargetPID, address,
                           reinterpret_cast<PBYTE>(buffer),
                           static_cast<DWORD>(size));
#else
    return true;
#endif
}

ULONG64 GetModuleBase(const char* moduleName) 
{
    if (!g_bConnected || !moduleName) {
        return 0;
    }

#if DMA_ENABLED
    if (!g_hVMM) return 0;
    return VMMDLL_ProcessGetModuleBase(g_hVMM, g_dwTargetPID, 
                                       const_cast<LPSTR>(moduleName));
#else
    return g_ModuleBase;
#endif
}

//=============================================================================
// BO6 Player List (Requires valid offsets)
//=============================================================================

bool UpdatePlayerList() 
{
    if (!g_bConnected) {
        return false;
    }

#if DMA_ENABLED
    // This is where you would read the actual player list from BO6
    // You need to find the correct offsets for the current game version
    
    /*
    // Example implementation (offsets need to be updated):
    ULONG64 clientInfo = Read<ULONG64>(g_ModuleBase + BO6Offsets::ClientInfo);
    if (!clientInfo) return false;
    
    ULONG64 clientBase = Read<ULONG64>(clientInfo + BO6Offsets::ClientInfoBase);
    if (!clientBase) return false;
    
    std::vector<PlayerData> newPlayers;
    
    for (int i = 0; i < BO6Offsets::MaxPlayers; i++) {
        ULONG64 entityAddr = clientBase + (i * BO6Offsets::EntitySize);
        
        PlayerData player;
        player.valid = Read<bool>(entityAddr + BO6Offsets::PlayerValid);
        if (!player.valid) continue;
        
        player.health = Read<int>(entityAddr + BO6Offsets::PlayerHealth);
        player.team = Read<int>(entityAddr + BO6Offsets::PlayerTeam);
        player.position = Read<Vec3>(entityAddr + BO6Offsets::PlayerPos);
        player.yaw = Read<float>(entityAddr + BO6Offsets::PlayerYaw);
        
        // Read name
        ReadMemory(entityAddr + BO6Offsets::PlayerName, player.name, sizeof(player.name) - 1);
        
        // Calculate if enemy
        player.isEnemy = (player.team != g_LocalTeam);
        
        // Calculate distance
        Vec3 diff = player.position - g_LocalPosition;
        player.distance = diff.Length();
        
        newPlayers.push_back(player);
    }
    
    std::lock_guard<std::mutex> lock(g_PlayersMutex);
    g_Players = newPlayers;
    */
    
#else
    // Simulation mode - update fake player positions
    static float angle = 0.0f;
    angle += 0.01f;
    
    for (size_t i = 0; i < g_Players.size(); i++) {
        // Make players move in circles for testing
        float radius = 200.0f + (i * 50.0f);
        float playerAngle = angle + (i * 0.5f);
        g_Players[i].position.x = cosf(playerAngle) * radius;
        g_Players[i].position.y = sinf(playerAngle) * radius;
        g_Players[i].distance = g_Players[i].position.Length2D();
    }
#endif

    return true;
}

const std::vector<PlayerData>& GetPlayerList() 
{
    return g_Players;
}

Vec3 GetLocalPlayerPosition() 
{
    return g_LocalPosition;
}

float GetLocalPlayerYaw() 
{
    return g_LocalYaw;
}

int GetLocalPlayerTeam() 
{
    return g_LocalTeam;
}

//=============================================================================
// Radar Functions
//=============================================================================

Vec2 WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float localYaw,
                   const Vec2& radarCenter, float radarSize, float zoom)
{
    // Calculate relative position
    float dx = worldPos.x - localPos.x;
    float dy = worldPos.y - localPos.y;
    
    // Rotate based on local player yaw
    float yawRad = localYaw * 3.14159265f / 180.0f;
    float cosYaw = cosf(yawRad);
    float sinYaw = sinf(yawRad);
    
    float rotatedX = dx * cosYaw - dy * sinYaw;
    float rotatedY = dx * sinYaw + dy * cosYaw;
    
    // Scale to radar size
    float scale = (radarSize / 2.0f) / (500.0f / zoom); // 500 units = half radar at zoom 1
    
    // Clamp to radar bounds
    float maxDist = radarSize / 2.0f - 5.0f;
    float dist = sqrtf(rotatedX * rotatedX + rotatedY * rotatedY) * scale;
    
    if (dist > maxDist) {
        float factor = maxDist / dist;
        rotatedX *= factor;
        rotatedY *= factor;
    } else {
        rotatedX *= scale;
        rotatedY *= scale;
    }
    
    return Vec2(
        radarCenter.x + rotatedX,
        radarCenter.y - rotatedY  // Y is inverted for screen coords
    );
}

void UpdateRadar() 
{
    if (!g_bConnected) return;
    
    // Update player positions
    UpdatePlayerList();
}

void RenderRadarOverlay() 
{
    // Check if ESP is enabled
    if (!esp_enabled || !g_bConnected) {
        return;
    }
    
    // Update radar data
    UpdateRadar();
    
    ImGuiIO& io = ImGui::GetIO();
    
    // Radar position and size
    Vec2 radarPos(20.0f, 20.0f);
    float radarSize = 200.0f;
    Vec2 radarCenter(radarPos.x + radarSize / 2.0f, radarPos.y + radarSize / 2.0f);
    
    // Get foreground draw list for overlay rendering
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;
    
    // Draw radar background
    ImU32 bgColor = IM_COL32(0, 0, 0, 180);
    ImU32 borderColor = IM_COL32(0, 255, 0, 255);
    
    drawList->AddRectFilled(
        ImVec2(radarPos.x, radarPos.y),
        ImVec2(radarPos.x + radarSize, radarPos.y + radarSize),
        bgColor,
        5.0f  // Rounded corners
    );
    
    drawList->AddRect(
        ImVec2(radarPos.x, radarPos.y),
        ImVec2(radarPos.x + radarSize, radarPos.y + radarSize),
        borderColor,
        5.0f,
        0,
        2.0f  // Border thickness
    );
    
    // Draw crosshair
    ImU32 crossColor = IM_COL32(100, 100, 100, 200);
    drawList->AddLine(
        ImVec2(radarCenter.x, radarPos.y + 5),
        ImVec2(radarCenter.x, radarPos.y + radarSize - 5),
        crossColor
    );
    drawList->AddLine(
        ImVec2(radarPos.x + 5, radarCenter.y),
        ImVec2(radarPos.x + radarSize - 5, radarCenter.y),
        crossColor
    );
    
    // Draw local player (center)
    ImU32 localColor = IM_COL32(0, 255, 0, 255);
    drawList->AddCircleFilled(
        ImVec2(radarCenter.x, radarCenter.y),
        5.0f,
        localColor
    );
    
    // Draw direction indicator
    float yawRad = g_LocalYaw * 3.14159265f / 180.0f;
    float dirLen = 15.0f;
    drawList->AddLine(
        ImVec2(radarCenter.x, radarCenter.y),
        ImVec2(radarCenter.x + sinf(yawRad) * dirLen, radarCenter.y - cosf(yawRad) * dirLen),
        localColor,
        2.0f
    );
    
    // Draw players
    std::lock_guard<std::mutex> lock(g_PlayersMutex);
    
    for (const auto& player : g_Players) {
        if (!player.valid) continue;
        if (player.isEnemy && !g_Radar.showEnemies) continue;
        if (!player.isEnemy && !g_Radar.showTeammates) continue;
        
        Vec2 screenPos = WorldToRadar(
            player.position, 
            g_LocalPosition, 
            g_LocalYaw,
            radarCenter, 
            radarSize, 
            g_Radar.zoom
        );
        
        // Choose color based on team
        ImU32 playerColor;
        if (player.isEnemy) {
            // Enemy - red
            playerColor = IM_COL32(255, 0, 0, 255);
        } else {
            // Teammate - blue
            playerColor = IM_COL32(0, 100, 255, 255);
        }
        
        // Draw player dot
        drawList->AddCircleFilled(
            ImVec2(screenPos.x, screenPos.y),
            4.0f,
            playerColor
        );
        
        // Draw player direction
        float playerYawRad = player.yaw * 3.14159265f / 180.0f;
        float pDirLen = 8.0f;
        drawList->AddLine(
            ImVec2(screenPos.x, screenPos.y),
            ImVec2(screenPos.x + sinf(playerYawRad) * pDirLen, 
                   screenPos.y - cosf(playerYawRad) * pDirLen),
            playerColor,
            1.5f
        );
    }
    
    // Draw radar title
    drawList->AddText(
        ImVec2(radarPos.x + 5, radarPos.y + radarSize + 5),
        IM_COL32(255, 255, 255, 200),
        "PROJECT ZERO - BO6 RADAR"
    );
    
    // Draw player count
    char countText[32];
    snprintf(countText, sizeof(countText), "Players: %zu", g_Players.size());
    drawList->AddText(
        ImVec2(radarPos.x + 5, radarPos.y + radarSize + 20),
        IM_COL32(200, 200, 200, 200),
        countText
    );
}
