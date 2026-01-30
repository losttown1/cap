// ZeroUI.cpp - Zero Elite User Interface Implementation
// Professional ImGui menu with ESP, Aimbot toggles, and Refresh Rate slider

#include "ZeroUI.h"
#include "DMA_Engine.h"
#include <iostream>

namespace ZeroElite {

    // Static member initialization
    bool ZeroUI::s_bMenuVisible = false;
    bool ZeroUI::s_bESPEnabled = false;
    bool ZeroUI::s_bAimbotEnabled = false;
    int ZeroUI::s_iRefreshRate = 60;
    ImVec4 ZeroUI::s_AccentColor = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    ImVec4 ZeroUI::s_BackgroundColor = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);

    void ZeroUI::Initialize()
    {
        ApplyCustomStyle();
        std::cout << "[Zero Elite] UI Initialized" << std::endl;
    }

    void ZeroUI::Shutdown()
    {
        std::cout << "[Zero Elite] UI Shutdown" << std::endl;
    }

    void ZeroUI::ApplyCustomStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Main style settings
        style.WindowRounding = 8.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;

        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.FramePadding = ImVec2(8.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 8.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
        style.IndentSpacing = 20.0f;
        style.ScrollbarSize = 12.0f;
        style.GrabMinSize = 10.0f;

        // Color scheme - Dark theme with blue accent
        colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = s_BackgroundColor;
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
        colors[ImGuiCol_CheckMark]              = s_AccentColor;
        colors[ImGuiCol_SliderGrab]             = s_AccentColor;
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.36f, 0.69f, 1.00f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(s_AccentColor.x, s_AccentColor.y, s_AccentColor.z, 0.70f);
        colors[ImGuiCol_ButtonActive]           = s_AccentColor;
        colors[ImGuiCol_Header]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(s_AccentColor.x, s_AccentColor.y, s_AccentColor.z, 0.70f);
        colors[ImGuiCol_HeaderActive]           = s_AccentColor;
        colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]       = s_AccentColor;
        colors[ImGuiCol_SeparatorActive]        = s_AccentColor;
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = s_AccentColor;
        colors[ImGuiCol_ResizeGripActive]       = s_AccentColor;
        colors[ImGuiCol_Tab]                    = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TabHovered]             = s_AccentColor;
        colors[ImGuiCol_TabActive]              = ImVec4(s_AccentColor.x * 0.8f, s_AccentColor.y * 0.8f, s_AccentColor.z * 0.8f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = s_AccentColor;
        colors[ImGuiCol_PlotHistogram]          = s_AccentColor;
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.36f, 0.69f, 1.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(s_AccentColor.x, s_AccentColor.y, s_AccentColor.z, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = s_AccentColor;
        colors[ImGuiCol_NavHighlight]           = s_AccentColor;
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    void ZeroUI::Render()
    {
        if (!s_bMenuVisible)
        {
            return;
        }

        // Set window properties
        ImGui::SetNextWindowSize(ImVec2(400.0f, 350.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);

        // Begin main window
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
        
        if (ImGui::Begin("Zero Elite | Professional Overlay", &s_bMenuVisible, windowFlags))
        {
            // Header
            RenderHeader();
            
            ImGui::Separator();
            ImGui::Spacing();

            // Tab bar for organized sections
            if (ImGui::BeginTabBar("MainTabBar"))
            {
                // Visuals Tab
                if (ImGui::BeginTabItem("Visuals"))
                {
                    RenderVisualsTab();
                    ImGui::EndTabItem();
                }

                // Aimbot Tab
                if (ImGui::BeginTabItem("Aimbot"))
                {
                    RenderAimbotTab();
                    ImGui::EndTabItem();
                }

                // Settings Tab
                if (ImGui::BeginTabItem("Settings"))
                {
                    RenderSettingsTab();
                    ImGui::EndTabItem();
                }

                // DMA Info Tab
                if (ImGui::BeginTabItem("DMA Info"))
                {
                    RenderDMAInfoTab();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            // Footer
            ImGui::Spacing();
            ImGui::Separator();
            RenderFooter();
        }
        ImGui::End();
    }

    void ZeroUI::RenderHeader()
    {
        ImGui::PushStyleColor(ImGuiCol_Text, s_AccentColor);
        ImGui::Text("ZERO ELITE");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("v1.0.0");
        ImGui::SameLine(ImGui::GetWindowWidth() - 120.0f);
        ImGui::TextDisabled("Press INSERT to toggle");
    }

    void ZeroUI::RenderVisualsTab()
    {
        ImGui::Spacing();
        
        // ESP Section
        ImGui::Text("ESP Options");
        ImGui::Separator();
        ImGui::Spacing();

        // ESP Toggle with custom styling
        if (ImGui::Checkbox("Enable ESP", &s_bESPEnabled))
        {
            std::cout << "[Zero Elite] ESP " << (s_bESPEnabled ? "Enabled" : "Disabled") << std::endl;
        }
        
        if (s_bESPEnabled)
        {
            ImGui::Indent(16.0f);
            
            static bool bShowBox = true;
            static bool bShowName = true;
            static bool bShowHealth = true;
            static bool bShowDistance = false;
            static bool bShowSkeleton = false;
            
            ImGui::Checkbox("Box ESP", &bShowBox);
            ImGui::Checkbox("Name ESP", &bShowName);
            ImGui::Checkbox("Health ESP", &bShowHealth);
            ImGui::Checkbox("Distance ESP", &bShowDistance);
            ImGui::Checkbox("Skeleton ESP", &bShowSkeleton);
            
            ImGui::Spacing();
            
            static float espDistance = 300.0f;
            ImGui::SliderFloat("ESP Distance", &espDistance, 50.0f, 1000.0f, "%.0f m");
            
            ImGui::Unindent(16.0f);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Radar Section
        ImGui::Text("Radar Options");
        ImGui::Separator();
        ImGui::Spacing();

        static bool bRadarEnabled = false;
        ImGui::Checkbox("Enable Radar", &bRadarEnabled);
        
        if (bRadarEnabled)
        {
            ImGui::Indent(16.0f);
            
            static float radarScale = 1.0f;
            ImGui::SliderFloat("Radar Scale", &radarScale, 0.5f, 3.0f, "%.1fx");
            
            static float radarOpacity = 0.8f;
            ImGui::SliderFloat("Radar Opacity", &radarOpacity, 0.1f, 1.0f, "%.1f");
            
            ImGui::Unindent(16.0f);
        }
    }

    void ZeroUI::RenderAimbotTab()
    {
        ImGui::Spacing();

        // Aimbot Section
        ImGui::Text("Aimbot Options");
        ImGui::Separator();
        ImGui::Spacing();

        // Aimbot Toggle
        if (ImGui::Checkbox("Enable Aimbot", &s_bAimbotEnabled))
        {
            std::cout << "[Zero Elite] Aimbot " << (s_bAimbotEnabled ? "Enabled" : "Disabled") << std::endl;
        }

        if (s_bAimbotEnabled)
        {
            ImGui::Indent(16.0f);
            ImGui::Spacing();

            // Aimbot Settings
            static float fov = 15.0f;
            ImGui::SliderFloat("FOV", &fov, 1.0f, 90.0f, "%.1f");

            static float smoothness = 5.0f;
            ImGui::SliderFloat("Smoothness", &smoothness, 1.0f, 20.0f, "%.1f");

            static int bone = 0;
            const char* bones[] = { "Head", "Neck", "Chest", "Stomach" };
            ImGui::Combo("Target Bone", &bone, bones, IM_ARRAYSIZE(bones));

            ImGui::Spacing();
            
            static bool bVisibleOnly = true;
            ImGui::Checkbox("Visible Only", &bVisibleOnly);
            
            static bool bPrediction = true;
            ImGui::Checkbox("Prediction", &bPrediction);
            
            static bool bRecoilControl = false;
            ImGui::Checkbox("Recoil Control", &bRecoilControl);

            ImGui::Unindent(16.0f);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Triggerbot Section
        ImGui::Text("Triggerbot Options");
        ImGui::Separator();
        ImGui::Spacing();

        static bool bTriggerbotEnabled = false;
        ImGui::Checkbox("Enable Triggerbot", &bTriggerbotEnabled);

        if (bTriggerbotEnabled)
        {
            ImGui::Indent(16.0f);
            
            static int delay = 50;
            ImGui::SliderInt("Delay (ms)", &delay, 0, 200);
            
            ImGui::Unindent(16.0f);
        }
    }

    void ZeroUI::RenderSettingsTab()
    {
        ImGui::Spacing();

        // Performance Section
        ImGui::Text("Performance");
        ImGui::Separator();
        ImGui::Spacing();

        // Refresh Rate Slider
        if (ImGui::SliderInt("Refresh Rate", &s_iRefreshRate, 30, 240, "%d Hz"))
        {
            std::cout << "[Zero Elite] Refresh Rate set to " << s_iRefreshRate << " Hz" << std::endl;
        }
        ImGui::TextDisabled("Higher values may impact performance");

        ImGui::Spacing();
        ImGui::Spacing();

        // Theme Section
        ImGui::Text("Theme");
        ImGui::Separator();
        ImGui::Spacing();

        static int themeIndex = 0;
        const char* themes[] = { "Dark Blue", "Dark Red", "Dark Green", "Dark Purple" };
        if (ImGui::Combo("Color Theme", &themeIndex, themes, IM_ARRAYSIZE(themes)))
        {
            switch (themeIndex)
            {
                case 0: s_AccentColor = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); break; // Blue
                case 1: s_AccentColor = ImVec4(0.98f, 0.26f, 0.26f, 1.00f); break; // Red
                case 2: s_AccentColor = ImVec4(0.26f, 0.98f, 0.36f, 1.00f); break; // Green
                case 3: s_AccentColor = ImVec4(0.69f, 0.26f, 0.98f, 1.00f); break; // Purple
            }
            ApplyCustomStyle();
        }

        static float menuOpacity = 0.94f;
        if (ImGui::SliderFloat("Menu Opacity", &menuOpacity, 0.5f, 1.0f, "%.2f"))
        {
            s_BackgroundColor.w = menuOpacity;
            ApplyCustomStyle();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Keybinds Section
        ImGui::Text("Keybinds");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("Menu Toggle: INSERT");
        ImGui::TextDisabled("ESP Toggle: F1");
        ImGui::TextDisabled("Aimbot Toggle: F2");
    }

    void ZeroUI::RenderDMAInfoTab()
    {
        ImGui::Spacing();

        // DMA Status Section
        ImGui::Text("DMA Status");
        ImGui::Separator();
        ImGui::Spacing();

        bool isInitialized = DMAEngine::IsInitialized();
        
        ImGui::Text("Status:");
        ImGui::SameLine();
        if (isInitialized)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::Text("Connected");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("Disconnected");
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // Target Information
        ImGui::Text("Target Information");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("PID: %u", TARGET_PID);
        ImGui::Text("Handle: 0x%08llX", TARGET_HANDLE);

        ImGui::Spacing();
        ImGui::Spacing();

        // Actions
        if (ImGui::Button("Initialize DMA", ImVec2(120.0f, 0.0f)))
        {
            if (DMAEngine::Initialize())
            {
                std::cout << "[Zero Elite] DMA Initialized successfully" << std::endl;
            }
            else
            {
                std::cout << "[Zero Elite] DMA Initialization failed" << std::endl;
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Shutdown DMA", ImVec2(120.0f, 0.0f)))
        {
            DMAEngine::Shutdown();
        }
    }

    void ZeroUI::RenderFooter()
    {
        ImGui::TextDisabled("Zero Elite - Professional Game Overlay");
        ImGui::SameLine(ImGui::GetWindowWidth() - 100.0f);
        
        ImGuiIO& io = ImGui::GetIO();
        ImGui::TextDisabled("FPS: %.0f", io.Framerate);
    }

    void ZeroUI::ToggleMenu()
    {
        s_bMenuVisible = !s_bMenuVisible;
        std::cout << "[Zero Elite] Menu " << (s_bMenuVisible ? "Opened" : "Closed") << std::endl;
    }

    bool ZeroUI::IsMenuVisible()
    {
        return s_bMenuVisible;
    }

    void ZeroUI::SetMenuVisible(bool visible)
    {
        s_bMenuVisible = visible;
    }

    bool ZeroUI::IsESPEnabled()
    {
        return s_bESPEnabled;
    }

    bool ZeroUI::IsAimbotEnabled()
    {
        return s_bAimbotEnabled;
    }

    int ZeroUI::GetRefreshRate()
    {
        return s_iRefreshRate;
    }

} // namespace ZeroElite
