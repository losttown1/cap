// PROJECT ZERO - Simple Reliable Menu UI
// Without Columns (not in minimal imgui.h)

#include "ZeroUI.hpp"
#include "DMA_Engine.hpp"
#include "../include/imgui.h"
#include <cstdio>

// ============================================================================
// Global Settings
// ============================================================================

bool esp_enabled = true;
bool g_ESP_Box = true;
bool g_ESP_Skeleton = true;
bool g_ESP_Health = true;
bool g_ESP_Name = true;
bool g_ESP_Distance = true;
bool g_ESP_HeadCircle = true;
bool g_ESP_Snapline = false;
int g_ESP_BoxType = 1;

bool aimbot_enabled = false;
float g_AimFOV = 120.0f;
float g_AimSmooth = 7.5f;
int g_AimBone = 0;
bool g_AimVisible = true;
float g_AimMaxDist = 150.0f;

bool g_Triggerbot = false;
bool g_NoFlash = true;
bool g_NoSmoke = true;
bool g_RadarHack = false;
bool g_Bhop = true;
bool g_MagicBullet = true;

float g_RadarSize = 250.0f;
float g_RadarZoom = 1.5f;
float g_RadarX = 20.0f;
float g_RadarY = 20.0f;
bool g_ShowEnemies = true;
bool g_ShowTeam = true;
bool g_ShowDistance = true;
float box_color[4] = { 1.0f, 0.2f, 0.2f, 1.0f };
float team_color[4] = { 0.2f, 0.5f, 1.0f, 1.0f };
float refresh_rate = 144.0f;

// ============================================================================
// Style
// ============================================================================
void InitializeZeroUI()
{
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(10, 10);
    style.ScrollbarSize = 15.0f;
    style.GrabMinSize = 10.0f;
    style.WindowBorderSize = 1.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.80f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.60f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.80f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.80f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.80f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.95f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.70f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.85f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.70f, 0.15f, 0.15f, 0.80f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.80f, 0.20f, 0.20f, 0.90f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.90f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.75f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.60f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.40f, 0.12f, 0.12f, 1.00f);
}

void ShutdownZeroUI() {}

// ============================================================================
// Main Menu
// ============================================================================
void RenderZeroMenu()
{
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("PROJECT ZERO | BO6 DMA", nullptr, ImGuiWindowFlags_NoCollapse);
    
    // Status
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "STATUS:");
    ImGui::SameLine();
    ImGui::TextColored(IsConnected() ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f) : ImVec4(0.9f, 0.9f, 0.3f, 1.0f),
                       IsConnected() ? "ONLINE" : "SIMULATION");
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Text("FPS: %.0f", io.Framerate);
    
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::BeginTabBar("Tabs", 0))
    {
        // ================== AIMBOT ==================
        if (ImGui::BeginTabItem("AIMBOT"))
        {
            ImGui::Spacing();
            
            ImGui::Checkbox("Enable Aimbot", &aimbot_enabled);
            
            if (aimbot_enabled)
            {
                ImGui::Spacing();
                ImGui::SliderFloat("FOV", &g_AimFOV, 1.0f, 180.0f, "%.0f");
                ImGui::SliderFloat("Smoothness", &g_AimSmooth, 1.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("Max Distance", &g_AimMaxDist, 10.0f, 500.0f, "%.0f m");
                
                ImGui::Spacing();
                const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
                ImGui::Combo("Target Bone", &g_AimBone, bones, 4);
                
                ImGui::Spacing();
                ImGui::Checkbox("Visibility Check", &g_AimVisible);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Text("Quick Toggles");
            ImGui::Checkbox("Triggerbot", &g_Triggerbot);
            ImGui::Checkbox("Magic Bullet", &g_MagicBullet);
            ImGui::Checkbox("Bhop", &g_Bhop);
            ImGui::Checkbox("No Flash", &g_NoFlash);
            ImGui::Checkbox("No Smoke", &g_NoSmoke);
            
            ImGui::EndTabItem();
        }
        
        // ================== ESP ==================
        if (ImGui::BeginTabItem("ESP"))
        {
            ImGui::Spacing();
            
            ImGui::Checkbox("Enable ESP", &esp_enabled);
            
            if (esp_enabled)
            {
                ImGui::Spacing();
                ImGui::Checkbox("Box ESP", &g_ESP_Box);
                if (g_ESP_Box)
                {
                    const char* types[] = { "2D Box", "Corner Box" };
                    ImGui::Combo("Box Type", &g_ESP_BoxType, types, 2);
                }
                
                ImGui::Checkbox("Skeleton ESP", &g_ESP_Skeleton);
                ImGui::Checkbox("Health Bars", &g_ESP_Health);
                ImGui::Checkbox("Name Tags", &g_ESP_Name);
                ImGui::Checkbox("Distance", &g_ESP_Distance);
                ImGui::Checkbox("Head Circle", &g_ESP_HeadCircle);
                ImGui::Checkbox("Snaplines", &g_ESP_Snapline);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Text("Filters");
            ImGui::Checkbox("Show Enemies", &g_ShowEnemies);
            ImGui::Checkbox("Show Teammates", &g_ShowTeam);
            
            ImGui::Spacing();
            
            ImGui::Text("Enemy Color");
            ImGui::ColorEdit4("##EnemyCol", box_color);
            
            ImGui::Text("Team Color");
            ImGui::ColorEdit4("##TeamCol", team_color);
            
            ImGui::EndTabItem();
        }
        
        // ================== RADAR ==================
        if (ImGui::BeginTabItem("RADAR"))
        {
            ImGui::Spacing();
            
            ImGui::SliderFloat("Radar Size", &g_RadarSize, 150.0f, 400.0f, "%.0f px");
            ImGui::SliderFloat("Radar Zoom", &g_RadarZoom, 0.5f, 5.0f, "%.1fx");
            
            ImGui::Spacing();
            ImGui::Checkbox("Radar Hack (UAV)", &g_RadarHack);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            const auto& players = GetPlayerList();
            ImGui::Text("Players Detected: %d", (int)players.size());
            
            ImGui::Spacing();
            
            if (ImGui::BeginChild("List", ImVec2(0, 200), true))
            {
                for (const auto& p : players)
                {
                    if (!p.valid) continue;
                    ImVec4 c = p.isEnemy ? ImVec4(1, 0.3f, 0.3f, 1) : ImVec4(0.3f, 0.6f, 1, 1);
                    ImGui::TextColored(c, "%s - HP:%d - %.0fm", p.name, p.health, p.distance);
                }
            }
            ImGui::EndChild();
            
            ImGui::EndTabItem();
        }
        
        // ================== SETTINGS ==================
        if (ImGui::BeginTabItem("SETTINGS"))
        {
            ImGui::Spacing();
            
            ImGui::Text("Connection");
            ImGui::Separator();
            ImGui::Text("Target: %s", TARGET_PROCESS_NAME);
            ImGui::Text("Mode: %s", IsConnected() ? "Hardware DMA" : "Simulation");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::SliderFloat("Refresh Rate", &refresh_rate, 30.0f, 240.0f, "%.0f Hz");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "PROJECT ZERO v2.0");
            ImGui::Text("DMA ESP & Radar for Black Ops 6");
            ImGui::Text("Close window with X button");
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void ToggleZeroMenu() {}
bool IsZeroMenuVisible() { return true; }
void SetZeroMenuVisible(bool) {}
