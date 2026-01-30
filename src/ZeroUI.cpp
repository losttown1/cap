// ZeroUI.cpp - PROJECT ZERO User Interface Implementation
// Modern ImGui Menu with ESP, Aimbot, and DMA Settings

#include "ZeroUI.hpp"
#include <imgui.h>
#include <iostream>

// إعدادات افتراضية لنظام Zero
bool esp_enabled = true;
bool aimbot_enabled = false;
float refresh_rate = 144.0f;
float box_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // أحمر افتراضي

// حالة ظهور القائمة
static bool g_MenuVisible = false;

// تهيئة واجهة المستخدم
void InitializeZeroUI()
{
    // تطبيق الستايل المخصص
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // إعدادات الشكل العصري
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

    // ألوان السمة الداكنة مع اللون الأخضر
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

// إغلاق واجهة المستخدم
void ShutdownZeroUI()
{
    std::cout << "[PROJECT ZERO] UI Shutdown" << std::endl;
}

// رسم القائمة الرئيسية
void RenderZeroMenu() 
{
    if (!g_MenuVisible)
        return;

    // تعيين شكل القائمة لتكون عصرية
    ImGui::SetNextWindowSize(ImVec2(400.0f, 350.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("PROJECT ZERO | PREVIEW", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("System Status: ONLINE");
    ImGui::Separator();

    // قسم الـ Visuals (الرسم)
    if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable ESP Boxes", &esp_enabled);
        ImGui::ColorEdit4("Box Color", box_color);
        ImGui::SliderFloat("Refresh Rate", &refresh_rate, 30.0f, 240.0f, "%.0f Hz");
    }

    ImGui::Separator();

    // قسم الـ Aimbot
    if (ImGui::CollapsingHeader("Aimbot Settings")) {
        ImGui::Checkbox("Enable Aimbot", &aimbot_enabled);
        ImGui::Text("Target PID: 35028"); // الـ PID الخاص بك
        
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

    // تذييل القائمة بمعلومات المشروع
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "DMA Handle: 0x021EE040");
    
    ImGui::Spacing();
    
    if (ImGui::Button("Save Settings", ImVec2(120.0f, 0.0f))) {
        // منطق حفظ الإعدادات مستقبلاً
        std::cout << "[PROJECT ZERO] Settings Saved" << std::endl;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Load Settings", ImVec2(120.0f, 0.0f))) {
        // منطق تحميل الإعدادات
        std::cout << "[PROJECT ZERO] Settings Loaded" << std::endl;
    }

    ImGui::Separator();
    
    // عرض FPS ورسالة التحكم
    ImGuiIO& io = ImGui::GetIO();
    ImGui::TextDisabled("FPS: %.0f | Press INSERT to toggle", io.Framerate);

    ImGui::End();
}

// تبديل ظهور القائمة
void ToggleZeroMenu()
{
    g_MenuVisible = !g_MenuVisible;
    std::cout << "[PROJECT ZERO] Menu " << (g_MenuVisible ? "Opened" : "Closed") << std::endl;
}

// التحقق من ظهور القائمة
bool IsZeroMenuVisible()
{
    return g_MenuVisible;
}

// تعيين حالة ظهور القائمة
void SetZeroMenuVisible(bool visible)
{
    g_MenuVisible = visible;
}
