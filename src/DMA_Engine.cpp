// PROJECT ZERO - Professional DMA Engine & ESP System
// Full ESP: Box, Skeleton, Health, Names, Distance
// Radar + 3D ESP like professional market tools

#include "DMA_Engine.hpp"
#include "ZeroUI.hpp"
#include "../include/imgui.h"
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>

#define DMA_ENABLED 0
#define PI 3.14159265358979323846f

#if DMA_ENABLED
#include "../include/vmmdll.h"
#pragma comment(lib, "../libs/vmmdll.lib")
static VMM_HANDLE g_VMM = nullptr;
#endif

// ============================================================================
// State
// ============================================================================
static bool g_Connected = false;
static DWORD g_PID = 0;
static ULONG64 g_Base = 0;
static std::vector<PlayerData> g_Players;

static Vec3 g_LocalPos(0, 0, 0);
static float g_LocalYaw = 0.0f;
static float g_LocalPitch = 0.0f;

// Screen dimensions
static float g_ScreenW = 2560.0f;
static float g_ScreenH = 1440.0f;

// ============================================================================
// External settings from ZeroUI
// ============================================================================
extern float g_RadarSize;
extern float g_RadarZoom;
extern float g_RadarX;
extern float g_RadarY;
extern bool g_ShowEnemies;
extern bool g_ShowTeam;
extern bool g_ShowDistance;
extern float box_color[4];

// ESP Settings
extern bool g_ESP_Box;
extern bool g_ESP_Skeleton;
extern bool g_ESP_Health;
extern bool g_ESP_Name;
extern bool g_ESP_Distance;
extern bool g_ESP_HeadCircle;
extern bool g_ESP_Snapline;
extern int g_ESP_BoxType;

// Misc Settings
extern bool g_Triggerbot;
extern bool g_NoFlash;
extern bool g_NoSmoke;
extern bool g_RadarHack;
extern bool g_Bhop;
extern bool g_MagicBullet;

// ============================================================================
// Bone structure for skeleton
// ============================================================================
struct BonePos {
    Vec3 head;
    Vec3 neck;
    Vec3 chest;
    Vec3 spine;
    Vec3 pelvis;
    Vec3 leftShoulder;
    Vec3 rightShoulder;
    Vec3 leftElbow;
    Vec3 rightElbow;
    Vec3 leftHand;
    Vec3 rightHand;
    Vec3 leftHip;
    Vec3 rightHip;
    Vec3 leftKnee;
    Vec3 rightKnee;
    Vec3 leftFoot;
    Vec3 rightFoot;
};

// ============================================================================
// Initialization
// ============================================================================
bool InitializeZeroDMA()
{
    if (g_Connected) return true;

#if DMA_ENABLED
    LPCSTR args[] = { "", "-device", "fpga" };
    g_VMM = VMMDLL_Initialize(3, (LPSTR*)args);
    if (!g_VMM) goto simulation;
    
    DWORD pid = 0;
    if (VMMDLL_PidGetFromName(g_VMM, (LPSTR)TARGET_PROCESS_NAME, &pid)) {
        g_PID = pid;
        g_Connected = true;
        return true;
    }
simulation:
#endif

    g_Connected = true;
    g_PID = 1234;
    g_Base = 0x140000000;
    
    srand((unsigned)time(nullptr));
    g_Players.clear();
    
    // Create test players with full data
    for (int i = 0; i < 12; i++)
    {
        PlayerData p = {};
        p.valid = true;
        p.isEnemy = (i < 6);
        p.health = 20 + (rand() % 80);
        p.maxHealth = 100;
        p.team = p.isEnemy ? 2 : 1;
        p.isVisible = (rand() % 2) == 0;
        
        // Random position around player
        float angle = (float)i * (2.0f * PI / 12.0f);
        float dist = 5.0f + (float)(rand() % 30);
        p.position = Vec3(cosf(angle) * dist, sinf(angle) * dist, 0);
        p.yaw = (float)(rand() % 360);
        p.distance = dist;
        
        snprintf(p.name, sizeof(p.name), "%s_%02d", p.isEnemy ? "Enemy" : "Ally", i + 1);
        
        // Screen position (simulated WorldToScreen)
        p.screenPos = Vec2(
            g_ScreenW * 0.3f + (float)(rand() % (int)(g_ScreenW * 0.4f)),
            g_ScreenH * 0.2f + (float)(rand() % (int)(g_ScreenH * 0.5f))
        );
        p.screenHeight = 80.0f + (float)(rand() % 120);
        p.onScreen = true;
        
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

bool ReadMemory(ULONG64 addr, void* buf, SIZE_T size) {
#if DMA_ENABLED
    if (g_VMM) return VMMDLL_MemRead(g_VMM, g_PID, addr, (PBYTE)buf, (DWORD)size);
#endif
    memset(buf, 0, size);
    return true;
}

bool WriteMemory(ULONG64 addr, void* buf, SIZE_T size) {
#if DMA_ENABLED
    if (g_VMM) return VMMDLL_MemWrite(g_VMM, g_PID, addr, (PBYTE)buf, (DWORD)size);
#endif
    return true;
}

ULONG64 GetModuleBase(const char*) { return g_Base; }

// ============================================================================
// Simulation Update
// ============================================================================
static void UpdateSimulation()
{
    static float t = 0.0f;
    t += 0.016f;
    
    ImGuiIO& io = ImGui::GetIO();
    g_ScreenW = io.DisplaySize.x;
    g_ScreenH = io.DisplaySize.y;
    
    for (size_t i = 0; i < g_Players.size(); i++)
    {
        PlayerData& p = g_Players[i];
        
        // Animate world position
        float angle = t * 0.3f + (float)i * 0.5f;
        float dist = 10.0f + sinf(t * 0.5f + i) * 5.0f;
        p.position.x = cosf(angle) * dist;
        p.position.y = sinf(angle) * dist;
        p.distance = dist;
        p.yaw = angle * 57.3f;
        
        // Simulate screen position (moving around screen)
        float sx = g_ScreenW * 0.5f + cosf(t * 0.5f + i * 0.8f) * (g_ScreenW * 0.35f);
        float sy = g_ScreenH * 0.3f + sinf(t * 0.3f + i * 0.6f) * (g_ScreenH * 0.25f);
        
        p.screenPos = Vec2(sx, sy);
        p.screenHeight = 100.0f + sinf(t + i) * 50.0f;
        p.onScreen = (sx > 50 && sx < g_ScreenW - 50 && sy > 50 && sy < g_ScreenH - 100);
        
        // Vary health
        p.health = 30 + (int)(sinf(t * 0.2f + i) * 35.0f + 35.0f);
    }
    
    g_LocalYaw = fmodf(t * 15.0f, 360.0f);
}

// ============================================================================
// Helper: Get bone screen positions (simulated)
// ============================================================================
static BonePos GetBoneScreenPositions(const PlayerData& p)
{
    BonePos b;
    float h = p.screenHeight;
    float x = p.screenPos.x;
    float y = p.screenPos.y;
    float w = h * 0.3f;
    
    // Head to feet positions
    b.head = Vec3(x, y - h * 0.45f, 0);
    b.neck = Vec3(x, y - h * 0.38f, 0);
    b.chest = Vec3(x, y - h * 0.25f, 0);
    b.spine = Vec3(x, y - h * 0.1f, 0);
    b.pelvis = Vec3(x, y, 0);
    
    // Arms
    b.leftShoulder = Vec3(x - w * 0.5f, y - h * 0.32f, 0);
    b.rightShoulder = Vec3(x + w * 0.5f, y - h * 0.32f, 0);
    b.leftElbow = Vec3(x - w * 0.7f, y - h * 0.15f, 0);
    b.rightElbow = Vec3(x + w * 0.7f, y - h * 0.15f, 0);
    b.leftHand = Vec3(x - w * 0.6f, y + h * 0.05f, 0);
    b.rightHand = Vec3(x + w * 0.6f, y + h * 0.05f, 0);
    
    // Legs
    b.leftHip = Vec3(x - w * 0.2f, y + h * 0.02f, 0);
    b.rightHip = Vec3(x + w * 0.2f, y + h * 0.02f, 0);
    b.leftKnee = Vec3(x - w * 0.25f, y + h * 0.28f, 0);
    b.rightKnee = Vec3(x + w * 0.25f, y + h * 0.28f, 0);
    b.leftFoot = Vec3(x - w * 0.3f, y + h * 0.5f, 0);
    b.rightFoot = Vec3(x + w * 0.3f, y + h * 0.5f, 0);
    
    return b;
}

// ============================================================================
// Draw ESP Box
// ============================================================================
static void DrawBox2D(ImDrawList* draw, float x, float y, float w, float h, ImU32 col)
{
    draw->AddRect(ImVec2(x - w/2, y - h/2), ImVec2(x + w/2, y + h/2), col, 0, 0, 2.0f);
}

static void DrawBoxCorner(ImDrawList* draw, float x, float y, float w, float h, ImU32 col)
{
    float cornerLen = w * 0.25f;
    float left = x - w/2, right = x + w/2;
    float top = y - h/2, bottom = y + h/2;
    
    // Top left
    draw->AddLine(ImVec2(left, top), ImVec2(left + cornerLen, top), col, 2.0f);
    draw->AddLine(ImVec2(left, top), ImVec2(left, top + cornerLen), col, 2.0f);
    // Top right
    draw->AddLine(ImVec2(right, top), ImVec2(right - cornerLen, top), col, 2.0f);
    draw->AddLine(ImVec2(right, top), ImVec2(right, top + cornerLen), col, 2.0f);
    // Bottom left
    draw->AddLine(ImVec2(left, bottom), ImVec2(left + cornerLen, bottom), col, 2.0f);
    draw->AddLine(ImVec2(left, bottom), ImVec2(left, bottom - cornerLen), col, 2.0f);
    // Bottom right
    draw->AddLine(ImVec2(right, bottom), ImVec2(right - cornerLen, bottom), col, 2.0f);
    draw->AddLine(ImVec2(right, bottom), ImVec2(right, bottom - cornerLen), col, 2.0f);
}

// ============================================================================
// Draw Skeleton
// ============================================================================
static void DrawSkeleton(ImDrawList* draw, const BonePos& b, ImU32 col)
{
    // Head
    draw->AddCircle(ImVec2(b.head.x, b.head.y), 8.0f, col, 12, 2.0f);
    
    // Spine
    draw->AddLine(ImVec2(b.head.x, b.head.y + 8), ImVec2(b.neck.x, b.neck.y), col, 2.0f);
    draw->AddLine(ImVec2(b.neck.x, b.neck.y), ImVec2(b.chest.x, b.chest.y), col, 2.0f);
    draw->AddLine(ImVec2(b.chest.x, b.chest.y), ImVec2(b.spine.x, b.spine.y), col, 2.0f);
    draw->AddLine(ImVec2(b.spine.x, b.spine.y), ImVec2(b.pelvis.x, b.pelvis.y), col, 2.0f);
    
    // Arms
    draw->AddLine(ImVec2(b.neck.x, b.neck.y), ImVec2(b.leftShoulder.x, b.leftShoulder.y), col, 2.0f);
    draw->AddLine(ImVec2(b.neck.x, b.neck.y), ImVec2(b.rightShoulder.x, b.rightShoulder.y), col, 2.0f);
    draw->AddLine(ImVec2(b.leftShoulder.x, b.leftShoulder.y), ImVec2(b.leftElbow.x, b.leftElbow.y), col, 2.0f);
    draw->AddLine(ImVec2(b.rightShoulder.x, b.rightShoulder.y), ImVec2(b.rightElbow.x, b.rightElbow.y), col, 2.0f);
    draw->AddLine(ImVec2(b.leftElbow.x, b.leftElbow.y), ImVec2(b.leftHand.x, b.leftHand.y), col, 2.0f);
    draw->AddLine(ImVec2(b.rightElbow.x, b.rightElbow.y), ImVec2(b.rightHand.x, b.rightHand.y), col, 2.0f);
    
    // Legs
    draw->AddLine(ImVec2(b.pelvis.x, b.pelvis.y), ImVec2(b.leftHip.x, b.leftHip.y), col, 2.0f);
    draw->AddLine(ImVec2(b.pelvis.x, b.pelvis.y), ImVec2(b.rightHip.x, b.rightHip.y), col, 2.0f);
    draw->AddLine(ImVec2(b.leftHip.x, b.leftHip.y), ImVec2(b.leftKnee.x, b.leftKnee.y), col, 2.0f);
    draw->AddLine(ImVec2(b.rightHip.x, b.rightHip.y), ImVec2(b.rightKnee.x, b.rightKnee.y), col, 2.0f);
    draw->AddLine(ImVec2(b.leftKnee.x, b.leftKnee.y), ImVec2(b.leftFoot.x, b.leftFoot.y), col, 2.0f);
    draw->AddLine(ImVec2(b.rightKnee.x, b.rightKnee.y), ImVec2(b.rightFoot.x, b.rightFoot.y), col, 2.0f);
}

// ============================================================================
// Draw Health Bar
// ============================================================================
static void DrawHealthBar(ImDrawList* draw, float x, float y, float w, float h, int health, int maxHealth)
{
    float barW = 4.0f;
    float barH = h;
    float barX = x - w/2 - barW - 3.0f;
    float barY = y - h/2;
    
    float healthPercent = (float)health / (float)maxHealth;
    float filledH = barH * healthPercent;
    
    // Background
    draw->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(0, 0, 0, 180));
    
    // Health fill (green to red gradient based on health)
    int r = (int)(255 * (1.0f - healthPercent));
    int g = (int)(255 * healthPercent);
    ImU32 healthCol = IM_COL32(r, g, 0, 255);
    
    draw->AddRectFilled(ImVec2(barX, barY + barH - filledH), ImVec2(barX + barW, barY + barH), healthCol);
    
    // Border
    draw->AddRect(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(0, 0, 0, 255));
}

// ============================================================================
// Render 3D ESP
// ============================================================================
void RenderESP()
{
    if (!g_Connected) return;
    
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    
    for (const auto& p : g_Players)
    {
        if (!p.valid || !p.onScreen) continue;
        if (p.isEnemy && !g_ShowEnemies) continue;
        if (!p.isEnemy && !g_ShowTeam) continue;
        
        float x = p.screenPos.x;
        float y = p.screenPos.y;
        float h = p.screenHeight;
        float w = h * 0.5f;
        
        // Color based on team and visibility
        ImU32 col;
        if (p.isEnemy) {
            if (p.isVisible)
                col = IM_COL32((int)(box_color[0]*255), (int)(box_color[1]*255), (int)(box_color[2]*255), 255);
            else
                col = IM_COL32(255, 150, 0, 255); // Orange when not visible
        } else {
            col = IM_COL32(80, 180, 255, 255); // Blue for team
        }
        
        // Box ESP
        if (g_ESP_Box) {
            if (g_ESP_BoxType == 0)
                DrawBox2D(draw, x, y, w, h, col);
            else
                DrawBoxCorner(draw, x, y, w, h, col);
        }
        
        // Skeleton ESP
        if (g_ESP_Skeleton) {
            BonePos bones = GetBoneScreenPositions(p);
            DrawSkeleton(draw, bones, col);
        }
        
        // Health bar
        if (g_ESP_Health) {
            DrawHealthBar(draw, x, y, w, h, p.health, p.maxHealth);
        }
        
        // Head circle
        if (g_ESP_HeadCircle) {
            float headY = y - h * 0.45f;
            draw->AddCircleFilled(ImVec2(x, headY), 5.0f, col);
        }
        
        // Snapline (from bottom of screen to player)
        if (g_ESP_Snapline) {
            draw->AddLine(ImVec2(g_ScreenW / 2, g_ScreenH), ImVec2(x, y + h/2), col, 1.0f);
        }
        
        // Name tag
        if (g_ESP_Name) {
            draw->AddText(ImVec2(x - 30, y - h/2 - 18), IM_COL32(255, 255, 255, 255), p.name);
        }
        
        // Distance
        if (g_ESP_Distance) {
            char dist[16];
            snprintf(dist, sizeof(dist), "%.0fm", p.distance);
            draw->AddText(ImVec2(x - 15, y + h/2 + 4), IM_COL32(200, 200, 200, 255), dist);
        }
    }
}

// ============================================================================
// Radar
// ============================================================================
static ImVec2 WorldToRadar(const Vec3& pos, const Vec3& local, float yaw, 
                            float cx, float cy, float size, float zoom)
{
    float dx = pos.x - local.x;
    float dy = pos.y - local.y;
    
    float rad = yaw * 0.0174533f;
    float rx = dx * cosf(rad) - dy * sinf(rad);
    float ry = dx * sinf(rad) + dy * cosf(rad);
    
    float scale = (size * 0.45f) / (30.0f / zoom);
    rx *= scale;
    ry *= scale;
    
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
    
    float x = g_RadarX;
    float y = g_RadarY;
    float size = g_RadarSize;
    float cx = x + size * 0.5f;
    float cy = y + size * 0.5f;
    
    // Background
    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + size, y + size), IM_COL32(15, 15, 20, 235), 6.0f);
    
    // Border glow
    draw->AddRect(ImVec2(x-1, y-1), ImVec2(x+size+1, y+size+1), IM_COL32(0, 100, 0, 80), 6.0f, 0, 4.0f);
    draw->AddRect(ImVec2(x, y), ImVec2(x+size, y+size), IM_COL32(40, 200, 40, 255), 6.0f, 0, 2.0f);
    
    // Grid
    ImU32 gridCol = IM_COL32(40, 40, 50, 150);
    draw->AddLine(ImVec2(cx, y+10), ImVec2(cx, y+size-10), gridCol);
    draw->AddLine(ImVec2(x+10, cy), ImVec2(x+size-10, cy), gridCol);
    for (int i = 1; i <= 3; i++) {
        float r = (size * 0.45f) * (i / 3.0f);
        draw->AddCircle(ImVec2(cx, cy), r, gridCol, 32);
    }
    
    // Compass
    ImU32 compassCol = IM_COL32(100, 100, 100, 200);
    draw->AddText(ImVec2(cx-3, y+2), compassCol, "N");
    draw->AddText(ImVec2(cx-3, y+size-14), compassCol, "S");
    draw->AddText(ImVec2(x+2, cy-6), compassCol, "W");
    draw->AddText(ImVec2(x+size-10, cy-6), compassCol, "E");
    
    // Players
    for (const auto& p : g_Players)
    {
        if (!p.valid) continue;
        if (p.isEnemy && !g_ShowEnemies) continue;
        if (!p.isEnemy && !g_ShowTeam) continue;
        
        ImVec2 pos = WorldToRadar(p.position, g_LocalPos, g_LocalYaw, cx, cy, size, g_RadarZoom);
        
        ImU32 col = p.isEnemy ? 
            IM_COL32((int)(box_color[0]*255), (int)(box_color[1]*255), (int)(box_color[2]*255), 255) :
            IM_COL32(80, 140, 255, 255);
        
        draw->AddCircleFilled(pos, 5.0f, col);
        
        float yawRad = p.yaw * 0.0174533f;
        draw->AddLine(pos, ImVec2(pos.x + sinf(yawRad)*10, pos.y - cosf(yawRad)*10), col, 2.0f);
    }
    
    // Local player
    draw->AddCircleFilled(ImVec2(cx, cy), 6.0f, IM_COL32(0, 255, 0, 255));
    draw->AddCircle(ImVec2(cx, cy), 8.0f, IM_COL32(0, 255, 0, 100), 12, 2.0f);
    
    float viewRad = g_LocalYaw * 0.0174533f;
    draw->AddLine(ImVec2(cx, cy), ImVec2(cx + sinf(viewRad)*18, cy - cosf(viewRad)*18), IM_COL32(0, 255, 0, 255), 2.0f);
    
    // Title bar
    draw->AddRectFilled(ImVec2(x, y+size), ImVec2(x+size, y+size+22), IM_COL32(15, 15, 20, 235));
    draw->AddText(ImVec2(x+6, y+size+4), IM_COL32(40, 200, 40, 255), "RADAR");
    
    char info[32];
    snprintf(info, sizeof(info), "%d players", (int)g_Players.size());
    draw->AddText(ImVec2(x+size-65, y+size+4), IM_COL32(150, 150, 150, 255), info);
    
    // Now render 3D ESP
    RenderESP();
}
