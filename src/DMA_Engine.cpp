// PROJECT ZERO - DMA Engine & Radar
// Simplified reliable implementation

#include "DMA_Engine.hpp"
#include "ZeroUI.hpp"
#include "../include/imgui.h"
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>

#define PI 3.14159265f

// External settings
extern float g_RadarSize;
extern float g_RadarZoom;
extern bool g_ShowEnemies;
extern bool g_ShowTeam;
extern float box_color[4];
extern float team_color[4];

extern bool g_ESP_Box;
extern bool g_ESP_Skeleton;
extern bool g_ESP_Health;
extern bool g_ESP_Name;
extern bool g_ESP_Distance;
extern bool g_ESP_HeadCircle;
extern bool g_ESP_Snapline;
extern int g_ESP_BoxType;

// State
static bool g_Connected = false;
static std::vector<PlayerData> g_Players;
static Vec3 g_LocalPos(0, 0, 0);
static float g_LocalYaw = 0.0f;
static float g_ScreenW = 1280.0f;
static float g_ScreenH = 720.0f;

bool InitializeZeroDMA()
{
    g_Connected = true;
    
    // Create test players
    srand((unsigned)time(nullptr));
    g_Players.clear();
    
    for (int i = 0; i < 12; i++)
    {
        PlayerData p = {};
        p.valid = true;
        p.isEnemy = (i < 6);
        p.health = 20 + rand() % 80;
        p.maxHealth = 100;
        p.team = p.isEnemy ? 2 : 1;
        p.isVisible = (rand() % 2) == 0;
        
        float angle = (float)i * (2.0f * PI / 12.0f);
        float dist = 10.0f + (float)(rand() % 30);
        p.position = Vec3(cosf(angle) * dist, sinf(angle) * dist, 0);
        p.yaw = (float)(rand() % 360);
        p.distance = dist;
        
        snprintf(p.name, sizeof(p.name), "%s_%02d", p.isEnemy ? "Enemy" : "Ally", i + 1);
        g_Players.push_back(p);
    }
    
    return true;
}

void ShutdownZeroDMA()
{
    g_Connected = false;
    g_Players.clear();
}

bool IsConnected() { return g_Connected; }
const std::vector<PlayerData>& GetPlayerList() { return g_Players; }
Vec3 GetLocalPlayerPosition() { return g_LocalPos; }
float GetLocalPlayerYaw() { return g_LocalYaw; }
int GetLocalPlayerTeam() { return 1; }

bool ReadMemory(ULONG64, void* buf, SIZE_T size) { memset(buf, 0, size); return true; }
bool WriteMemory(ULONG64, void*, SIZE_T) { return true; }
ULONG64 GetModuleBase(const char*) { return 0x140000000; }

// Update simulation
static void UpdateSim()
{
    static float t = 0.0f;
    t += 0.016f;
    
    ImGuiIO& io = ImGui::GetIO();
    g_ScreenW = io.DisplaySize.x;
    g_ScreenH = io.DisplaySize.y;
    
    for (size_t i = 0; i < g_Players.size(); i++)
    {
        PlayerData& p = g_Players[i];
        
        float angle = t * 0.3f + (float)i * 0.5f;
        float dist = 10.0f + sinf(t * 0.5f + (float)i) * 5.0f;
        p.position.x = cosf(angle) * dist;
        p.position.y = sinf(angle) * dist;
        p.distance = dist;
        p.yaw = angle * 57.3f;
        
        p.health = 30 + (int)(sinf(t * 0.2f + (float)i) * 35.0f + 35.0f);
    }
    
    g_LocalYaw = fmodf(t * 15.0f, 360.0f);
}

// World to radar conversion
static ImVec2 W2R(const Vec3& pos, float cx, float cy, float size, float zoom)
{
    float dx = pos.x - g_LocalPos.x;
    float dy = pos.y - g_LocalPos.y;
    
    float rad = g_LocalYaw * PI / 180.0f;
    float rx = dx * cosf(rad) - dy * sinf(rad);
    float ry = dx * sinf(rad) + dy * cosf(rad);
    
    float scale = (size * 0.45f) / (30.0f / zoom);
    rx *= scale;
    ry *= scale;
    
    float maxR = size * 0.45f;
    float len = sqrtf(rx * rx + ry * ry);
    if (len > maxR) { rx = rx / len * maxR; ry = ry / len * maxR; }
    
    return ImVec2(cx + rx, cy - ry);
}

void RenderRadarOverlay()
{
    if (!g_Connected) return;
    
    UpdateSim();
    
    ImGuiIO& io = ImGui::GetIO();
    
    // Radar window
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - g_RadarSize - 30, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(g_RadarSize + 20, g_RadarSize + 50), ImGuiCond_Always);
    
    ImGui::Begin("RADAR", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    float size = g_RadarSize;
    float cx = pos.x + size * 0.5f;
    float cy = pos.y + size * 0.5f;
    
    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(20, 20, 25, 255), 6.0f);
    draw->AddRect(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(180, 50, 50, 255), 6.0f, 0, 2.0f);
    
    // Grid
    ImU32 gridCol = IM_COL32(50, 50, 60, 200);
    draw->AddLine(ImVec2(cx, pos.y + 5), ImVec2(cx, pos.y + size - 5), gridCol);
    draw->AddLine(ImVec2(pos.x + 5, cy), ImVec2(pos.x + size - 5, cy), gridCol);
    for (int i = 1; i <= 3; i++)
    {
        float r = (size * 0.45f) * (i / 3.0f);
        draw->AddCircle(ImVec2(cx, cy), r, gridCol, 32);
    }
    
    // Players
    for (const auto& p : g_Players)
    {
        if (!p.valid) continue;
        if (p.isEnemy && !g_ShowEnemies) continue;
        if (!p.isEnemy && !g_ShowTeam) continue;
        
        ImVec2 rpos = W2R(p.position, cx, cy, size, g_RadarZoom);
        
        ImU32 col = p.isEnemy ? 
            IM_COL32((int)(box_color[0]*255), (int)(box_color[1]*255), (int)(box_color[2]*255), 255) :
            IM_COL32((int)(team_color[0]*255), (int)(team_color[1]*255), (int)(team_color[2]*255), 255);
        
        draw->AddCircleFilled(rpos, 5.0f, col);
        
        float yawRad = p.yaw * PI / 180.0f;
        draw->AddLine(rpos, ImVec2(rpos.x + sinf(yawRad) * 10, rpos.y - cosf(yawRad) * 10), col, 2.0f);
    }
    
    // Local player
    draw->AddCircleFilled(ImVec2(cx, cy), 6.0f, IM_COL32(50, 255, 50, 255));
    float viewRad = g_LocalYaw * PI / 180.0f;
    draw->AddLine(ImVec2(cx, cy), ImVec2(cx + sinf(viewRad) * 15, cy - cosf(viewRad) * 15), IM_COL32(50, 255, 50, 255), 2.0f);
    
    // Move cursor past the radar
    ImGui::Dummy(ImVec2(size, size));
    
    ImGui::Text("Players: %d", (int)g_Players.size());
    
    ImGui::End();
    
    // ESP Window (shows simulated ESP)
    if (esp_enabled)
    {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 320, g_RadarSize + 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 350), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("ESP PREVIEW", nullptr, ImGuiWindowFlags_NoCollapse);
        
        ImVec2 espPos = ImGui::GetCursorScreenPos();
        ImDrawList* espDraw = ImGui::GetWindowDrawList();
        
        // Draw some sample ESP boxes
        for (int i = 0; i < 3; i++)
        {
            float x = espPos.x + 50 + i * 90;
            float y = espPos.y + 50;
            float h = 120.0f - i * 20;
            float w = h * 0.4f;
            
            bool isEnemy = (i < 2);
            ImU32 col = isEnemy ? 
                IM_COL32((int)(box_color[0]*255), (int)(box_color[1]*255), (int)(box_color[2]*255), 255) :
                IM_COL32((int)(team_color[0]*255), (int)(team_color[1]*255), (int)(team_color[2]*255), 255);
            
            // Box
            if (g_ESP_Box)
            {
                if (g_ESP_BoxType == 0)
                {
                    espDraw->AddRect(ImVec2(x - w/2, y), ImVec2(x + w/2, y + h), col, 0, 0, 2.0f);
                }
                else
                {
                    float c = w * 0.3f;
                    // Corners
                    espDraw->AddLine(ImVec2(x - w/2, y), ImVec2(x - w/2 + c, y), col, 2.0f);
                    espDraw->AddLine(ImVec2(x - w/2, y), ImVec2(x - w/2, y + c), col, 2.0f);
                    espDraw->AddLine(ImVec2(x + w/2, y), ImVec2(x + w/2 - c, y), col, 2.0f);
                    espDraw->AddLine(ImVec2(x + w/2, y), ImVec2(x + w/2, y + c), col, 2.0f);
                    espDraw->AddLine(ImVec2(x - w/2, y + h), ImVec2(x - w/2 + c, y + h), col, 2.0f);
                    espDraw->AddLine(ImVec2(x - w/2, y + h), ImVec2(x - w/2, y + h - c), col, 2.0f);
                    espDraw->AddLine(ImVec2(x + w/2, y + h), ImVec2(x + w/2 - c, y + h), col, 2.0f);
                    espDraw->AddLine(ImVec2(x + w/2, y + h), ImVec2(x + w/2, y + h - c), col, 2.0f);
                }
            }
            
            // Skeleton
            if (g_ESP_Skeleton)
            {
                float headY = y + h * 0.05f;
                float neckY = y + h * 0.12f;
                float chestY = y + h * 0.25f;
                float pelvisY = y + h * 0.5f;
                float footY = y + h;
                float armW = w * 0.6f;
                
                espDraw->AddCircle(ImVec2(x, headY), 6, col, 12, 2.0f);
                espDraw->AddLine(ImVec2(x, headY + 6), ImVec2(x, pelvisY), col, 2.0f);
                espDraw->AddLine(ImVec2(x, neckY), ImVec2(x - armW, chestY + 20), col, 2.0f);
                espDraw->AddLine(ImVec2(x, neckY), ImVec2(x + armW, chestY + 20), col, 2.0f);
                espDraw->AddLine(ImVec2(x, pelvisY), ImVec2(x - w * 0.3f, footY), col, 2.0f);
                espDraw->AddLine(ImVec2(x, pelvisY), ImVec2(x + w * 0.3f, footY), col, 2.0f);
            }
            
            // Health bar
            if (g_ESP_Health)
            {
                int hp = 70 - i * 20;
                float hpPct = hp / 100.0f;
                float barH = h * hpPct;
                int r = (int)(255 * (1.0f - hpPct));
                int g = (int)(255 * hpPct);
                
                espDraw->AddRectFilled(ImVec2(x - w/2 - 7, y), ImVec2(x - w/2 - 3, y + h), IM_COL32(0, 0, 0, 200));
                espDraw->AddRectFilled(ImVec2(x - w/2 - 6, y + h - barH), ImVec2(x - w/2 - 4, y + h), IM_COL32(r, g, 0, 255));
            }
            
            // Name
            if (g_ESP_Name)
            {
                char name[16];
                snprintf(name, sizeof(name), "%s_%d", isEnemy ? "Enemy" : "Team", i + 1);
                espDraw->AddText(ImVec2(x - 25, y - 15), IM_COL32(255, 255, 255, 255), name);
            }
            
            // Distance
            if (g_ESP_Distance)
            {
                char dist[16];
                snprintf(dist, sizeof(dist), "%dm", 20 + i * 15);
                espDraw->AddText(ImVec2(x - 10, y + h + 3), IM_COL32(200, 200, 200, 255), dist);
            }
            
            // Head circle
            if (g_ESP_HeadCircle)
            {
                espDraw->AddCircleFilled(ImVec2(x, y + h * 0.05f), 4, col);
            }
        }
        
        ImGui::Dummy(ImVec2(280, 200));
        ImGui::Text("ESP Preview - Toggle options in ESP tab");
        
        ImGui::End();
    }
}
