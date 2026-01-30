// ZeroUI.cpp - Zero Elite User Interface Implementation
// PROJECT ZERO | Modern Menu Interface

#include "ZeroUI.hpp"
#include <imgui.h>
#include <iostream>

// Default settings for Zero system
bool esp_enabled = true;
bool aimbot_enabled = false;
float refresh_rate = 144.0f;
float box_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // Default red

// Menu visibility state
static bool g_MenuVisible = false;

void InitializeZeroUI()
{
    // Apply custom style
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Modern dark theme
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);

    // Color scheme - Dark with green accent
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.04f, 0.04f, 0.04f, 0.50f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.00f, 1.00f, 0.00f, 1.00f); // Green
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.00f, 0.60f, 0.00f, 0.70f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.60f, 0.00f, 0.70f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.00f, 0.80f, 0.00f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

    std::cout << "[PROJECT ZERO] UI Initialized" << std::endl;
}

void ShutdownZeroUI()
{
    std::cout << "[PROJECT ZERO] UI Shutdown" << std::endl;
}

void RenderZeroMenu() 
{
    if (!g_MenuVisible)
        return;

    // Set window style for modern look
    ImGui::SetNextWindowSize(ImVec2(380.0f, 320.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("PROJECT ZERO | PREVIEW", nullptr, ImGuiWindowFlags_NoCollapse);

    // System status header
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "System Status: ONLINE");
    ImGui::Separator();

    // Visuals section
    if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable ESP Boxes", &esp_enabled);
        ImGui::ColorEdit4("Box Color", box_color);
        ImGui::SliderFloat("Refresh Rate", &refresh_rate, 30.0f, 240.0f, "%.0f Hz");
        
        if (esp_enabled) {
            ImGui::Indent(16.0f);
            ImGui::TextDisabled("ESP is active");
            ImGui::Unindent(16.0f);
        }
    }

    ImGui::Separator();

    // Aimbot section
    if (ImGui::CollapsingHeader("Aimbot Settings")) {
        ImGui::Checkbox("Enable Aimbot", &aimbot_enabled);
        ImGui::Text("Target PID: 35028");
        
        if (aimbot_enabled) {
            ImGui::Indent(16.0f);
            
            static float fov = 15.0f;
            static float smoothness = 5.0f;
            static int bone = 0;
            const char* bones[] = { "Head", "Neck", "Chest", "Stomach" };
            
            ImGui::SliderFloat("FOV", &fov, 1.0f, 90.0f, "%.1f");
            ImGui::SliderFloat("Smoothness", &smoothness, 1.0f, 20.0f, "%.1f");
            ImGui::Combo("Target Bone", &bone, bones, IM_ARRAYSIZE(bones));
            
            ImGui::Unindent(16.0f);
        }
    }

    ImGui::Separator();

    // Footer with project info
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "DMA Handle: 0x021EE040");
    
    ImGui::Spacing();
    
    if (ImGui::Button("Save Settings", ImVec2(120.0f, 0.0f))) {
        // Save settings logic
        std::cout << "[PROJECT ZERO] Settings saved" << std::endl;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Load Settings", ImVec2(120.0f, 0.0f))) {
        // Load settings logic
        std::cout << "[PROJECT ZERO] Settings loaded" << std::endl;
    }

    ImGui::Separator();
    
    // FPS display
    ImGuiIO& io = ImGui::GetIO();
    ImGui::TextDisabled("FPS: %.0f | Press INSERT to toggle", io.Framerate);

    ImGui::End();
}

void ToggleZeroMenu()
{
    g_MenuVisible = !g_MenuVisible;
    std::cout << "[PROJECT ZERO] Menu " << (g_MenuVisible ? "Opened" : "Closed") << std::endl;
}

bool IsZeroMenuVisible()
{
    return g_MenuVisible;
}

void SetZeroMenuVisible(bool visible)
{
    g_MenuVisible = visible;
}
