// DMA_Engine.cpp - PROJECT ZERO DMA Memory Engine
// Black Ops 6 (BO6) Radar Support with FPGA DMA
// Includes 2D Radar overlay and player tracking

#include "DMA_Engine.hpp"
#include "ZeroUI.hpp"
#include "../include/imgui.h"
#include <iostream>
#include <cmath>
#include <mutex>

//=============================================================================
// DMA Configuration
//=============================================================================

#define DMA_ENABLED 1  // Set to 1 for real DMA, 0 for simulation

#if DMA_ENABLED
#include <vmmdll.h>
#pragma comment(lib, "vmmdll.lib")
static VMM_HANDLE g_hVMM = nullptr;
#else
typedef void* VMM_HANDLE;
static VMM_HANDLE g_hVMM = nullptr;
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
    // Try to initialize VMMDLL with FPGA
    LPCSTR args[] = { 
        "", 
        "-device", "fpga",
        "-v",
        "-printf"
    };
    
    std::cout << "[DMA Engine] Connecting to FPGA device..." << std::endl;
    
    g_hVMM = VMMDLL_Initialize(5, (LPSTR*)args);
    
    if (!g_hVMM) {
        std::cerr << "[DMA Engine] Failed to initialize VMMDLL" << std::endl;
        std::cerr << "[DMA Engine] Possible causes:" << std::endl;
        std::cerr << "[DMA Engine]   - FPGA device not connected" << std::endl;
        std::cerr << "[DMA Engine]   - Driver not installed" << std::endl;
        std::cerr << "[DMA Engine]   - Device busy (close other DMA tools)" << std::endl;
        std::cerr << "[DMA Engine]   - Missing vmm.dll/leechcore.dll" << std::endl;
        return false;
    }

    std::cout << "[DMA Engine] VMMDLL initialized successfully" << std::endl;

    // Find the target process (cod.exe)
    DWORD dwPID = 0;
    if (VMMDLL_PidGetFromName(g_hVMM, (LPSTR)TARGET_PROCESS_NAME, &dwPID)) {
        g_dwTargetPID = dwPID;
        std::cout << "[DMA Engine] Found " << TARGET_PROCESS_NAME << " (PID: " << dwPID << ")" << std::endl;
    } else {
        std::cerr << "[DMA Engine] Process not found: " << TARGET_PROCESS_NAME << std::endl;
        std::cerr << "[DMA Engine] Make sure Black Ops 6 is running" << std::endl;
        VMMDLL_Close(g_hVMM);
        g_hVMM = nullptr;
        return false;
    }

    // Get module base
    g_ModuleBase = VMMDLL_ProcessGetModuleBase(g_hVMM, g_dwTargetPID, (LPSTR)TARGET_PROCESS_NAME);
    if (g_ModuleBase) {
        std::cout << "[DMA Engine] Module base: 0x" << std::hex << g_ModuleBase << std::dec << std::endl;
    } else {
        std::cout << "[DMA Engine] Warning: Could not get module base" << std::endl;
    }

    g_bConnected = true;
    std::cout << "[DMA Engine] Connection established!" << std::endl;
    
#else
    // Simulation mode
    std::cout << "[DMA Engine] Running in SIMULATION MODE" << std::endl;
    std::cout << "[DMA Engine] No real DMA - using fake data for testing" << std::endl;
    
    g_bConnected = true;
    g_dwTargetPID = 12345;
    g_ModuleBase = 0x140000000;
    
    // Generate some fake players for testing
    g_Players.clear();
    for (int i = 0; i < 10; i++) {
        PlayerData player;
        player.valid = true;
        player.isEnemy = (i % 2 == 0);
        player.health = 100 - (i * 5);
        player.team = (i % 2);
        player.position = Vec3(
            (float)(rand() % 2000 - 1000),
            (float)(rand() % 2000 - 1000),
            0.0f
        );
        player.yaw = (float)(rand() % 360);
        snprintf(player.name, sizeof(player.name), "Player%d", i + 1);
        g_Players.push_back(player);
    }
    
    g_LocalPosition = Vec3(0, 0, 0);
    g_LocalYaw = 0.0f;
    g_LocalTeam = 1;
#endif
    
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
