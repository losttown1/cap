// ZeroUI.cpp - PROJECT ZERO User Interface Implementation
// Black Ops 6 Radar Menu with ImGui

#include "ZeroUI.hpp"
#include "DMA_Engine.hpp"
#include <imgui.h>
#include <iostream>

// Default settings for Zero system
bool esp_enabled = true;
bool aimbot_enabled = false;
float refresh_rate = 144.0f;
float box_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // Default red

// Radar settings
static float radar_zoom = 1.0f;
static bool show_enemies = true;
static bool show_teammates = false;
static bool show_names = false;
static float radar_size = 200.0f;

// Menu visibility state
static bool g_MenuVisible = false;

// Initialize UI
void InitializeZeroUI()
{
    // Apply custom style
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Modern style settings
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

    // Dark theme with green accent
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
    colors[ImGuiCol_CheckMark]              = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
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

    // Set window style
    ImGui::SetNextWindowSize(ImVec2(420.0f, 450.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("PROJECT ZERO | BO6 RADAR", nullptr, ImGuiWindowFlags_NoCollapse);

    // Connection status
    if (IsConnected()) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: CONNECTED");
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: DISCONNECTED");
    }
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 120);
    ImGui::TextDisabled("Press INSERT");
    
    ImGui::Separator();

    // Tab bar
    if (ImGui::BeginTabBar("MainTabs"))
    {
        // Radar Tab
        if (ImGui::BeginTabItem("Radar"))
        {
            ImGui::Spacing();
            
            ImGui::Text("Radar Settings");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Enable Radar/ESP", &esp_enabled);
            
            if (esp_enabled) {
                ImGui::Indent(16.0f);
                
                ImGui::Checkbox("Show Enemies", &show_enemies);
                ImGui::Checkbox("Show Teammates", &show_teammates);
                ImGui::Checkbox("Show Names", &show_names);
                
                ImGui::Spacing();
                
                ImGui::SliderFloat("Radar Size", &radar_size, 100.0f, 400.0f, "%.0f px");
                ImGui::SliderFloat("Radar Zoom", &radar_zoom, 0.5f, 5.0f, "%.1fx");
                
                ImGui::Spacing();
                ImGui::ColorEdit4("Enemy Color", box_color);
                
                ImGui::Unindent(16.0f);
            }

            ImGui::Spacing();
            ImGui::Separator();
            
            // Player list info
            const auto& players = GetPlayerList();
            ImGui::Text("Detected Players: %zu", players.size());
            
            if (ImGui::TreeNode("Player List"))
            {
                for (size_t i = 0; i < players.size() && i < 20; i++) {
                    const auto& p = players[i];
                    if (!p.valid) continue;
                    
                    ImVec4 color = p.isEnemy ? ImVec4(1, 0.3f, 0.3f, 1) : ImVec4(0.3f, 0.3f, 1, 1);
                    ImGui::TextColored(color, "%s - HP: %d - Dist: %.0fm", 
                        p.name[0] ? p.name : "Unknown",
                        p.health,
                        p.distance
                    );
                }
                ImGui::TreePop();
            }
            
            ImGui::EndTabItem();
        }

        // Aimbot Tab
        if (ImGui::BeginTabItem("Aimbot"))
        {
            ImGui::Spacing();
            
            ImGui::Text("Aimbot Settings");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Enable Aimbot", &aimbot_enabled);
            
            if (aimbot_enabled) {
                ImGui::Indent(16.0f);
                
                static float fov = 15.0f;
                static float smoothness = 5.0f;
                static int bone = 0;
                const char* bones[] = { "Head", "Neck", "Chest", "Stomach" };
                
                ImGui::SliderFloat("FOV", &fov, 1.0f, 90.0f, "%.1f");
                ImGui::SliderFloat("Smoothness", &smoothness, 1.0f, 20.0f, "%.1f");
                ImGui::Combo("Target Bone", &bone, bones, IM_ARRAYSIZE(bones));
                
                ImGui::Spacing();
                
                static bool visibleOnly = true;
                static bool prediction = true;
                ImGui::Checkbox("Visible Only", &visibleOnly);
                ImGui::Checkbox("Prediction", &prediction);
                
                ImGui::Unindent(16.0f);
            }
            
            ImGui::EndTabItem();
        }

        // Settings Tab
        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::Spacing();
            
            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SliderFloat("Refresh Rate", &refresh_rate, 30.0f, 240.0f, "%.0f Hz");
            ImGui::TextDisabled("Higher values = smoother but more CPU usage");

            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::Text("DMA Connection");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Target: %s", TARGET_PROCESS_NAME);
            
            if (IsConnected()) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "DMA: Connected");
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "DMA: Disconnected");
                
                if (ImGui::Button("Reconnect DMA", ImVec2(120, 0))) {
                    ShutdownZeroDMA();
                    InitializeZeroDMA();
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::Text("Hotkeys");
            ImGui::Separator();
            ImGui::TextDisabled("INSERT - Toggle Menu");
            ImGui::TextDisabled("END    - Exit Overlay");
            
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    
    // Footer
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("FPS: %.0f", io.Framerate);
    ImGui::SameLine(ImGui::GetWindowWidth() - 180);
    ImGui::TextDisabled("PROJECT ZERO v1.0");

    ImGui::End();
}

void ToggleZeroMenu()
{
    g_MenuVisible = !g_MenuVisible;
}

bool IsZeroMenuVisible()
{
    return g_MenuVisible;
}

void SetZeroMenuVisible(bool visible)
{
    g_MenuVisible = visible;
}
