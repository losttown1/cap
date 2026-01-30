// PROJECT ZERO - DMA Engine & Radar
// Professional radar implementation for BO6

#include "DMA_Engine.hpp"
#include "ZeroUI.hpp"
#include "../include/imgui.h"
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>

#define DMA_ENABLED 0

#if DMA_ENABLED
#include "../include/vmmdll.h"
#pragma comment(lib, "../libs/vmmdll.lib")
static VMM_HANDLE g_VMM = nullptr;
#endif

// State
static bool g_Connected = false;
static DWORD g_PID = 0;
static ULONG64 g_Base = 0;
static std::vector<PlayerData> g_Players;

// Local player
static Vec3 g_LocalPos(0, 0, 0);
static float g_LocalYaw = 0.0f;

// External radar settings from ZeroUI
extern float g_RadarSize;
extern float g_RadarZoom;
extern float g_RadarX;
extern float g_RadarY;
extern bool g_ShowEnemies;
extern bool g_ShowTeam;
extern bool g_ShowDistance;
extern float box_color[4];

bool InitializeZeroDMA()
{
    if (g_Connected) return true;

#if DMA_ENABLED
    LPCSTR args[] = { "", "-device", "fpga" };
    g_VMM = VMMDLL_Initialize(3, (LPSTR*)args);
    if (!g_VMM) {
        std::cout << "[DMA] FPGA not found, using simulation" << std::endl;
        goto simulation;
    }
    
    DWORD pid = 0;
    if (VMMDLL_PidGetFromName(g_VMM, (LPSTR)TARGET_PROCESS_NAME, &pid)) {
        g_PID = pid;
        g_Connected = true;
        return true;
    }
    
simulation:
#endif

    // Simulation mode
    g_Connected = true;
    g_PID = 1234;
    g_Base = 0x140000000;
    
    // Create test players
    srand((unsigned)time(nullptr));
    g_Players.clear();
    
    for (int i = 0; i < 16; i++)
    {
        PlayerData p = {};
        p.valid = true;
        p.isEnemy = (i < 8);
        p.health = 100;
        p.team = p.isEnemy ? 2 : 1;
        
        float angle = (float)i * (6.28318f / 16.0f);
        float dist = 100.0f + (float)(rand() % 300);
        p.position = Vec3(cosf(angle) * dist, sinf(angle) * dist, 0);
        p.yaw = (float)(rand() % 360);
        p.distance = dist;
        
        snprintf(p.name, sizeof(p.name), "%s_%02d", p.isEnemy ? "Enemy" : "Team", i + 1);
        g_Players.push_back(p);
    }
    
    return true;
}

void ShutdownZeroDMA()
{
#if DMA_ENABLED
    if (g_VMM) { VMMDLL_Close(g_VMM); g_VMM = nullptr; }
#endif
    g_Connected = false;
    g_Players.clear();
}

bool IsConnected() { return g_Connected; }
const std::vector<PlayerData>& GetPlayerList() { return g_Players; }
Vec3 GetLocalPlayerPosition() { return g_LocalPos; }
float GetLocalPlayerYaw() { return g_LocalYaw; }
int GetLocalPlayerTeam() { return 1; }

bool ReadMemory(ULONG64 addr, void* buf, SIZE_T size)
{
#if DMA_ENABLED
    if (g_VMM) return VMMDLL_MemRead(g_VMM, g_PID, addr, (PBYTE)buf, (DWORD)size);
#endif
    memset(buf, 0, size);
    return true;
}

bool WriteMemory(ULONG64 addr, void* buf, SIZE_T size)
{
#if DMA_ENABLED
    if (g_VMM) return VMMDLL_MemWrite(g_VMM, g_PID, addr, (PBYTE)buf, (DWORD)size);
#endif
    return true;
}

ULONG64 GetModuleBase(const char*) { return g_Base; }

// Update simulation
static void UpdateSimulation()
{
    static float t = 0.0f;
    t += 0.016f;
    
    for (size_t i = 0; i < g_Players.size(); i++)
    {
        float angle = t * 0.5f + (float)i * 0.4f;
        float dist = 150.0f + sinf(t + i * 0.5f) * 100.0f;
        
        g_Players[i].position.x = cosf(angle) * dist;
        g_Players[i].position.y = sinf(angle) * dist;
        g_Players[i].distance = dist;
        g_Players[i].yaw = angle * 57.3f;
    }
    
    g_LocalYaw = fmodf(t * 20.0f, 360.0f);
}

// Convert world position to radar position
static ImVec2 WorldToRadar(const Vec3& pos, const Vec3& local, float yaw, 
                            float cx, float cy, float size, float zoom)
{
    float dx = pos.x - local.x;
    float dy = pos.y - local.y;
    
    // Rotate
    float rad = yaw * 0.0174533f;
    float rx = dx * cosf(rad) - dy * sinf(rad);
    float ry = dx * sinf(rad) + dy * cosf(rad);
    
    // Scale
    float scale = (size * 0.45f) / (300.0f / zoom);
    rx *= scale;
    ry *= scale;
    
    // Clamp
    float maxR = size * 0.45f;
    float len = sqrtf(rx * rx + ry * ry);
    if (len > maxR) {
        rx = rx / len * maxR;
        ry = ry / len * maxR;
    }
    
    return ImVec2(cx + rx, cy - ry);
}

void RenderRadarOverlay()
{
    if (!g_Connected) return;
    
    UpdateSimulation();
    
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    
    // Radar bounds
    float x = g_RadarX;
    float y = g_RadarY;
    float size = g_RadarSize;
    float cx = x + size * 0.5f;
    float cy = y + size * 0.5f;
    
    // Background with gradient effect
    draw->AddRectFilled(
        ImVec2(x, y), 
        ImVec2(x + size, y + size),
        IM_COL32(10, 10, 15, 230),
        8.0f
    );
    
    // Border glow
    draw->AddRect(
        ImVec2(x - 1, y - 1),
        ImVec2(x + size + 1, y + size + 1),
        IM_COL32(0, 80, 0, 100),
        8.0f, 0, 3.0f
    );
    
    // Border
    draw->AddRect(
        ImVec2(x, y),
        ImVec2(x + size, y + size),
        IM_COL32(30, 180, 30, 255),
        8.0f, 0, 2.0f
    );
    
    // Grid lines
    ImU32 gridCol = IM_COL32(40, 40, 50, 150);
    
    // Cross
    draw->AddLine(ImVec2(cx, y + 10), ImVec2(cx, y + size - 10), gridCol, 1.0f);
    draw->AddLine(ImVec2(x + 10, cy), ImVec2(x + size - 10, cy), gridCol, 1.0f);
    
    // Circles
    for (int i = 1; i <= 3; i++)
    {
        float r = (size * 0.45f) * ((float)i / 3.0f);
        draw->AddCircle(ImVec2(cx, cy), r, gridCol, 32, 1.0f);
    }
    
    // Compass
    ImU32 compassCol = IM_COL32(100, 100, 100, 200);
    draw->AddText(ImVec2(cx - 3, y + 2), compassCol, "N");
    draw->AddText(ImVec2(cx - 3, y + size - 14), compassCol, "S");
    draw->AddText(ImVec2(x + 2, cy - 6), compassCol, "W");
    draw->AddText(ImVec2(x + size - 10, cy - 6), compassCol, "E");
    
    // Draw players
    for (const auto& p : g_Players)
    {
        if (!p.valid) continue;
        if (p.isEnemy && !g_ShowEnemies) continue;
        if (!p.isEnemy && !g_ShowTeam) continue;
        
        ImVec2 pos = WorldToRadar(p.position, g_LocalPos, g_LocalYaw, cx, cy, size, g_RadarZoom);
        
        // Color
        ImU32 col;
        if (p.isEnemy)
        {
            col = IM_COL32(
                (int)(box_color[0] * 255),
                (int)(box_color[1] * 255),
                (int)(box_color[2] * 255),
                255
            );
        }
        else
        {
            col = IM_COL32(80, 140, 255, 255);
        }
        
        // Glow
        draw->AddCircleFilled(pos, 6.0f, IM_COL32(0, 0, 0, 100));
        
        // Dot
        draw->AddCircleFilled(pos, 4.0f, col);
        
        // Direction arrow
        float yawRad = p.yaw * 0.0174533f;
        ImVec2 dir(pos.x + sinf(yawRad) * 10.0f, pos.y - cosf(yawRad) * 10.0f);
        draw->AddLine(pos, dir, col, 2.0f);
        
        // Distance text
        if (g_ShowDistance && p.distance < 500)
        {
            char dist[16];
            snprintf(dist, sizeof(dist), "%.0fm", p.distance);
            draw->AddText(ImVec2(pos.x + 8, pos.y - 6), IM_COL32(200, 200, 200, 200), dist);
        }
    }
    
    // Local player (center)
    draw->AddCircleFilled(ImVec2(cx, cy), 6.0f, IM_COL32(0, 255, 0, 255));
    draw->AddCircle(ImVec2(cx, cy), 8.0f, IM_COL32(0, 255, 0, 100), 12, 2.0f);
    
    // View direction
    float viewRad = g_LocalYaw * 0.0174533f;
    ImVec2 viewDir(cx + sinf(viewRad) * 20.0f, cy - cosf(viewRad) * 20.0f);
    draw->AddLine(ImVec2(cx, cy), viewDir, IM_COL32(0, 255, 0, 255), 2.0f);
    
    // Title bar
    draw->AddRectFilled(
        ImVec2(x, y + size),
        ImVec2(x + size, y + size + 25),
        IM_COL32(10, 10, 15, 230),
        0.0f
    );
    
    // Title
    draw->AddText(ImVec2(x + 8, y + size + 5), IM_COL32(30, 180, 30, 255), "RADAR");
    
    // Player count
    char info[32];
    snprintf(info, sizeof(info), "%d players", (int)g_Players.size());
    draw->AddText(ImVec2(x + size - 70, y + size + 5), IM_COL32(150, 150, 150, 255), info);
}
