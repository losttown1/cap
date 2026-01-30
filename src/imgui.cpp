// dear imgui, v1.89.9
// (main code)
// Zero Elite - ImGui Core Implementation

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "../include/imgui.h"
#include "../include/imgui_internal.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef _MSC_VER
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4996)  // deprecated functions
#endif

//-----------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS
//-----------------------------------------------------------------------------

// Forward declarations for ImGui implementations
// Note: Full ImGui implementation would be thousands of lines
// This is a skeleton that links with the full ImGui library

//-----------------------------------------------------------------------------
// [SECTION] CONTEXT AND MEMORY ALLOCATORS
//-----------------------------------------------------------------------------

static ImGuiContext* GImGui = nullptr;

// Memory allocators
static void* ImGuiMemAlloc(size_t size, void* user_data)
{
    (void)user_data;
    return malloc(size);
}

static void ImGuiMemFree(void* ptr, void* user_data)
{
    (void)user_data;
    free(ptr);
}

static ImGuiMemAllocFunc GImAllocatorAllocFunc = ImGuiMemAlloc;
static ImGuiMemFreeFunc GImAllocatorFreeFunc = ImGuiMemFree;
static void* GImAllocatorUserData = nullptr;

//-----------------------------------------------------------------------------
// [SECTION] ImGuiStyle
//-----------------------------------------------------------------------------

ImGuiStyle::ImGuiStyle()
{
    Alpha                   = 1.0f;
    DisabledAlpha           = 0.60f;
    WindowPadding           = ImVec2(8, 8);
    WindowRounding          = 0.0f;
    WindowBorderSize        = 1.0f;
    WindowMinSize           = ImVec2(32, 32);
    WindowTitleAlign        = ImVec2(0.0f, 0.5f);
    WindowMenuButtonPosition = 0;
    ChildRounding           = 0.0f;
    ChildBorderSize         = 1.0f;
    PopupRounding           = 0.0f;
    PopupBorderSize         = 1.0f;
    FramePadding            = ImVec2(4, 3);
    FrameRounding           = 0.0f;
    FrameBorderSize         = 0.0f;
    ItemSpacing             = ImVec2(8, 4);
    ItemInnerSpacing        = ImVec2(4, 4);
    CellPadding             = ImVec2(4, 2);
    TouchExtraPadding       = ImVec2(0, 0);
    IndentSpacing           = 21.0f;
    ColumnsMinSpacing       = 6.0f;
    ScrollbarSize           = 14.0f;
    ScrollbarRounding       = 9.0f;
    GrabMinSize             = 10.0f;
    GrabRounding            = 0.0f;
    LogSliderDeadzone       = 4.0f;
    TabRounding             = 4.0f;
    TabBorderSize           = 0.0f;
    TabMinWidthForCloseButton = 0.0f;
    ColorButtonPosition     = 1;
    ButtonTextAlign         = ImVec2(0.5f, 0.5f);
    SelectableTextAlign     = ImVec2(0.0f, 0.0f);
    DisplayWindowPadding    = ImVec2(19, 19);
    DisplaySafeAreaPadding  = ImVec2(3, 3);
    MouseCursorScale        = 1.0f;
    AntiAliasedLines        = true;
    AntiAliasedLinesUseTex  = true;
    AntiAliasedFill         = true;
    CurveTessellationTol    = 1.25f;
    CircleTessellationMaxError = 0.30f;

    // Setup dark colors by default
    ImGui::StyleColorsDark(this);
}

void ImGuiStyle::ScaleAllSizes(float scale_factor)
{
    WindowPadding = ImVec2((int)(WindowPadding.x * scale_factor), (int)(WindowPadding.y * scale_factor));
    WindowRounding = (int)(WindowRounding * scale_factor);
    WindowMinSize = ImVec2((int)(WindowMinSize.x * scale_factor), (int)(WindowMinSize.y * scale_factor));
    ChildRounding = (int)(ChildRounding * scale_factor);
    PopupRounding = (int)(PopupRounding * scale_factor);
    FramePadding = ImVec2((int)(FramePadding.x * scale_factor), (int)(FramePadding.y * scale_factor));
    FrameRounding = (int)(FrameRounding * scale_factor);
    ItemSpacing = ImVec2((int)(ItemSpacing.x * scale_factor), (int)(ItemSpacing.y * scale_factor));
    ItemInnerSpacing = ImVec2((int)(ItemInnerSpacing.x * scale_factor), (int)(ItemInnerSpacing.y * scale_factor));
    CellPadding = ImVec2((int)(CellPadding.x * scale_factor), (int)(CellPadding.y * scale_factor));
    TouchExtraPadding = ImVec2((int)(TouchExtraPadding.x * scale_factor), (int)(TouchExtraPadding.y * scale_factor));
    IndentSpacing = (int)(IndentSpacing * scale_factor);
    ColumnsMinSpacing = (int)(ColumnsMinSpacing * scale_factor);
    ScrollbarSize = (int)(ScrollbarSize * scale_factor);
    ScrollbarRounding = (int)(ScrollbarRounding * scale_factor);
    GrabMinSize = (int)(GrabMinSize * scale_factor);
    GrabRounding = (int)(GrabRounding * scale_factor);
    LogSliderDeadzone = (int)(LogSliderDeadzone * scale_factor);
    TabRounding = (int)(TabRounding * scale_factor);
    DisplayWindowPadding = ImVec2((int)(DisplayWindowPadding.x * scale_factor), (int)(DisplayWindowPadding.y * scale_factor));
    DisplaySafeAreaPadding = ImVec2((int)(DisplaySafeAreaPadding.x * scale_factor), (int)(DisplaySafeAreaPadding.y * scale_factor));
    MouseCursorScale = (int)(MouseCursorScale * scale_factor);
}

//-----------------------------------------------------------------------------
// [SECTION] ImGuiIO
//-----------------------------------------------------------------------------

ImGuiIO::ImGuiIO()
{
    // Most fields are zero-initialized by default
    memset(this, 0, sizeof(*this));

    ConfigFlags = 0;
    BackendFlags = 0;
    DisplaySize = ImVec2(-1.0f, -1.0f);
    DeltaTime = 1.0f / 60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini";
    LogFilename = "imgui_log.txt";
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
    MouseDragThreshold = 6.0f;
    KeyRepeatDelay = 0.275f;
    KeyRepeatRate = 0.050f;
    HoverDelayNormal = 0.30f;
    HoverDelayShort = 0.10f;
    UserData = nullptr;

    Fonts = nullptr;
    FontGlobalScale = 1.0f;
    FontDefault = nullptr;
    FontAllowUserScaling = false;
    DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    MouseDrawCursor = false;
    ConfigMacOSXBehaviors = false;
    ConfigInputTrickleEventQueue = true;
    ConfigInputTextCursorBlink = true;
    ConfigInputTextEnterKeepActive = false;
    ConfigDragClickToInputText = false;
    ConfigWindowsResizeFromEdges = true;
    ConfigWindowsMoveFromTitleBarOnly = false;
    ConfigMemoryCompactTimer = 60.0f;

    BackendPlatformName = nullptr;
    BackendRendererName = nullptr;
    BackendPlatformUserData = nullptr;
    BackendRendererUserData = nullptr;
    BackendLanguageUserData = nullptr;

    MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    for (int i = 0; i < 5; i++) MouseDown[i] = false;
    MouseWheel = 0.0f;
    MouseWheelH = 0.0f;
    KeyCtrl = false;
    KeyShift = false;
    KeyAlt = false;
    KeySuper = false;

    WantCaptureMouse = false;
    WantCaptureKeyboard = false;
    WantTextInput = false;
    WantSetMousePos = false;
    WantSaveIniSettings = false;
    NavActive = false;
    NavVisible = false;
    Framerate = 0.0f;
    MetricsRenderVertices = 0;
    MetricsRenderIndices = 0;
    MetricsRenderWindows = 0;
    MetricsActiveWindows = 0;
    MetricsActiveAllocations = 0;
    MouseDelta = ImVec2(0.0f, 0.0f);
}

void ImGuiIO::AddKeyEvent(ImGuiKey key, bool down) { (void)key; (void)down; }
void ImGuiIO::AddMousePosEvent(float x, float y) { MousePos = ImVec2(x, y); }
void ImGuiIO::AddMouseButtonEvent(int button, bool down) { if (button >= 0 && button < 5) MouseDown[button] = down; }
void ImGuiIO::AddMouseWheelEvent(float wheel_x, float wheel_y) { MouseWheelH += wheel_x; MouseWheel += wheel_y; }
void ImGuiIO::AddInputCharacter(unsigned int c) { (void)c; }
void ImGuiIO::ClearInputCharacters() {}

//-----------------------------------------------------------------------------
// [SECTION] MAIN CODE (ImGui namespace functions)
//-----------------------------------------------------------------------------

namespace ImGui
{
    // Context creation
    ImGuiContext* CreateContext(ImFontAtlas* shared_font_atlas)
    {
        (void)shared_font_atlas;
        GImGui = (ImGuiContext*)malloc(sizeof(ImGuiContext));
        memset(GImGui, 0, sizeof(ImGuiContext));
        return GImGui;
    }

    void DestroyContext(ImGuiContext* ctx)
    {
        if (ctx == nullptr) ctx = GImGui;
        if (ctx == GImGui) GImGui = nullptr;
        free(ctx);
    }

    ImGuiContext* GetCurrentContext() { return GImGui; }
    void SetCurrentContext(ImGuiContext* ctx) { GImGui = ctx; }

    // Main
    static ImGuiIO s_IO;
    static ImGuiStyle s_Style;
    static ImDrawData s_DrawData;

    ImGuiIO& GetIO() { return s_IO; }
    ImGuiStyle& GetStyle() { return s_Style; }
    
    void NewFrame() 
    { 
        s_IO.DeltaTime = 1.0f / 60.0f;
        s_DrawData.Valid = false;
    }
    
    void EndFrame() {}
    
    void Render() 
    { 
        s_DrawData.Valid = true;
        s_DrawData.CmdListsCount = 0;
        s_DrawData.TotalIdxCount = 0;
        s_DrawData.TotalVtxCount = 0;
        s_DrawData.DisplayPos = ImVec2(0, 0);
        s_DrawData.DisplaySize = s_IO.DisplaySize;
        s_DrawData.FramebufferScale = s_IO.DisplayFramebufferScale;
    }
    
    ImDrawData* GetDrawData() { return &s_DrawData; }

    // Demo windows (stubs)
    void ShowDemoWindow(bool* p_open) { (void)p_open; }
    void ShowMetricsWindow(bool* p_open) { (void)p_open; }
    void ShowDebugLogWindow(bool* p_open) { (void)p_open; }
    void ShowStackToolWindow(bool* p_open) { (void)p_open; }
    void ShowAboutWindow(bool* p_open) { (void)p_open; }
    void ShowStyleEditor(ImGuiStyle* style) { (void)style; }
    bool ShowStyleSelector(const char* label) { (void)label; return false; }
    void ShowFontSelector(const char* label) { (void)label; }
    void ShowUserGuide() {}
    const char* GetVersion() { return IMGUI_VERSION; }
    
    bool DebugCheckVersionAndDataLayout(const char* version, size_t sz_io, size_t sz_style, size_t sz_vec2, size_t sz_vec4, size_t sz_vert, size_t sz_idx)
    {
        (void)version; (void)sz_io; (void)sz_style; (void)sz_vec2; (void)sz_vec4; (void)sz_vert; (void)sz_idx;
        return true;
    }

    // Styles
    void StyleColorsDark(ImGuiStyle* dst)
    {
        ImGuiStyle* style = dst ? dst : &s_Style;
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
        colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    void StyleColorsLight(ImGuiStyle* dst) { StyleColorsDark(dst); }
    void StyleColorsClassic(ImGuiStyle* dst) { StyleColorsDark(dst); }

    // Windows - stubs for compilation
    bool Begin(const char* name, bool* p_open, ImGuiWindowFlags flags) { (void)name; (void)p_open; (void)flags; return true; }
    void End() {}
    bool BeginChild(const char* str_id, const ImVec2& size, bool border, ImGuiWindowFlags flags) { (void)str_id; (void)size; (void)border; (void)flags; return true; }
    bool BeginChild(ImU32 id, const ImVec2& size, bool border, ImGuiWindowFlags flags) { (void)id; (void)size; (void)border; (void)flags; return true; }
    void EndChild() {}

    // Window utilities
    bool IsWindowAppearing() { return false; }
    bool IsWindowCollapsed() { return false; }
    bool IsWindowFocused(ImGuiFocusedFlags flags) { (void)flags; return false; }
    bool IsWindowHovered(ImGuiHoveredFlags flags) { (void)flags; return false; }
    ImDrawList* GetWindowDrawList() { return nullptr; }
    ImVec2 GetWindowPos() { return ImVec2(0, 0); }
    ImVec2 GetWindowSize() { return ImVec2(0, 0); }
    float GetWindowWidth() { return 0.0f; }
    float GetWindowHeight() { return 0.0f; }

    // Window manipulation
    void SetNextWindowPos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot) { (void)pos; (void)cond; (void)pivot; }
    void SetNextWindowSize(const ImVec2& size, ImGuiCond cond) { (void)size; (void)cond; }
    void SetNextWindowSizeConstraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeCallback cb, void* data) { (void)size_min; (void)size_max; (void)cb; (void)data; }
    void SetNextWindowContentSize(const ImVec2& size) { (void)size; }
    void SetNextWindowCollapsed(bool collapsed, ImGuiCond cond) { (void)collapsed; (void)cond; }
    void SetNextWindowFocus() {}
    void SetNextWindowBgAlpha(float alpha) { (void)alpha; }

    // Content region
    ImVec2 GetContentRegionAvail() { return ImVec2(0, 0); }
    ImVec2 GetContentRegionMax() { return ImVec2(0, 0); }
    ImVec2 GetWindowContentRegionMin() { return ImVec2(0, 0); }
    ImVec2 GetWindowContentRegionMax() { return ImVec2(0, 0); }

    // Scrolling
    float GetScrollX() { return 0.0f; }
    float GetScrollY() { return 0.0f; }
    void SetScrollX(float scroll_x) { (void)scroll_x; }
    void SetScrollY(float scroll_y) { (void)scroll_y; }
    float GetScrollMaxX() { return 0.0f; }
    float GetScrollMaxY() { return 0.0f; }
    void SetScrollHereX(float center_x_ratio) { (void)center_x_ratio; }
    void SetScrollHereY(float center_y_ratio) { (void)center_y_ratio; }
    void SetScrollFromPosX(float local_x, float center_x_ratio) { (void)local_x; (void)center_x_ratio; }
    void SetScrollFromPosY(float local_y, float center_y_ratio) { (void)local_y; (void)center_y_ratio; }

    // Parameters stacks (shared)
    void PushFont(ImFont* font) { (void)font; }
    void PopFont() {}
    void PushStyleColor(ImGuiCol idx, ImU32 col) { (void)idx; (void)col; }
    void PushStyleColor(ImGuiCol idx, const ImVec4& col) { (void)idx; (void)col; }
    void PopStyleColor(int count) { (void)count; }
    void PushStyleVar(ImGuiStyleVar idx, float val) { (void)idx; (void)val; }
    void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val) { (void)idx; (void)val; }
    void PopStyleVar(int count) { (void)count; }
    void PushAllowKeyboardFocus(bool allow) { (void)allow; }
    void PopAllowKeyboardFocus() {}
    void PushButtonRepeat(bool repeat) { (void)repeat; }
    void PopButtonRepeat() {}

    // Parameters stacks (current window)
    void PushItemWidth(float item_width) { (void)item_width; }
    void PopItemWidth() {}
    void SetNextItemWidth(float item_width) { (void)item_width; }
    float CalcItemWidth() { return 0.0f; }
    void PushTextWrapPos(float wrap_local_pos_x) { (void)wrap_local_pos_x; }
    void PopTextWrapPos() {}

    // Style read access
    ImFont* GetFont() { return nullptr; }
    float GetFontSize() { return 14.0f; }
    ImVec2 GetFontTexUvWhitePixel() { return ImVec2(0, 0); }
    ImU32 GetColorU32(ImGuiCol idx, float alpha_mul) { (void)idx; (void)alpha_mul; return 0xFFFFFFFF; }
    ImU32 GetColorU32(const ImVec4& col) { (void)col; return 0xFFFFFFFF; }
    ImU32 GetColorU32(ImU32 col) { return col; }
    const ImVec4& GetStyleColorVec4(ImGuiCol idx) { return s_Style.Colors[idx]; }

    // Cursor / Layout
    void Separator() {}
    void SameLine(float offset_from_start_x, float spacing) { (void)offset_from_start_x; (void)spacing; }
    void NewLine() {}
    void Spacing() {}
    void Dummy(const ImVec2& size) { (void)size; }
    void Indent(float indent_w) { (void)indent_w; }
    void Unindent(float indent_w) { (void)indent_w; }
    void BeginGroup() {}
    void EndGroup() {}
    ImVec2 GetCursorPos() { return ImVec2(0, 0); }
    float GetCursorPosX() { return 0.0f; }
    float GetCursorPosY() { return 0.0f; }
    void SetCursorPos(const ImVec2& local_pos) { (void)local_pos; }
    void SetCursorPosX(float local_x) { (void)local_x; }
    void SetCursorPosY(float local_y) { (void)local_y; }
    ImVec2 GetCursorStartPos() { return ImVec2(0, 0); }
    ImVec2 GetCursorScreenPos() { return ImVec2(0, 0); }
    void SetCursorScreenPos(const ImVec2& pos) { (void)pos; }
    void AlignTextToFramePadding() {}
    float GetTextLineHeight() { return 14.0f; }
    float GetTextLineHeightWithSpacing() { return 18.0f; }
    float GetFrameHeight() { return 20.0f; }
    float GetFrameHeightWithSpacing() { return 24.0f; }

    // ID stack/scopes
    void PushID(const char* str_id) { (void)str_id; }
    void PushID(const char* str_id_begin, const char* str_id_end) { (void)str_id_begin; (void)str_id_end; }
    void PushID(const void* ptr_id) { (void)ptr_id; }
    void PushID(int int_id) { (void)int_id; }
    void PopID() {}
    ImU32 GetID(const char* str_id) { (void)str_id; return 0; }
    ImU32 GetID(const char* str_id_begin, const char* str_id_end) { (void)str_id_begin; (void)str_id_end; return 0; }
    ImU32 GetID(const void* ptr_id) { (void)ptr_id; return 0; }

    // Text widgets
    void TextUnformatted(const char* text, const char* text_end) { (void)text; (void)text_end; }
    void Text(const char* fmt, ...) { (void)fmt; }
    void TextV(const char* fmt, va_list args) { (void)fmt; (void)args; }
    void TextColored(const ImVec4& col, const char* fmt, ...) { (void)col; (void)fmt; }
    void TextColoredV(const ImVec4& col, const char* fmt, va_list args) { (void)col; (void)fmt; (void)args; }
    void TextDisabled(const char* fmt, ...) { (void)fmt; }
    void TextDisabledV(const char* fmt, va_list args) { (void)fmt; (void)args; }
    void TextWrapped(const char* fmt, ...) { (void)fmt; }
    void TextWrappedV(const char* fmt, va_list args) { (void)fmt; (void)args; }
    void LabelText(const char* label, const char* fmt, ...) { (void)label; (void)fmt; }
    void LabelTextV(const char* label, const char* fmt, va_list args) { (void)label; (void)fmt; (void)args; }
    void BulletText(const char* fmt, ...) { (void)fmt; }
    void BulletTextV(const char* fmt, va_list args) { (void)fmt; (void)args; }

    // Main widgets
    bool Button(const char* label, const ImVec2& size) { (void)label; (void)size; return false; }
    bool SmallButton(const char* label) { (void)label; return false; }
    bool InvisibleButton(const char* str_id, const ImVec2& size, ImGuiButtonFlags flags) { (void)str_id; (void)size; (void)flags; return false; }
    bool ArrowButton(const char* str_id, ImGuiDir dir) { (void)str_id; (void)dir; return false; }
    bool Checkbox(const char* label, bool* v) { (void)label; (void)v; return false; }
    bool CheckboxFlags(const char* label, int* flags, int flags_value) { (void)label; (void)flags; (void)flags_value; return false; }
    bool CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value) { (void)label; (void)flags; (void)flags_value; return false; }
    bool RadioButton(const char* label, bool active) { (void)label; (void)active; return false; }
    bool RadioButton(const char* label, int* v, int v_button) { (void)label; (void)v; (void)v_button; return false; }
    void ProgressBar(float fraction, const ImVec2& size_arg, const char* overlay) { (void)fraction; (void)size_arg; (void)overlay; }
    void Bullet() {}

    // Combo Box
    bool BeginCombo(const char* label, const char* preview_value, ImGuiWindowFlags flags) { (void)label; (void)preview_value; (void)flags; return false; }
    void EndCombo() {}
    bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items) { (void)label; (void)current_item; (void)items; (void)items_count; (void)popup_max_height_in_items; return false; }
    bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items) { (void)label; (void)current_item; (void)items_separated_by_zeros; (void)popup_max_height_in_items; return false; }
    bool Combo(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items) { (void)label; (void)current_item; (void)items_getter; (void)data; (void)items_count; (void)popup_max_height_in_items; return false; }

    // Drag sliders
    bool DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_speed; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }

    // Sliders
    bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v_rad; (void)v_degrees_min; (void)v_degrees_max; (void)format; (void)flags; return false; }
    bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)size; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }
    bool VSliderInt(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) { (void)label; (void)size; (void)v; (void)v_min; (void)v_max; (void)format; (void)flags; return false; }

    // Input widgets
    bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) { (void)label; (void)buf; (void)buf_size; (void)flags; (void)callback; (void)user_data; return false; }
    bool InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) { (void)label; (void)buf; (void)buf_size; (void)size; (void)flags; (void)callback; (void)user_data; return false; }
    bool InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) { (void)label; (void)hint; (void)buf; (void)buf_size; (void)flags; (void)callback; (void)user_data; return false; }
    bool InputFloat(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags flags) { (void)label; (void)v; (void)step; (void)step_fast; (void)format; (void)flags; return false; }
    bool InputFloat2(const char* label, float v[2], const char* format, ImGuiInputTextFlags flags) { (void)label; (void)v; (void)format; (void)flags; return false; }
    bool InputFloat3(const char* label, float v[3], const char* format, ImGuiInputTextFlags flags) { (void)label; (void)v; (void)format; (void)flags; return false; }
    bool InputFloat4(const char* label, float v[4], const char* format, ImGuiInputTextFlags flags) { (void)label; (void)v; (void)format; (void)flags; return false; }
    bool InputInt(const char* label, int* v, int step, int step_fast, ImGuiInputTextFlags flags) { (void)label; (void)v; (void)step; (void)step_fast; (void)flags; return false; }
    bool InputInt2(const char* label, int v[2], ImGuiInputTextFlags flags) { (void)label; (void)v; (void)flags; return false; }
    bool InputInt3(const char* label, int v[3], ImGuiInputTextFlags flags) { (void)label; (void)v; (void)flags; return false; }
    bool InputInt4(const char* label, int v[4], ImGuiInputTextFlags flags) { (void)label; (void)v; (void)flags; return false; }
    bool InputDouble(const char* label, double* v, double step, double step_fast, const char* format, ImGuiInputTextFlags flags) { (void)label; (void)v; (void)step; (void)step_fast; (void)format; (void)flags; return false; }

    // Color Editor/Picker
    bool ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags) { (void)label; (void)col; (void)flags; return false; }
    bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags) { (void)label; (void)col; (void)flags; return false; }
    bool ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags) { (void)label; (void)col; (void)flags; return false; }
    bool ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col) { (void)label; (void)col; (void)flags; (void)ref_col; return false; }
    bool ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags, const ImVec2& size) { (void)desc_id; (void)col; (void)flags; (void)size; return false; }
    void SetColorEditOptions(ImGuiColorEditFlags flags) { (void)flags; }

    // Trees
    bool TreeNode(const char* label) { (void)label; return false; }
    bool TreeNode(const char* str_id, const char* fmt, ...) { (void)str_id; (void)fmt; return false; }
    bool TreeNode(const void* ptr_id, const char* fmt, ...) { (void)ptr_id; (void)fmt; return false; }
    bool TreeNodeV(const char* str_id, const char* fmt, va_list args) { (void)str_id; (void)fmt; (void)args; return false; }
    bool TreeNodeV(const void* ptr_id, const char* fmt, va_list args) { (void)ptr_id; (void)fmt; (void)args; return false; }
    bool TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags) { (void)label; (void)flags; return false; }
    bool TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) { (void)str_id; (void)flags; (void)fmt; return false; }
    bool TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) { (void)ptr_id; (void)flags; (void)fmt; return false; }
    bool TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) { (void)str_id; (void)flags; (void)fmt; (void)args; return false; }
    bool TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) { (void)ptr_id; (void)flags; (void)fmt; (void)args; return false; }
    void TreePush(const char* str_id) { (void)str_id; }
    void TreePush(const void* ptr_id) { (void)ptr_id; }
    void TreePop() {}
    float GetTreeNodeToLabelSpacing() { return 0.0f; }
    bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags) { (void)label; (void)flags; return false; }
    bool CollapsingHeader(const char* label, bool* p_visible, ImGuiTreeNodeFlags flags) { (void)label; (void)p_visible; (void)flags; return false; }
    void SetNextItemOpen(bool is_open, ImGuiCond cond) { (void)is_open; (void)cond; }

    // Selectables
    bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size) { (void)label; (void)selected; (void)flags; (void)size; return false; }
    bool Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size) { (void)label; (void)p_selected; (void)flags; (void)size; return false; }

    // List Boxes
    bool BeginListBox(const char* label, const ImVec2& size) { (void)label; (void)size; return false; }
    void EndListBox() {}
    bool ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items) { (void)label; (void)current_item; (void)items; (void)items_count; (void)height_in_items; return false; }
    bool ListBox(const char* label, int* current_item, bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items) { (void)label; (void)current_item; (void)items_getter; (void)data; (void)items_count; (void)height_in_items; return false; }

    // Menus
    bool BeginMenuBar() { return false; }
    void EndMenuBar() {}
    bool BeginMainMenuBar() { return false; }
    void EndMainMenuBar() {}
    bool BeginMenu(const char* label, bool enabled) { (void)label; (void)enabled; return false; }
    void EndMenu() {}
    bool MenuItem(const char* label, const char* shortcut, bool selected, bool enabled) { (void)label; (void)shortcut; (void)selected; (void)enabled; return false; }
    bool MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled) { (void)label; (void)shortcut; (void)p_selected; (void)enabled; return false; }

    // Tooltips
    void BeginTooltip() {}
    void EndTooltip() {}
    void SetTooltip(const char* fmt, ...) { (void)fmt; }
    void SetTooltipV(const char* fmt, va_list args) { (void)fmt; (void)args; }

    // Popups
    bool BeginPopup(const char* str_id, ImGuiWindowFlags flags) { (void)str_id; (void)flags; return false; }
    bool BeginPopupModal(const char* name, bool* p_open, ImGuiWindowFlags flags) { (void)name; (void)p_open; (void)flags; return false; }
    void EndPopup() {}
    void OpenPopup(const char* str_id, ImGuiPopupFlags popup_flags) { (void)str_id; (void)popup_flags; }
    void OpenPopup(ImU32 id, ImGuiPopupFlags popup_flags) { (void)id; (void)popup_flags; }
    void OpenPopupOnItemClick(const char* str_id, ImGuiPopupFlags popup_flags) { (void)str_id; (void)popup_flags; }
    void CloseCurrentPopup() {}
    bool BeginPopupContextItem(const char* str_id, ImGuiPopupFlags popup_flags) { (void)str_id; (void)popup_flags; return false; }
    bool BeginPopupContextWindow(const char* str_id, ImGuiPopupFlags popup_flags) { (void)str_id; (void)popup_flags; return false; }
    bool BeginPopupContextVoid(const char* str_id, ImGuiPopupFlags popup_flags) { (void)str_id; (void)popup_flags; return false; }
    bool IsPopupOpen(const char* str_id, ImGuiPopupFlags flags) { (void)str_id; (void)flags; return false; }

    // Tables
    bool BeginTable(const char* str_id, int column, ImGuiTableFlags flags, const ImVec2& outer_size, float inner_width) { (void)str_id; (void)column; (void)flags; (void)outer_size; (void)inner_width; return false; }
    void EndTable() {}
    void TableNextRow(ImGuiTableRowFlags row_flags, float min_row_height) { (void)row_flags; (void)min_row_height; }
    bool TableNextColumn() { return false; }
    bool TableSetColumnIndex(int column_n) { (void)column_n; return false; }

    // Tab Bars
    bool BeginTabBar(const char* str_id, ImGuiTabBarFlags flags) { (void)str_id; (void)flags; return false; }
    void EndTabBar() {}
    bool BeginTabItem(const char* label, bool* p_open, ImGuiTabItemFlags flags) { (void)label; (void)p_open; (void)flags; return false; }
    void EndTabItem() {}
    bool TabItemButton(const char* label, ImGuiTabItemFlags flags) { (void)label; (void)flags; return false; }
    void SetTabItemClosed(const char* tab_or_docked_window_label) { (void)tab_or_docked_window_label; }

    // Logging
    void LogToTTY(int auto_open_depth) { (void)auto_open_depth; }
    void LogToFile(int auto_open_depth, const char* filename) { (void)auto_open_depth; (void)filename; }
    void LogToClipboard(int auto_open_depth) { (void)auto_open_depth; }
    void LogFinish() {}
    void LogButtons() {}
    void LogText(const char* fmt, ...) { (void)fmt; }
    void LogTextV(const char* fmt, va_list args) { (void)fmt; (void)args; }

    // Clipping
    void PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect) { (void)clip_rect_min; (void)clip_rect_max; (void)intersect_with_current_clip_rect; }
    void PopClipRect() {}

    // Focus
    void SetItemDefaultFocus() {}
    void SetKeyboardFocusHere(int offset) { (void)offset; }

    // Item/Widget Utilities
    bool IsItemHovered(ImGuiHoveredFlags flags) { (void)flags; return false; }
    bool IsItemActive() { return false; }
    bool IsItemFocused() { return false; }
    bool IsItemClicked(ImGuiMouseButton mouse_button) { (void)mouse_button; return false; }
    bool IsItemVisible() { return false; }
    bool IsItemEdited() { return false; }
    bool IsItemActivated() { return false; }
    bool IsItemDeactivated() { return false; }
    bool IsItemDeactivatedAfterEdit() { return false; }
    bool IsItemToggledOpen() { return false; }
    bool IsAnyItemHovered() { return false; }
    bool IsAnyItemActive() { return false; }
    bool IsAnyItemFocused() { return false; }
    ImVec2 GetItemRectMin() { return ImVec2(0, 0); }
    ImVec2 GetItemRectMax() { return ImVec2(0, 0); }
    ImVec2 GetItemRectSize() { return ImVec2(0, 0); }
    void SetItemAllowOverlap() {}

    // Misc Utilities
    bool IsRectVisible(const ImVec2& size) { (void)size; return false; }
    bool IsRectVisible(const ImVec2& rect_min, const ImVec2& rect_max) { (void)rect_min; (void)rect_max; return false; }
    double GetTime() { return 0.0; }
    int GetFrameCount() { return 0; }
    ImDrawList* GetBackgroundDrawList() { return nullptr; }
    ImDrawList* GetForegroundDrawList() { return nullptr; }
    const char* GetStyleColorName(ImGuiCol idx) { (void)idx; return ""; }
    void SetStateStorage(ImGuiStorage* storage) { (void)storage; }
    ImGuiStorage* GetStateStorage() { return nullptr; }
    bool BeginChildFrame(ImU32 id, const ImVec2& size, ImGuiWindowFlags flags) { (void)id; (void)size; (void)flags; return false; }
    void EndChildFrame() {}

    // Text Utilities
    ImVec2 CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width) { (void)text; (void)text_end; (void)hide_text_after_double_hash; (void)wrap_width; return ImVec2(0, 0); }

    // Color Utilities
    ImVec4 ColorConvertU32ToFloat4(ImU32 in) { (void)in; return ImVec4(0, 0, 0, 0); }
    ImU32 ColorConvertFloat4ToU32(const ImVec4& in) { (void)in; return 0; }
    void ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v) { (void)r; (void)g; (void)b; out_h = out_s = out_v = 0; }
    void ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b) { (void)h; (void)s; (void)v; out_r = out_g = out_b = 0; }

    // Keyboard Input
    bool IsKeyDown(ImGuiKey key) { (void)key; return false; }
    bool IsKeyPressed(ImGuiKey key, bool repeat) { (void)key; (void)repeat; return false; }
    bool IsKeyReleased(ImGuiKey key) { (void)key; return false; }
    int GetKeyPressedAmount(ImGuiKey key, float repeat_delay, float rate) { (void)key; (void)repeat_delay; (void)rate; return 0; }
    const char* GetKeyName(ImGuiKey key) { (void)key; return ""; }
    void SetNextFrameWantCaptureKeyboard(bool want_capture_keyboard) { (void)want_capture_keyboard; }

    // Mouse Input
    bool IsMouseDown(ImGuiMouseButton button) { return s_IO.MouseDown[button]; }
    bool IsMouseClicked(ImGuiMouseButton button, bool repeat) { (void)button; (void)repeat; return false; }
    bool IsMouseReleased(ImGuiMouseButton button) { (void)button; return false; }
    bool IsMouseDoubleClicked(ImGuiMouseButton button) { (void)button; return false; }
    int GetMouseClickedCount(ImGuiMouseButton button) { (void)button; return 0; }
    bool IsMouseHoveringRect(const ImVec2& r_min, const ImVec2& r_max, bool clip) { (void)r_min; (void)r_max; (void)clip; return false; }
    bool IsMousePosValid(const ImVec2* mouse_pos) { (void)mouse_pos; return true; }
    bool IsAnyMouseDown() { return false; }
    ImVec2 GetMousePos() { return s_IO.MousePos; }
    ImVec2 GetMousePosOnOpeningCurrentPopup() { return ImVec2(0, 0); }
    bool IsMouseDragging(ImGuiMouseButton button, float lock_threshold) { (void)button; (void)lock_threshold; return false; }
    ImVec2 GetMouseDragDelta(ImGuiMouseButton button, float lock_threshold) { (void)button; (void)lock_threshold; return ImVec2(0, 0); }
    void ResetMouseDragDelta(ImGuiMouseButton button) { (void)button; }
    ImGuiMouseCursor GetMouseCursor() { return 0; }
    void SetMouseCursor(ImGuiMouseCursor cursor_type) { (void)cursor_type; }
    void SetNextFrameWantCaptureMouse(bool want_capture_mouse) { (void)want_capture_mouse; }

    // Clipboard
    const char* GetClipboardText() { return ""; }
    void SetClipboardText(const char* text) { (void)text; }

    // Settings
    void LoadIniSettingsFromDisk(const char* ini_filename) { (void)ini_filename; }
    void LoadIniSettingsFromMemory(const char* ini_data, size_t ini_size) { (void)ini_data; (void)ini_size; }
    void SaveIniSettingsToDisk(const char* ini_filename) { (void)ini_filename; }
    const char* SaveIniSettingsToMemory(size_t* out_ini_size) { (void)out_ini_size; return ""; }

    // Memory Allocators
    void SetAllocatorFunctions(ImGuiMemAllocFunc alloc_func, ImGuiMemFreeFunc free_func, void* user_data) { GImAllocatorAllocFunc = alloc_func; GImAllocatorFreeFunc = free_func; GImAllocatorUserData = user_data; }
    void GetAllocatorFunctions(ImGuiMemAllocFunc* p_alloc_func, ImGuiMemFreeFunc* p_free_func, void** p_user_data) { *p_alloc_func = GImAllocatorAllocFunc; *p_free_func = GImAllocatorFreeFunc; *p_user_data = GImAllocatorUserData; }
    void* MemAlloc(size_t size) { return GImAllocatorAllocFunc(size, GImAllocatorUserData); }
    void MemFree(void* ptr) { GImAllocatorFreeFunc(ptr, GImAllocatorUserData); }

} // namespace ImGui

//-----------------------------------------------------------------------------
// [SECTION] ImDrawData
//-----------------------------------------------------------------------------

void ImDrawData::DeIndexAllBuffers() {}
void ImDrawData::ScaleClipRects(const ImVec2& fb_scale) { (void)fb_scale; }

//-----------------------------------------------------------------------------
// [SECTION] ImDrawList
//-----------------------------------------------------------------------------

void ImDrawList::PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect) { (void)clip_rect_min; (void)clip_rect_max; (void)intersect_with_current_clip_rect; }
void ImDrawList::PushClipRectFullScreen() {}
void ImDrawList::PopClipRect() {}
void ImDrawList::PushTextureID(ImTextureID texture_id) { (void)texture_id; }
void ImDrawList::PopTextureID() {}

void ImDrawList::AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness) { (void)p1; (void)p2; (void)col; (void)thickness; }
void ImDrawList::AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags, float thickness) { (void)p_min; (void)p_max; (void)col; (void)rounding; (void)flags; (void)thickness; }
void ImDrawList::AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags) { (void)p_min; (void)p_max; (void)col; (void)rounding; (void)flags; }
void ImDrawList::AddRectFilledMultiColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left) { (void)p_min; (void)p_max; (void)col_upr_left; (void)col_upr_right; (void)col_bot_right; (void)col_bot_left; }
void ImDrawList::AddQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness) { (void)p1; (void)p2; (void)p3; (void)p4; (void)col; (void)thickness; }
void ImDrawList::AddQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col) { (void)p1; (void)p2; (void)p3; (void)p4; (void)col; }
void ImDrawList::AddTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness) { (void)p1; (void)p2; (void)p3; (void)col; (void)thickness; }
void ImDrawList::AddTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col) { (void)p1; (void)p2; (void)p3; (void)col; }
void ImDrawList::AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness) { (void)center; (void)radius; (void)col; (void)num_segments; (void)thickness; }
void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments) { (void)center; (void)radius; (void)col; (void)num_segments; }
void ImDrawList::AddNgon(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness) { (void)center; (void)radius; (void)col; (void)num_segments; (void)thickness; }
void ImDrawList::AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments) { (void)center; (void)radius; (void)col; (void)num_segments; }
void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end) { (void)pos; (void)col; (void)text_begin; (void)text_end; }
void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect) { (void)font; (void)font_size; (void)pos; (void)col; (void)text_begin; (void)text_end; (void)wrap_width; (void)cpu_fine_clip_rect; }
void ImDrawList::AddPolyline(const ImVec2* points, int num_points, ImU32 col, ImDrawFlags flags, float thickness) { (void)points; (void)num_points; (void)col; (void)flags; (void)thickness; }
void ImDrawList::AddConvexPolyFilled(const ImVec2* points, int num_points, ImU32 col) { (void)points; (void)num_points; (void)col; }
void ImDrawList::AddBezierCubic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments) { (void)p1; (void)p2; (void)p3; (void)p4; (void)col; (void)thickness; (void)num_segments; }
void ImDrawList::AddBezierQuadratic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness, int num_segments) { (void)p1; (void)p2; (void)p3; (void)col; (void)thickness; (void)num_segments; }
void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col) { (void)user_texture_id; (void)p_min; (void)p_max; (void)uv_min; (void)uv_max; (void)col; }
void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, ImU32 col) { (void)user_texture_id; (void)p1; (void)p2; (void)p3; (void)p4; (void)uv1; (void)uv2; (void)uv3; (void)uv4; (void)col; }
void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawFlags flags) { (void)user_texture_id; (void)p_min; (void)p_max; (void)uv_min; (void)uv_max; (void)col; (void)rounding; (void)flags; }

void ImDrawList::PathClear() {}
void ImDrawList::PathLineTo(const ImVec2& pos) { (void)pos; }
void ImDrawList::PathLineToMergeDuplicate(const ImVec2& pos) { (void)pos; }
void ImDrawList::PathFillConvex(ImU32 col) { (void)col; }
void ImDrawList::PathStroke(ImU32 col, ImDrawFlags flags, float thickness) { (void)col; (void)flags; (void)thickness; }
void ImDrawList::PathArcTo(const ImVec2& center, float radius, float a_min, float a_max, int num_segments) { (void)center; (void)radius; (void)a_min; (void)a_max; (void)num_segments; }
void ImDrawList::PathArcToFast(const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12) { (void)center; (void)radius; (void)a_min_of_12; (void)a_max_of_12; }
void ImDrawList::PathBezierCubicCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments) { (void)p2; (void)p3; (void)p4; (void)num_segments; }
void ImDrawList::PathBezierQuadraticCurveTo(const ImVec2& p2, const ImVec2& p3, int num_segments) { (void)p2; (void)p3; (void)num_segments; }
void ImDrawList::PathRect(const ImVec2& rect_min, const ImVec2& rect_max, float rounding, ImDrawFlags flags) { (void)rect_min; (void)rect_max; (void)rounding; (void)flags; }

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    memset(this, 0, sizeof(*this));
    FontDataOwnedByAtlas = true;
    OversampleH = 3;
    OversampleV = 1;
    GlyphMaxAdvanceX = FLT_MAX;
    RasterizerMultiply = 1.0f;
    EllipsisChar = (ImWchar)-1;
}

ImFontAtlas::ImFontAtlas() { memset(this, 0, sizeof(*this)); }
ImFontAtlas::~ImFontAtlas() { Clear(); }

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg) { (void)font_cfg; return nullptr; }
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg) { (void)font_cfg; return nullptr; }
ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges) { (void)filename; (void)size_pixels; (void)font_cfg; (void)glyph_ranges; return nullptr; }
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* font_data, int font_size, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges) { (void)font_data; (void)font_size; (void)size_pixels; (void)font_cfg; (void)glyph_ranges; return nullptr; }
ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_font_data, int compressed_font_size, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges) { (void)compressed_font_data; (void)compressed_font_size; (void)size_pixels; (void)font_cfg; (void)glyph_ranges; return nullptr; }
ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_font_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges) { (void)compressed_font_data_base85; (void)size_pixels; (void)font_cfg; (void)glyph_ranges; return nullptr; }
void ImFontAtlas::ClearInputData() {}
void ImFontAtlas::ClearTexData() {}
void ImFontAtlas::ClearFonts() {}
void ImFontAtlas::Clear() {}
bool ImFontAtlas::Build() { return true; }
void ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel) { (void)out_pixels; (void)out_width; (void)out_height; (void)out_bytes_per_pixel; }
void ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel) { (void)out_pixels; (void)out_width; (void)out_height; (void)out_bytes_per_pixel; }

const ImWchar* ImFontAtlas::GetGlyphRangesDefault() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesGreek() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesKorean() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesJapanese() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesChineseFull() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesCyrillic() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesThai() { return nullptr; }
const ImWchar* ImFontAtlas::GetGlyphRangesVietnamese() { return nullptr; }

//-----------------------------------------------------------------------------
// [SECTION] ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont() { memset(this, 0, sizeof(*this)); Scale = 1.0f; }
ImFont::~ImFont() {}
const ImFontGlyph* ImFont::FindGlyph(ImWchar c) const { (void)c; return nullptr; }
const ImFontGlyph* ImFont::FindGlyphNoFallback(ImWchar c) const { (void)c; return nullptr; }
float ImFont::GetCharAdvance(ImWchar c) const { (void)c; return 0.0f; }
const char* ImFont::GetDebugName() const { return ""; }
ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const { (void)size; (void)max_width; (void)wrap_width; (void)text_begin; (void)text_end; (void)remaining; return ImVec2(0, 0); }
void ImFont::RenderChar(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, ImWchar c) const { (void)draw_list; (void)size; (void)pos; (void)col; (void)c; }
void ImFont::RenderText(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip) const { (void)draw_list; (void)size; (void)pos; (void)col; (void)clip_rect; (void)text_begin; (void)text_end; (void)wrap_width; (void)cpu_fine_clip; }
