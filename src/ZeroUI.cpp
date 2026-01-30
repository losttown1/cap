// ZeroUI.cpp - PROJECT ZERO User Interface
// Simplified menu that actually works

#include "ZeroUI.hpp"
#include "DMA_Engine.hpp"
#include "../include/imgui.h"
#include <iostream>
#include <cstdio>

// Settings - exported
bool esp_enabled = true;
bool aimbot_enabled = false;
float refresh_rate = 144.0f;
float box_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

// Local settings
static float radar_zoom = 1.5f;
static float radar_size = 200.0f;
static bool show_teammates = true;
static int target_bone = 0;
static float aim_fov = 15.0f;
static float aim_smooth = 5.0f;

void InitializeZeroUI()
{
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Simple dark style
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(5, 3);
    style.ItemSpacing = ImVec2(8, 6);
    
    // Colors
    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    c[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    c[ImGuiCol_ChildBg]               = ImVec4(0.15f, 0.15f, 0.15f, 0.95f);
    c[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.10f, 0.10f, 0.95f);
    c[ImGuiCol_Border]                = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    c[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.40f, 0.00f, 1.00f);
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.30f, 0.00f, 1.00f);
    c[ImGuiCol_CheckMark]             = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
    c[ImGuiCol_SliderGrab]            = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
    c[ImGuiCol_SliderGrabActive]      = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
    c[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.00f, 0.70f, 0.00f, 1.00f);
    c[ImGuiCol_Header]                = ImVec4(0.00f, 0.40f, 0.00f, 1.00f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.60f, 0.00f, 1.00f);
    c[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
    c[ImGuiCol_TabActive]             = ImVec4(0.00f, 0.40f, 0.00f, 1.00f);
    c[ImGuiCol_TabUnfocused]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    
    std::cout << "[UI] Initialized" << std::endl;
}

void ShutdownZeroUI()
{
    std::cout << "[UI] Shutdown" << std::endl;
}

void RenderZeroMenu()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Window position and size
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("PROJECT ZERO | BO6", nullptr, 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    
    // Status
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "STATUS: ONLINE");
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Text("FPS: %.0f", io.Framerate);
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // ===== RADAR SECTION =====
    if (ImGui::CollapsingHeader("RADAR", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable Radar", &esp_enabled);
        
        if (esp_enabled)
        {
            ImGui::SliderFloat("Size", &radar_size, 100.0f, 400.0f, "%.0f");
            ImGui::SliderFloat("Zoom", &radar_zoom, 0.5f, 5.0f, "%.1f");
            ImGui::Checkbox("Show Teammates", &show_teammates);
            ImGui::ColorEdit4("Enemy Color", box_color, ImGuiColorEditFlags_NoInputs);
        }
        
        // Player count
        const auto& players = GetPlayerList();
        ImGui::Text("Players: %d", (int)players.size());
    }
    
    ImGui::Spacing();
    
    // ===== AIMBOT SECTION =====
    if (ImGui::CollapsingHeader("AIMBOT"))
    {
        ImGui::Checkbox("Enable Aimbot", &aimbot_enabled);
        
        if (aimbot_enabled)
        {
            ImGui::SliderFloat("FOV", &aim_fov, 1.0f, 90.0f, "%.0f");
            ImGui::SliderFloat("Smooth", &aim_smooth, 1.0f, 20.0f, "%.1f");
            
            const char* bones[] = { "Head", "Neck", "Chest", "Body" };
            ImGui::Combo("Bone", &target_bone, bones, 4);
        }
    }
    
    ImGui::Spacing();
    
    // ===== SETTINGS SECTION =====
    if (ImGui::CollapsingHeader("SETTINGS"))
    {
        ImGui::Text("Target: %s", TARGET_PROCESS_NAME);
        ImGui::Text("Mode: %s", IsConnected() ? "DMA Connected" : "Simulation");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Hotkeys:");
        ImGui::BulletText("F1 = Toggle ALL overlay");
        ImGui::BulletText("INSERT = Toggle menu");
        ImGui::BulletText("END = Exit");
    }
    
    ImGui::End();
}

// These functions are now handled in ZeroMain.cpp
void ToggleZeroMenu() {}
bool IsZeroMenuVisible() { return true; }
void SetZeroMenuVisible(bool) {}
