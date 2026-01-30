// ZeroUI.h - Zero Elite User Interface Header
// Professional ImGui menu interface

#pragma once

#include "../include/imgui.h"

namespace ZeroElite {

    class ZeroUI
    {
    public:
        // Initialization
        static void Initialize();
        static void Shutdown();
        
        // Rendering
        static void Render();
        
        // Menu control
        static void ToggleMenu();
        static bool IsMenuVisible();
        static void SetMenuVisible(bool visible);
        
        // Feature state getters
        static bool IsESPEnabled();
        static bool IsAimbotEnabled();
        static int GetRefreshRate();
        
    private:
        // Internal render methods
        static void ApplyCustomStyle();
        static void RenderHeader();
        static void RenderVisualsTab();
        static void RenderAimbotTab();
        static void RenderSettingsTab();
        static void RenderDMAInfoTab();
        static void RenderFooter();
        
        // State variables
        static bool s_bMenuVisible;
        static bool s_bESPEnabled;
        static bool s_bAimbotEnabled;
        static int s_iRefreshRate;
        
        // Theme colors
        static ImVec4 s_AccentColor;
        static ImVec4 s_BackgroundColor;
    };

} // namespace ZeroElite
