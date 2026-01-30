// PROJECT ZERO - Professional Menu UI
// Modern design like market DMA tools

#include "ZeroUI.hpp"
#include "DMA_Engine.hpp"
#include "../include/imgui.h"
#include <cstdio>

// Exported settings
bool esp_enabled = true;
bool aimbot_enabled = false;
float refresh_rate = 144.0f;
float box_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

// Radar settings
float g_RadarSize = 250.0f;
float g_RadarZoom = 1.0f;
float g_RadarX = 20.0f;
float g_RadarY = 20.0f;
bool g_ShowEnemies = true;
bool g_ShowTeam = false;
bool g_ShowDistance = true;

// Aimbot settings  
float g_AimFOV = 15.0f;
float g_AimSmooth = 5.0f;
int g_AimBone = 0;
bool g_AimVisible = true;

void InitializeZeroUI()
{
    ImGuiStyle& s = ImGui::GetStyle();
    
    // Modern rounded style
    s.WindowRounding = 8.0f;
    s.ChildRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.PopupRounding = 4.0f;
    s.ScrollbarRounding = 4.0f;
    s.GrabRounding = 4.0f;
    s.TabRounding = 4.0f;
    
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    s.WindowPadding = ImVec2(12, 12);
    s.FramePadding = ImVec2(8, 4);
    s.ItemSpacing = ImVec2(10, 8);
    s.ItemInnerSpacing = ImVec2(6, 4);
    s.IndentSpacing = 20.0f;
    s.ScrollbarSize = 12.0f;
    s.GrabMinSize = 8.0f;
    
    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize = 1.0f;
    s.FrameBorderSize = 0.0f;
    s.PopupBorderSize = 1.0f;
    
    // Dark theme with accent color
    ImVec4* c = s.Colors;
    
    // Background colors
    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
    c[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.96f);
    c[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
    
    // Border
    c[ImGuiCol_Border] = ImVec4(0.20f, 0.60f, 0.20f, 0.60f);
    c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    // Frame
    c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    
    // Title
    c[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.06f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.40f, 0.10f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.04f, 0.04f, 0.06f, 0.60f);
    
    // Scrollbar
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.60f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.42f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    
    // Accent color (green)
    c[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.90f, 0.20f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.70f, 0.20f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 0.90f, 0.20f, 1.00f);
    
    // Button
    c[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.50f, 0.20f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.70f, 0.20f, 1.00f);
    
    // Header
    c[ImGuiCol_Header] = ImVec4(0.16f, 0.40f, 0.16f, 0.80f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.50f, 0.20f, 0.90f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.60f, 0.20f, 1.00f);
    
    // Separator
    c[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.32f, 0.60f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.70f, 0.20f, 0.80f);
    c[ImGuiCol_SeparatorActive] = ImVec4(0.20f, 0.90f, 0.20f, 1.00f);
    
    // Tab
    c[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    c[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.50f, 0.20f, 0.90f);
    c[ImGuiCol_TabActive] = ImVec4(0.16f, 0.40f, 0.16f, 1.00f);
    c[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.30f, 0.14f, 1.00f);
    
    // Text
    c[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}

void ShutdownZeroUI() {}

void RenderZeroMenu()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Center window on first use
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 200, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    
    if (!ImGui::Begin("PROJECT ZERO | BO6", nullptr, flags))
    {
        ImGui::End();
        return;
    }
    
    // Header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
    ImGui::Text("STATUS: %s", IsConnected() ? "CONNECTED" : "SIMULATION");
    ImGui::PopStyleColor();
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 80);
    ImGui::Text("%.0f FPS", io.Framerate);
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Tab bar
    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
    {
        // RADAR TAB
        if (ImGui::BeginTabItem("RADAR"))
        {
            ImGui::Spacing();
            
            ImGui::Checkbox("Enable Radar", &esp_enabled);
            
            if (esp_enabled)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Display");
                ImGui::SliderFloat("Size", &g_RadarSize, 150.0f, 400.0f, "%.0f px");
                ImGui::SliderFloat("Zoom", &g_RadarZoom, 0.5f, 3.0f, "%.1fx");
                ImGui::SliderFloat("Pos X", &g_RadarX, 0.0f, 500.0f, "%.0f");
                ImGui::SliderFloat("Pos Y", &g_RadarY, 0.0f, 500.0f, "%.0f");
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Filters");
                ImGui::Checkbox("Show Enemies", &g_ShowEnemies);
                ImGui::Checkbox("Show Teammates", &g_ShowTeam);
                ImGui::Checkbox("Show Distance", &g_ShowDistance);
                
                ImGui::Spacing();
                ImGui::ColorEdit4("Enemy Color", box_color);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Player list
            const auto& players = GetPlayerList();
            ImGui::Text("Players Detected: %d", (int)players.size());
            
            ImGui::EndTabItem();
        }
        
        // AIMBOT TAB
        if (ImGui::BeginTabItem("AIMBOT"))
        {
            ImGui::Spacing();
            
            ImGui::Checkbox("Enable Aimbot", &aimbot_enabled);
            
            if (aimbot_enabled)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Settings");
                ImGui::SliderFloat("FOV", &g_AimFOV, 1.0f, 90.0f, "%.0f");
                ImGui::SliderFloat("Smoothness", &g_AimSmooth, 1.0f, 20.0f, "%.1f");
                
                const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
                ImGui::Combo("Target Bone", &g_AimBone, bones, 4);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Options");
                ImGui::Checkbox("Visible Only", &g_AimVisible);
            }
            
            ImGui::EndTabItem();
        }
        
        // SETTINGS TAB
        if (ImGui::BeginTabItem("SETTINGS"))
        {
            ImGui::Spacing();
            
            ImGui::Text("Connection");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Text("Target: %s", TARGET_PROCESS_NAME);
            ImGui::Text("Mode: %s", IsConnected() ? "DMA Hardware" : "Simulation");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::Text("Hotkeys");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::BulletText("F1 - Toggle ALL Overlay");
            ImGui::BulletText("INSERT - Toggle Menu");
            ImGui::BulletText("END - Exit Program");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::SliderFloat("Refresh Rate", &refresh_rate, 30.0f, 240.0f, "%.0f Hz");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            // Credits
            ImGui::TextDisabled("PROJECT ZERO v1.0");
            ImGui::TextDisabled("DMA Radar for Black Ops 6");
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

// Stub functions
void ToggleZeroMenu() {}
bool IsZeroMenuVisible() { return true; }
void SetZeroMenuVisible(bool) {}
