#include "ZeroUI.h"

#include <imgui.h>

void ZeroUI::ApplyStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.92f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.20f, 0.80f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.16f, 0.90f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.10f, 0.66f, 0.92f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.10f, 0.66f, 0.92f, 0.85f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.10f, 0.66f, 0.92f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.14f, 0.14f, 0.16f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.14f, 0.14f, 0.16f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
}

void ZeroUI::Render(bool* menuOpen, ZeroSettings& settings)
{
    if (!menuOpen || !*menuOpen)
        return;

    ImGui::SetNextWindowSize(ImVec2(560, 360), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Zero Elite", menuOpen, flags))
    {
        ImGui::TextUnformatted("Overlay Menu (Toggle: INSERT)");
        ImGui::Separator();

        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Combat"))
            {
                ImGui::Checkbox("Aimbot", &settings.aimbotEnabled);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Visuals"))
            {
                ImGui::Checkbox("ESP", &settings.espEnabled);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings"))
            {
                ImGui::SliderInt("Refresh Rate (Hz)", &settings.refreshRateHz, 30, 240);
                ImGui::Text("Current: %d Hz", settings.refreshRateHz);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::TextDisabled("DMA Target PID: %u", 35028u);
        ImGui::TextDisabled("DMA Handle: 0x%llX", 0x021EE040ull);
    }
    ImGui::End();
}

