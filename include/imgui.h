#pragma once

// Minimal ImGui header for compilation
struct ImVec2 { float x, y; };
struct ImVec4 { float x, y, z, w; };

// Dummy flags
typedef int ImGuiWindowFlags;
enum ImGuiWindowFlags_ {
    ImGuiWindowFlags_None = 0,
    ImGuiWindowFlags_NoTitleBar = 1 << 0,
    ImGuiWindowFlags_NoResize = 1 << 1,
    ImGuiWindowFlags_NoMove = 1 << 2,
    ImGuiWindowFlags_NoScrollbar = 1 << 3,
    ImGuiWindowFlags_NoScrollWithMouse = 1 << 4,
    ImGuiWindowFlags_NoCollapse = 1 << 5,
    ImGuiWindowFlags_AlwaysAutoResize = 1 << 6,
    ImGuiWindowFlags_NoBackground = 1 << 7,
    ImGuiWindowFlags_NoSavedSettings = 1 << 8,
    ImGuiWindowFlags_NoMouseInputs = 1 << 9,
    ImGuiWindowFlags_MenuBar = 1 << 10,
    ImGuiWindowFlags_HorizontalScrollbar = 1 << 11,
    ImGuiWindowFlags_NoFocusOnAppearing = 1 << 12,
    ImGuiWindowFlags_NoBringToFrontOnFocus = 1 << 13,
    ImGuiWindowFlags_AlwaysVerticalScrollbar= 1 << 14,
    ImGuiWindowFlags_AlwaysHorizontalScrollbar=1<< 15,
    ImGuiWindowFlags_AlwaysUseWindowPadding = 1 << 16,
    ImGuiWindowFlags_NoNavInputs = 1 << 18,
    ImGuiWindowFlags_NoNavFocus = 1 << 19,
    ImGuiWindowFlags_UnsavedDocument = 1 << 20,
    ImGuiWindowFlags_NoNav = ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
    ImGuiWindowFlags_NoDecoration = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse,
    ImGuiWindowFlags_NoInputs = ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
};

namespace ImGui {
    void CreateContext();
    void DestroyContext();
    void StyleColorsDark();
    void NewFrame();
    void Render();
    void Text(const char* fmt, ...);
    void TextColored(const ImVec4& col, const char* fmt, ...);
    void Checkbox(const char* label, bool* v);
    void SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", int flags = 0);
    bool Begin(const char* name, bool* p_open = 0, int flags = 0);
    void End();
    void SetNextWindowPos(const ImVec2& pos, int cond = 0, const ImVec2& pivot = {0,0});
    
    struct DrawData;
    DrawData* GetDrawData();

    struct IO {
        float DeltaTime;
        bool WantCaptureMouse;
        bool WantCaptureKeyboard;
    };
    IO& GetIO();
}
