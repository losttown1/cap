// DMA_Engine.cpp - PROJECT ZERO DMA Memory Engine
// Black Ops 6 Radar with FPGA DMA or Simulation

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

// Set to 1 for real DMA, 0 for simulation
#define DMA_ENABLED 0

#if DMA_ENABLED
    #include "../include/vmmdll.h"
    #pragma comment(lib, "../libs/vmmdll.lib")
    static VMM_HANDLE g_hVMM = nullptr;
#else
    typedef void* VMM_HANDLE;
    static VMM_HANDLE g_hVMM = nullptr;
#endif

//=============================================================================
// State
//=============================================================================

static bool g_bConnected = false;
static DWORD g_dwPID = 0;
static ULONG64 g_ModuleBase = 0;
static std::vector<PlayerData> g_Players;
static std::mutex g_Mutex;

// Local player
static Vec3 g_LocalPos(0, 0, 0);
static float g_LocalYaw = 0.0f;
static int g_LocalTeam = 1;

//=============================================================================
// Initialization
//=============================================================================

bool InitializeZeroDMA()
{
    if (g_bConnected) return true;

    std::cout << "[DMA] Target: " << TARGET_PROCESS_NAME << std::endl;

#if DMA_ENABLED
    std::cout << "[DMA] Mode: HARDWARE" << std::endl;
    
    LPCSTR args[] = { "", "-device", "fpga" };
    g_hVMM = VMMDLL_Initialize(3, (LPSTR*)args);
    
    if (!g_hVMM) {
        std::cerr << "[DMA] FPGA connection failed" << std::endl;
        std::cerr << "[DMA] Falling back to simulation" << std::endl;
        goto sim_mode;
    }
    
    DWORD pid = 0;
    if (VMMDLL_PidGetFromName(g_hVMM, (LPSTR)TARGET_PROCESS_NAME, &pid)) {
        g_dwPID = pid;
        std::cout << "[DMA] Found PID: " << pid << std::endl;
    } else {
        std::cerr << "[DMA] Process not found" << std::endl;
    }
    
    g_bConnected = true;
    return true;
    
sim_mode:
#endif
    // Simulation mode
    std::cout << "[DMA] Mode: SIMULATION" << std::endl;
    
    g_bConnected = true;
    g_dwPID = 12345;
    g_ModuleBase = 0x140000000;
    
    // Create test players
    srand((unsigned)time(nullptr));
    g_Players.clear();
    
    for (int i = 0; i < 12; i++)
    {
        PlayerData p;
        p.valid = true;
        p.isEnemy = (i < 6);
        p.health = 100 - (i * 5);
        p.team = p.isEnemy ? 2 : 1;
        
        // Position players in a circle
        float angle = (float)i * (360.0f / 12.0f) * 3.14159f / 180.0f;
        float dist = 150.0f + (float)(rand() % 300);
        p.position = Vec3(cosf(angle) * dist, sinf(angle) * dist, 0);
        p.yaw = (float)(rand() % 360);
        p.distance = dist;
        
        snprintf(p.name, sizeof(p.name), "%s_%d", p.isEnemy ? "Enemy" : "Ally", i + 1);
        g_Players.push_back(p);
    }
    
    std::cout << "[DMA] Created " << g_Players.size() << " test players" << std::endl;
    return true;
}

void ShutdownZeroDMA()
{
#if DMA_ENABLED
    if (g_hVMM) {
        VMMDLL_Close(g_hVMM);
        g_hVMM = nullptr;
    }
#endif
    g_bConnected = false;
    g_Players.clear();
}

bool IsConnected() { return g_bConnected; }

//=============================================================================
// Memory Operations (stubs in simulation)
//=============================================================================

bool ReadMemory(ULONG64 addr, void* buf, SIZE_T size)
{
#if DMA_ENABLED
    if (!g_hVMM) return false;
    return VMMDLL_MemRead(g_hVMM, g_dwPID, addr, (PBYTE)buf, (DWORD)size);
#else
    memset(buf, 0, size);
    return true;
#endif
}

bool WriteMemory(ULONG64 addr, void* buf, SIZE_T size)
{
#if DMA_ENABLED
    if (!g_hVMM) return false;
    return VMMDLL_MemWrite(g_hVMM, g_dwPID, addr, (PBYTE)buf, (DWORD)size);
#else
    return true;
#endif
}

ULONG64 GetModuleBase(const char* name)
{
#if DMA_ENABLED
    if (!g_hVMM) return 0;
    return VMMDLL_ProcessGetModuleBase(g_hVMM, g_dwPID, (LPSTR)name);
#else
    return g_ModuleBase;
#endif
}

//=============================================================================
// Player Data
//=============================================================================

bool UpdatePlayerList()
{
    if (!g_bConnected) return false;

#if DMA_ENABLED
    // Real implementation would read from game memory here
#else
    // Simulation - animate players
    static float time = 0.0f;
    time += 0.02f;
    
    for (size_t i = 0; i < g_Players.size(); i++)
    {
        float angle = time + (float)i * 0.5f;
        float dist = 150.0f + sinf(time * 0.5f + i) * 100.0f;
        g_Players[i].position.x = cosf(angle) * dist;
        g_Players[i].position.y = sinf(angle) * dist;
        g_Players[i].distance = dist;
        g_Players[i].yaw = angle * 180.0f / 3.14159f;
    }
    
    // Rotate local player view
    g_LocalYaw = fmodf(g_LocalYaw + 0.5f, 360.0f);
#endif
    
    return true;
}

const std::vector<PlayerData>& GetPlayerList() { return g_Players; }
Vec3 GetLocalPlayerPosition() { return g_LocalPos; }
float GetLocalPlayerYaw() { return g_LocalYaw; }
int GetLocalPlayerTeam() { return g_LocalTeam; }

//=============================================================================
// Radar Rendering
//=============================================================================

Vec2 WorldToRadar(const Vec3& world, const Vec3& local, float yaw,
                   const Vec2& center, float size, float zoom)
{
    float dx = world.x - local.x;
    float dy = world.y - local.y;
    
    // Rotate by yaw
    float rad = yaw * 3.14159f / 180.0f;
    float rx = dx * cosf(rad) - dy * sinf(rad);
    float ry = dx * sinf(rad) + dy * cosf(rad);
    
    // Scale
    float scale = (size * 0.5f) / (400.0f / zoom);
    rx *= scale;
    ry *= scale;
    
    // Clamp to radar
    float maxR = size * 0.5f - 5.0f;
    float len = sqrtf(rx * rx + ry * ry);
    if (len > maxR) {
        rx = rx / len * maxR;
        ry = ry / len * maxR;
    }
    
    return Vec2(center.x + rx, center.y - ry);
}

void RenderRadarOverlay()
{
    if (!g_bConnected) return;
    
    // Update player positions
    UpdatePlayerList();
    
    // Get draw list
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    
    // Radar settings
    float radarX = 20.0f;
    float radarY = 20.0f;
    float radarSize = 200.0f;
    float radarZoom = 1.5f;
    
    Vec2 center(radarX + radarSize * 0.5f, radarY + radarSize * 0.5f);
    
    // Background
    draw->AddRectFilled(
        ImVec2(radarX, radarY),
        ImVec2(radarX + radarSize, radarY + radarSize),
        IM_COL32(0, 0, 0, 200),
        5.0f
    );
    
    // Border
    draw->AddRect(
        ImVec2(radarX, radarY),
        ImVec2(radarX + radarSize, radarY + radarSize),
        IM_COL32(0, 255, 0, 255),
        5.0f, 0, 2.0f
    );
    
    // Crosshair
    draw->AddLine(
        ImVec2(center.x, radarY + 5),
        ImVec2(center.x, radarY + radarSize - 5),
        IM_COL32(50, 50, 50, 200)
    );
    draw->AddLine(
        ImVec2(radarX + 5, center.y),
        ImVec2(radarX + radarSize - 5, center.y),
        IM_COL32(50, 50, 50, 200)
    );
    
    // Local player (center)
    draw->AddCircleFilled(
        ImVec2(center.x, center.y),
        6.0f,
        IM_COL32(0, 255, 0, 255)
    );
    
    // Direction arrow
    float yawRad = g_LocalYaw * 3.14159f / 180.0f;
    draw->AddLine(
        ImVec2(center.x, center.y),
        ImVec2(center.x + sinf(yawRad) * 15.0f, center.y - cosf(yawRad) * 15.0f),
        IM_COL32(0, 255, 0, 255),
        2.0f
    );
    
    // Draw players
    for (const auto& p : g_Players)
    {
        if (!p.valid) continue;
        
        Vec2 pos = WorldToRadar(p.position, g_LocalPos, g_LocalYaw, center, radarSize, radarZoom);
        
        // Color: red for enemy, blue for team
        ImU32 col = p.isEnemy ? IM_COL32(255, 50, 50, 255) : IM_COL32(50, 100, 255, 255);
        
        // Player dot
        draw->AddCircleFilled(ImVec2(pos.x, pos.y), 4.0f, col);
        
        // Direction
        float pYaw = p.yaw * 3.14159f / 180.0f;
        draw->AddLine(
            ImVec2(pos.x, pos.y),
            ImVec2(pos.x + sinf(pYaw) * 8.0f, pos.y - cosf(pYaw) * 8.0f),
            col, 1.5f
        );
    }
    
    // Title
    draw->AddText(
        ImVec2(radarX + 5, radarY + radarSize + 5),
        IM_COL32(200, 200, 200, 255),
        "RADAR"
    );
    
    // Player count
    char buf[32];
    snprintf(buf, sizeof(buf), "Players: %d", (int)g_Players.size());
    draw->AddText(
        ImVec2(radarX + 5, radarY + radarSize + 20),
        IM_COL32(150, 150, 150, 255),
        buf
    );
}
