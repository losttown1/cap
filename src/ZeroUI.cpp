// PROJECT ZERO - Professional Menu UI
// Modern design with sidebar, toggles, and red accent
// Similar to professional DMA tools

#include "ZeroUI.hpp"
#include "DMA_Engine.hpp"
#include "../include/imgui.h"
#include <cstdio>
#include <cstring>

// ============================================================================
// Settings
// ============================================================================

// ESP
bool esp_enabled = true;
bool g_ESP_Box = true;
bool g_ESP_Skeleton = false;
bool g_ESP_Health = true;
bool g_ESP_Name = false;
bool g_ESP_Distance = true;
bool g_ESP_HeadCircle = true;
bool g_ESP_Snapline = false;
int g_ESP_BoxType = 1;

// Aimbot
bool aimbot_enabled = false;
float g_AimFOV = 120.0f;
float g_AimSmooth = 7.5f;
int g_AimBone = 0;
int g_AimKey = 0;  // 0=Right Click
int g_AimPriority = 0;  // 0=Closest to Crosshair
bool g_AimVisible = true;
bool g_IgnoreKnocked = false;
bool g_IgnoreTeam = true;
float g_AimMaxDist = 150.0f;

// Misc
bool g_Triggerbot = false;
bool g_NoFlash = true;
bool g_NoSmoke = true;
bool g_RadarHack = false;
bool g_Bhop = true;
bool g_MagicBullet = true;

// Radar
float g_RadarSize = 220.0f;
float g_RadarZoom = 1.5f;
float g_RadarX = 20.0f;
float g_RadarY = 20.0f;
bool g_ShowEnemies = true;
bool g_ShowTeam = false;
bool g_ShowDistance = true;
float box_color[4] = { 1.0f, 0.2f, 0.2f, 1.0f };
float refresh_rate = 144.0f;

// Menu state
static int g_CurrentTab = 0;  // 0=Aimbot, 1=ESP, 2=Misc, 3=Radar, 4=Settings

// ============================================================================
// Custom UI Helpers
// ============================================================================

// Colors
#define COL_BG          ImVec4(0.08f, 0.08f, 0.08f, 1.0f)
#define COL_SIDEBAR     ImVec4(0.06f, 0.06f, 0.06f, 1.0f)
#define COL_PANEL       ImVec4(0.10f, 0.10f, 0.10f, 1.0f)
#define COL_RED         ImVec4(0.90f, 0.20f, 0.20f, 1.0f)
#define COL_RED_DARK    ImVec4(0.70f, 0.15f, 0.15f, 1.0f)
#define COL_GRAY        ImVec4(0.25f, 0.25f, 0.25f, 1.0f)
#define COL_GRAY_LIGHT  ImVec4(0.35f, 0.35f, 0.35f, 1.0f)
#define COL_TEXT        ImVec4(0.90f, 0.90f, 0.90f, 1.0f)
#define COL_TEXT_DIM    ImVec4(0.50f, 0.50f, 0.50f, 1.0f)

static bool ToggleButton(const char* label, bool* v)
{
    ImGui::PushID(label);
    
    bool changed = false;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    float height = 22.0f;
    float width = 44.0f;
    float radius = height * 0.5f;
    
    // Invisible button for interaction
    if (ImGui::InvisibleButton("##toggle", ImVec2(width, height)))
    {
        *v = !*v;
        changed = true;
    }
    
    // Colors
    ImU32 colBg = *v ? IM_COL32(200, 50, 50, 255) : IM_COL32(60, 60, 60, 255);
    ImU32 colKnob = IM_COL32(255, 255, 255, 255);
    
    // Background pill
    draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), colBg, radius);
    
    // Knob
    float knobX = *v ? (p.x + width - radius) : (p.x + radius);
    draw->AddCircleFilled(ImVec2(knobX, p.y + radius), radius - 3.0f, colKnob);
    
    ImGui::PopID();
    return changed;
}

static void ToggleOption(const char* label, bool* v)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 12));
    
    ImGui::Text("%s", label);
    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
    ToggleButton(label, v);
    
    ImGui::PopStyleVar();
}

static void SliderOption(const char* label, float* v, float minVal, float maxVal, const char* format)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 12));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, COL_RED);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, COL_RED);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_GRAY);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, COL_GRAY_LIGHT);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, COL_GRAY_LIGHT);
    
    ImGui::Text("%s", label);
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    
    // Show value
    char valStr[32];
    snprintf(valStr, sizeof(valStr), format, *v);
    ImGui::PushStyleColor(ImGuiCol_Text, COL_RED);
    ImGui::Text("%s", valStr);
    ImGui::PopStyleColor();
    
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 30);
    
    char id[64];
    snprintf(id, sizeof(id), "##%s", label);
    ImGui::SliderFloat(id, v, minVal, maxVal, "");
    
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar();
}

static void ComboOption(const char* label, int* current, const char* const items[], int count)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 12));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_GRAY);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, COL_GRAY_LIGHT);
    ImGui::PushStyleColor(ImGuiCol_Button, COL_GRAY);
    ImGui::PushStyleColor(ImGuiCol_Header, COL_RED_DARK);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COL_RED);
    
    ImGui::Text("%s", label);
    
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 30);
    
    char id[64];
    snprintf(id, sizeof(id), "##%s", label);
    ImGui::Combo(id, current, items, count);
    
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar();
}

static void KeyOption(const char* label, int* key)
{
    const char* keyNames[] = { "RIGHT CLICK", "LEFT CLICK", "MIDDLE", "SHIFT", "CTRL" };
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 12));
    
    ImGui::Text("%s", label);
    ImGui::SameLine(ImGui::GetWindowWidth() - 120);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_RED);
    ImGui::PushStyleColor(ImGuiCol_Border, COL_RED);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    
    if (ImGui::Button(keyNames[*key], ImVec2(100, 24)))
    {
        *key = (*key + 1) % 5;
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
}

static void SectionHeader(const char* label)
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_DIM);
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

static void SidebarButton(const char* icon, int tabIndex, const char* tooltip)
{
    bool selected = (g_CurrentTab == tabIndex);
    
    ImGui::PushStyleColor(ImGuiCol_Button, selected ? COL_RED : ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? COL_RED : COL_GRAY);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, COL_RED);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    
    if (ImGui::Button(icon, ImVec2(40, 40)))
    {
        g_CurrentTab = tabIndex;
    }
    
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", tooltip);
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
}

// ============================================================================
// Initialization
// ============================================================================

void InitializeZeroUI()
{
    ImGuiStyle& s = ImGui::GetStyle();
    
    s.WindowRounding = 10.0f;
    s.ChildRounding = 8.0f;
    s.FrameRounding = 6.0f;
    s.PopupRounding = 6.0f;
    s.ScrollbarRounding = 6.0f;
    s.GrabRounding = 6.0f;
    s.TabRounding = 6.0f;
    
    s.WindowPadding = ImVec2(0, 0);
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(10, 8);
    s.ItemInnerSpacing = ImVec2(8, 4);
    s.ScrollbarSize = 10.0f;
    s.GrabMinSize = 10.0f;
    
    s.WindowBorderSize = 0.0f;
    s.ChildBorderSize = 0.0f;
    s.FrameBorderSize = 0.0f;
    s.PopupBorderSize = 0.0f;
    
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = COL_BG;
    c[ImGuiCol_ChildBg] = COL_PANEL;
    c[ImGuiCol_PopupBg] = COL_PANEL;
    c[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    c[ImGuiCol_FrameBg] = COL_GRAY;
    c[ImGuiCol_FrameBgHovered] = COL_GRAY_LIGHT;
    c[ImGuiCol_FrameBgActive] = COL_GRAY_LIGHT;
    c[ImGuiCol_TitleBg] = COL_BG;
    c[ImGuiCol_TitleBgActive] = COL_BG;
    c[ImGuiCol_ScrollbarBg] = COL_BG;
    c[ImGuiCol_ScrollbarGrab] = COL_GRAY;
    c[ImGuiCol_ScrollbarGrabHovered] = COL_GRAY_LIGHT;
    c[ImGuiCol_ScrollbarGrabActive] = COL_RED;
    c[ImGuiCol_CheckMark] = COL_RED;
    c[ImGuiCol_SliderGrab] = COL_RED;
    c[ImGuiCol_SliderGrabActive] = COL_RED;
    c[ImGuiCol_Button] = COL_GRAY;
    c[ImGuiCol_ButtonHovered] = COL_RED_DARK;
    c[ImGuiCol_ButtonActive] = COL_RED;
    c[ImGuiCol_Header] = COL_RED_DARK;
    c[ImGuiCol_HeaderHovered] = COL_RED;
    c[ImGuiCol_HeaderActive] = COL_RED;
    c[ImGuiCol_Text] = COL_TEXT;
    c[ImGuiCol_TextDisabled] = COL_TEXT_DIM;
}

void ShutdownZeroUI() {}

// ============================================================================
// Main Render
// ============================================================================

void RenderZeroMenu()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Window size
    float winW = 750.0f;
    float winH = 500.0f;
    
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - winW) * 0.5f, (io.DisplaySize.y - winH) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (!ImGui::Begin("##ProjectZero", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    
    ImGui::PopStyleVar();
    
    // ==================== SIDEBAR ====================
    ImGui::PushStyleColor(ImGuiCol_ChildBg, COL_SIDEBAR);
    ImGui::BeginChild("##sidebar", ImVec2(60, 0), false);
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    // Logo
    ImGui::PushStyleColor(ImGuiCol_Button, COL_RED);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::SetCursorPosX(10);
    ImGui::Button("Z", ImVec2(40, 40));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    
    // Navigation buttons
    ImGui::SetCursorPosX(10);
    SidebarButton("+", 0, "Aimbot");
    ImGui::Spacing();
    
    ImGui::SetCursorPosX(10);
    SidebarButton("O", 1, "ESP");
    ImGui::Spacing();
    
    ImGui::SetCursorPosX(10);
    SidebarButton("*", 2, "Misc");
    ImGui::Spacing();
    
    ImGui::SetCursorPosX(10);
    SidebarButton("#", 3, "Radar");
    ImGui::Spacing();
    
    ImGui::SetCursorPosX(10);
    SidebarButton("=", 4, "Settings");
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // ==================== MAIN CONTENT ====================
    ImGui::BeginChild("##content", ImVec2(0, 0), false);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    
    // Tab title
    const char* tabTitles[] = { "AIMBOT", "VISUALS", "MISC", "RADAR", "SETTINGS" };
    
    ImGui::Spacing();
    ImGui::PushFont(nullptr);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::SetCursorPosX(20);
    ImGui::Text("%s", tabTitles[g_CurrentTab]);
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::Spacing();
    
    // Content area
    ImGui::SetCursorPosX(15);
    ImGui::BeginChild("##innercontent", ImVec2(0, 0), false);
    
    // ==================== AIMBOT TAB ====================
    if (g_CurrentTab == 0)
    {
        ImGui::BeginChild("##left", ImVec2(340, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        ToggleOption("Enable Aimbot", &aimbot_enabled);
        ImGui::Spacing();
        
        KeyOption("Aim Key", &g_AimKey);
        ImGui::Spacing();
        
        SliderOption("Field of View", &g_AimFOV, 1.0f, 180.0f, "%.0f°");
        SliderOption("Smoothness", &g_AimSmooth, 1.0f, 20.0f, "%.1f");
        
        SectionHeader("Target Priority");
        const char* priorities[] = { "Closest to Crosshair", "Closest Distance", "Lowest Health" };
        ComboOption("", &g_AimPriority, priorities, 3);
        
        SectionHeader("Target Hitbox");
        const char* hitboxes[] = { "Head", "Neck", "Chest", "Pelvis" };
        ComboOption("", &g_AimBone, hitboxes, 4);
        
        ImGui::Spacing();
        ToggleOption("Ignore Knocked", &g_IgnoreKnocked);
        ToggleOption("Ignore Teammates", &g_IgnoreTeam);
        ToggleOption("Visibility Check", &g_AimVisible);
        
        SliderOption("Max Distance", &g_AimMaxDist, 10.0f, 500.0f, "%.0fm");
        
        ImGui::PopStyleVar();
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Right panel
        ImGui::BeginChild("##right", ImVec2(0, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        ToggleOption("Player ESP", &esp_enabled);
        ToggleOption("Box ESP", &g_ESP_Box);
        ToggleOption("Name Tags", &g_ESP_Name);
        ToggleOption("Health Bars", &g_ESP_Health);
        ToggleOption("Distance Info", &g_ESP_Distance);
        ToggleOption("Skeleton ESP", &g_ESP_Skeleton);
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        ToggleOption("Triggerbot", &g_Triggerbot);
        ToggleOption("No Flash", &g_NoFlash);
        ToggleOption("No Smoke", &g_NoSmoke);
        ToggleOption("Radar Hack", &g_RadarHack);
        ToggleOption("Bhop", &g_Bhop);
        ToggleOption("Magic Bullet", &g_MagicBullet);
        
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    
    // ==================== ESP TAB ====================
    else if (g_CurrentTab == 1)
    {
        ImGui::BeginChild("##left", ImVec2(340, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        ToggleOption("Enable ESP", &esp_enabled);
        ImGui::Spacing();
        
        SectionHeader("Box Settings");
        ToggleOption("Box ESP", &g_ESP_Box);
        
        const char* boxTypes[] = { "2D Box", "Corner Box", "3D Box" };
        ComboOption("Box Type", &g_ESP_BoxType, boxTypes, 3);
        
        SectionHeader("Player Info");
        ToggleOption("Name Tags", &g_ESP_Name);
        ToggleOption("Health Bars", &g_ESP_Health);
        ToggleOption("Distance", &g_ESP_Distance);
        ToggleOption("Skeleton", &g_ESP_Skeleton);
        ToggleOption("Head Circle", &g_ESP_HeadCircle);
        ToggleOption("Snaplines", &g_ESP_Snapline);
        
        ImGui::PopStyleVar();
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        ImGui::BeginChild("##right", ImVec2(0, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        SectionHeader("Filters");
        ToggleOption("Show Enemies", &g_ShowEnemies);
        ToggleOption("Show Teammates", &g_ShowTeam);
        
        SectionHeader("Colors");
        ImGui::Text("Enemy Color");
        ImGui::SetNextItemWidth(200);
        ImGui::ColorEdit4("##enemycol", box_color);
        
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    
    // ==================== MISC TAB ====================
    else if (g_CurrentTab == 2)
    {
        ImGui::BeginChild("##left", ImVec2(340, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        SectionHeader("Combat");
        ToggleOption("Triggerbot", &g_Triggerbot);
        ToggleOption("Magic Bullet", &g_MagicBullet);
        
        SectionHeader("Movement");
        ToggleOption("Bhop", &g_Bhop);
        
        SectionHeader("Visual");
        ToggleOption("No Flash", &g_NoFlash);
        ToggleOption("No Smoke", &g_NoSmoke);
        ToggleOption("Radar Hack", &g_RadarHack);
        
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    
    // ==================== RADAR TAB ====================
    else if (g_CurrentTab == 3)
    {
        ImGui::BeginChild("##left", ImVec2(340, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        SectionHeader("Radar Settings");
        SliderOption("Size", &g_RadarSize, 100.0f, 400.0f, "%.0f px");
        SliderOption("Zoom", &g_RadarZoom, 0.5f, 5.0f, "%.1fx");
        SliderOption("Position X", &g_RadarX, 0.0f, 500.0f, "%.0f");
        SliderOption("Position Y", &g_RadarY, 0.0f, 500.0f, "%.0f");
        
        ImGui::Spacing();
        const auto& players = GetPlayerList();
        ImGui::Text("Players Detected: %d", (int)players.size());
        
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    
    // ==================== SETTINGS TAB ====================
    else if (g_CurrentTab == 4)
    {
        ImGui::BeginChild("##left", ImVec2(340, 0), false);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        
        SectionHeader("Connection");
        ImGui::Text("Target: %s", TARGET_PROCESS_NAME);
        ImGui::Text("Status: %s", IsConnected() ? "Connected" : "Simulation");
        
        SectionHeader("Hotkeys");
        ImGui::BulletText("F1 - Toggle Overlay");
        ImGui::BulletText("INSERT - Toggle Menu");
        ImGui::BulletText("END - Exit");
        
        SectionHeader("Performance");
        SliderOption("Refresh Rate", &refresh_rate, 30.0f, 240.0f, "%.0f Hz");
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_Text, COL_RED);
        ImGui::Text("PROJECT ZERO v2.0");
        ImGui::PopStyleColor();
        ImGui::Text("Professional DMA ESP for BO6");
        
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    
    ImGui::End();
}

void ToggleZeroMenu() {}
bool IsZeroMenuVisible() { return true; }
void SetZeroMenuVisible(bool) {}
