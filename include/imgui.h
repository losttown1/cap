// dear imgui, v1.89.9
// (headers)
// Zero Elite - Minimal ImGui Header for Overlay

#pragma once

#include "imconfig.h"

#ifndef IMGUI_DISABLE
#include <float.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

// Version
#define IMGUI_VERSION               "1.89.9"
#define IMGUI_VERSION_NUM           18909
#define IMGUI_CHECKVERSION()        ImGui::DebugCheckVersionAndDataLayout(IMGUI_VERSION, sizeof(ImGuiIO), sizeof(ImGuiStyle), sizeof(ImVec2), sizeof(ImVec4), sizeof(ImDrawVert), sizeof(ImDrawIdx))

// Define attributes
#ifndef IMGUI_API
#define IMGUI_API
#endif
#ifndef IMGUI_IMPL_API
#define IMGUI_IMPL_API              IMGUI_API
#endif

// Helpers
#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR) / sizeof(*(_ARR))))
#define IM_UNUSED(_VAR)             ((void)(_VAR))
#define IM_OFFSETOF(_TYPE, _MEMBER) offsetof(_TYPE, _MEMBER)
#define IMGUI_PAYLOAD_TYPE_COLOR_3F "_COL3F"
#define IMGUI_PAYLOAD_TYPE_COLOR_4F "_COL4F"

// Forward declarations
struct ImDrawChannel;
struct ImDrawCmd;
struct ImDrawData;
struct ImDrawList;
struct ImDrawListSharedData;
struct ImDrawListSplitter;
struct ImDrawVert;
struct ImFont;
struct ImFontAtlas;
struct ImFontBuilderIO;
struct ImFontConfig;
struct ImFontGlyph;
struct ImFontGlyphRangesBuilder;
struct ImColor;
struct ImGuiContext;
struct ImGuiIO;
struct ImGuiInputTextCallbackData;
struct ImGuiKeyData;
struct ImGuiListClipper;
struct ImGuiOnceUponAFrame;
struct ImGuiPayload;
struct ImGuiPlatformImeData;
struct ImGuiSizeCallbackData;
struct ImGuiStorage;
struct ImGuiStyle;
struct ImGuiTableSortSpecs;
struct ImGuiTableColumnSortSpecs;
struct ImGuiTextBuffer;
struct ImGuiTextFilter;
struct ImGuiViewport;

// Enums
typedef int ImGuiCol;
typedef int ImGuiCond;
typedef int ImGuiDataType;
typedef int ImGuiDir;
typedef int ImGuiMouseButton;
typedef int ImGuiMouseCursor;
typedef int ImGuiSortDirection;
typedef int ImGuiStyleVar;
typedef int ImGuiTableBgTarget;

// Flags
typedef int ImDrawFlags;
typedef int ImDrawListFlags;
typedef int ImFontAtlasFlags;
typedef int ImGuiBackendFlags;
typedef int ImGuiButtonFlags;
typedef int ImGuiColorEditFlags;
typedef int ImGuiConfigFlags;
typedef int ImGuiFocusedFlags;
typedef int ImGuiHoveredFlags;
typedef int ImGuiInputTextFlags;
typedef int ImGuiKeyChord;
typedef int ImGuiPopupFlags;
typedef int ImGuiSelectableFlags;
typedef int ImGuiSliderFlags;
typedef int ImGuiTabBarFlags;
typedef int ImGuiTabItemFlags;
typedef int ImGuiTableFlags;
typedef int ImGuiTableColumnFlags;
typedef int ImGuiTableRowFlags;
typedef int ImGuiTreeNodeFlags;
typedef int ImGuiViewportFlags;
typedef int ImGuiWindowFlags;

// Types
typedef void* ImTextureID;
typedef unsigned short ImDrawIdx;
typedef unsigned int ImWchar32;
typedef unsigned short ImWchar16;
typedef ImWchar16 ImWchar;
typedef signed char ImS8;
typedef unsigned char ImU8;
typedef signed short ImS16;
typedef unsigned short ImU16;
typedef signed int ImS32;
typedef unsigned int ImU32;
typedef signed long long ImS64;
typedef unsigned long long ImU64;

// Color helpers (must be after ImU32 typedef)
#define IM_COL32_R_SHIFT    0
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    16
#define IM_COL32_A_SHIFT    24
#define IM_COL32(R,G,B,A)   (((ImU32)(A)<<IM_COL32_A_SHIFT) | ((ImU32)(B)<<IM_COL32_B_SHIFT) | ((ImU32)(G)<<IM_COL32_G_SHIFT) | ((ImU32)(R)<<IM_COL32_R_SHIFT))
#define IM_COL32_WHITE      IM_COL32(255,255,255,255)
#define IM_COL32_BLACK      IM_COL32(0,0,0,255)
#define IM_COL32_BLACK_TRANS IM_COL32(0,0,0,0)

// Callback types
typedef int (*ImGuiInputTextCallback)(ImGuiInputTextCallbackData* data);
typedef void (*ImGuiSizeCallback)(ImGuiSizeCallbackData* data);
typedef void* (*ImGuiMemAllocFunc)(size_t sz, void* user_data);
typedef void (*ImGuiMemFreeFunc)(void* ptr, void* user_data);

// ImVec2
struct ImVec2
{
    float x, y;
    constexpr ImVec2() : x(0.0f), y(0.0f) { }
    constexpr ImVec2(float _x, float _y) : x(_x), y(_y) { }
#ifdef IMGUI_DEFINE_MATH_OPERATORS
    ImVec2 operator*(float rhs) const { return ImVec2(x * rhs, y * rhs); }
    ImVec2 operator/(float rhs) const { return ImVec2(x / rhs, y / rhs); }
    ImVec2 operator+(const ImVec2& rhs) const { return ImVec2(x + rhs.x, y + rhs.y); }
    ImVec2 operator-(const ImVec2& rhs) const { return ImVec2(x - rhs.x, y - rhs.y); }
    ImVec2 operator*(const ImVec2& rhs) const { return ImVec2(x * rhs.x, y * rhs.y); }
    ImVec2 operator/(const ImVec2& rhs) const { return ImVec2(x / rhs.x, y / rhs.y); }
    ImVec2& operator*=(float rhs) { x *= rhs; y *= rhs; return *this; }
    ImVec2& operator/=(float rhs) { x /= rhs; y /= rhs; return *this; }
    ImVec2& operator+=(const ImVec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    ImVec2& operator-=(const ImVec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    ImVec2& operator*=(const ImVec2& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
    ImVec2& operator/=(const ImVec2& rhs) { x /= rhs.x; y /= rhs.y; return *this; }
#endif
};

// ImVec4
struct ImVec4
{
    float x, y, z, w;
    constexpr ImVec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
    constexpr ImVec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) { }
};

// ImGui Enums
enum ImGuiWindowFlags_
{
    ImGuiWindowFlags_None                   = 0,
    ImGuiWindowFlags_NoTitleBar             = 1 << 0,
    ImGuiWindowFlags_NoResize               = 1 << 1,
    ImGuiWindowFlags_NoMove                 = 1 << 2,
    ImGuiWindowFlags_NoScrollbar            = 1 << 3,
    ImGuiWindowFlags_NoScrollWithMouse      = 1 << 4,
    ImGuiWindowFlags_NoCollapse             = 1 << 5,
    ImGuiWindowFlags_AlwaysAutoResize       = 1 << 6,
    ImGuiWindowFlags_NoBackground           = 1 << 7,
    ImGuiWindowFlags_NoSavedSettings        = 1 << 8,
    ImGuiWindowFlags_NoMouseInputs          = 1 << 9,
    ImGuiWindowFlags_MenuBar                = 1 << 10,
    ImGuiWindowFlags_HorizontalScrollbar    = 1 << 11,
    ImGuiWindowFlags_NoFocusOnAppearing     = 1 << 12,
    ImGuiWindowFlags_NoBringToFrontOnFocus  = 1 << 13,
    ImGuiWindowFlags_AlwaysVerticalScrollbar= 1 << 14,
    ImGuiWindowFlags_AlwaysHorizontalScrollbar=1<< 15,
    ImGuiWindowFlags_AlwaysUseWindowPadding = 1 << 16,
    ImGuiWindowFlags_NoNavInputs            = 1 << 18,
    ImGuiWindowFlags_NoNavFocus             = 1 << 19,
    ImGuiWindowFlags_UnsavedDocument        = 1 << 20,
    ImGuiWindowFlags_NoNav                  = ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
    ImGuiWindowFlags_NoDecoration           = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse,
    ImGuiWindowFlags_NoInputs               = ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
};

enum ImGuiCol_
{
    ImGuiCol_Text,
    ImGuiCol_TextDisabled,
    ImGuiCol_WindowBg,
    ImGuiCol_ChildBg,
    ImGuiCol_PopupBg,
    ImGuiCol_Border,
    ImGuiCol_BorderShadow,
    ImGuiCol_FrameBg,
    ImGuiCol_FrameBgHovered,
    ImGuiCol_FrameBgActive,
    ImGuiCol_TitleBg,
    ImGuiCol_TitleBgActive,
    ImGuiCol_TitleBgCollapsed,
    ImGuiCol_MenuBarBg,
    ImGuiCol_ScrollbarBg,
    ImGuiCol_ScrollbarGrab,
    ImGuiCol_ScrollbarGrabHovered,
    ImGuiCol_ScrollbarGrabActive,
    ImGuiCol_CheckMark,
    ImGuiCol_SliderGrab,
    ImGuiCol_SliderGrabActive,
    ImGuiCol_Button,
    ImGuiCol_ButtonHovered,
    ImGuiCol_ButtonActive,
    ImGuiCol_Header,
    ImGuiCol_HeaderHovered,
    ImGuiCol_HeaderActive,
    ImGuiCol_Separator,
    ImGuiCol_SeparatorHovered,
    ImGuiCol_SeparatorActive,
    ImGuiCol_ResizeGrip,
    ImGuiCol_ResizeGripHovered,
    ImGuiCol_ResizeGripActive,
    ImGuiCol_Tab,
    ImGuiCol_TabHovered,
    ImGuiCol_TabActive,
    ImGuiCol_TabUnfocused,
    ImGuiCol_TabUnfocusedActive,
    ImGuiCol_PlotLines,
    ImGuiCol_PlotLinesHovered,
    ImGuiCol_PlotHistogram,
    ImGuiCol_PlotHistogramHovered,
    ImGuiCol_TableHeaderBg,
    ImGuiCol_TableBorderStrong,
    ImGuiCol_TableBorderLight,
    ImGuiCol_TableRowBg,
    ImGuiCol_TableRowBgAlt,
    ImGuiCol_TextSelectedBg,
    ImGuiCol_DragDropTarget,
    ImGuiCol_NavHighlight,
    ImGuiCol_NavWindowingHighlight,
    ImGuiCol_NavWindowingDimBg,
    ImGuiCol_ModalWindowDimBg,
    ImGuiCol_COUNT
};

enum ImGuiKey : int
{
    ImGuiKey_None = 0,
    ImGuiKey_Tab = 512,
    ImGuiKey_LeftArrow,
    ImGuiKey_RightArrow,
    ImGuiKey_UpArrow,
    ImGuiKey_DownArrow,
    ImGuiKey_PageUp,
    ImGuiKey_PageDown,
    ImGuiKey_Home,
    ImGuiKey_End,
    ImGuiKey_Insert,
    ImGuiKey_Delete,
    ImGuiKey_Backspace,
    ImGuiKey_Space,
    ImGuiKey_Enter,
    ImGuiKey_Escape,
    ImGuiKey_LeftCtrl, ImGuiKey_LeftShift, ImGuiKey_LeftAlt, ImGuiKey_LeftSuper,
    ImGuiKey_RightCtrl, ImGuiKey_RightShift, ImGuiKey_RightAlt, ImGuiKey_RightSuper,
    ImGuiKey_Menu,
    ImGuiKey_0, ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9,
    ImGuiKey_A, ImGuiKey_B, ImGuiKey_C, ImGuiKey_D, ImGuiKey_E, ImGuiKey_F, ImGuiKey_G, ImGuiKey_H, ImGuiKey_I, ImGuiKey_J,
    ImGuiKey_K, ImGuiKey_L, ImGuiKey_M, ImGuiKey_N, ImGuiKey_O, ImGuiKey_P, ImGuiKey_Q, ImGuiKey_R, ImGuiKey_S, ImGuiKey_T,
    ImGuiKey_U, ImGuiKey_V, ImGuiKey_W, ImGuiKey_X, ImGuiKey_Y, ImGuiKey_Z,
    ImGuiKey_F1, ImGuiKey_F2, ImGuiKey_F3, ImGuiKey_F4, ImGuiKey_F5, ImGuiKey_F6,
    ImGuiKey_F7, ImGuiKey_F8, ImGuiKey_F9, ImGuiKey_F10, ImGuiKey_F11, ImGuiKey_F12,
    ImGuiKey_Apostrophe,
    ImGuiKey_Comma,
    ImGuiKey_Minus,
    ImGuiKey_Period,
    ImGuiKey_Slash,
    ImGuiKey_Semicolon,
    ImGuiKey_Equal,
    ImGuiKey_LeftBracket,
    ImGuiKey_Backslash,
    ImGuiKey_RightBracket,
    ImGuiKey_GraveAccent,
    ImGuiKey_CapsLock,
    ImGuiKey_ScrollLock,
    ImGuiKey_NumLock,
    ImGuiKey_PrintScreen,
    ImGuiKey_Pause,
    ImGuiKey_Keypad0, ImGuiKey_Keypad1, ImGuiKey_Keypad2, ImGuiKey_Keypad3, ImGuiKey_Keypad4,
    ImGuiKey_Keypad5, ImGuiKey_Keypad6, ImGuiKey_Keypad7, ImGuiKey_Keypad8, ImGuiKey_Keypad9,
    ImGuiKey_KeypadDecimal,
    ImGuiKey_KeypadDivide,
    ImGuiKey_KeypadMultiply,
    ImGuiKey_KeypadSubtract,
    ImGuiKey_KeypadAdd,
    ImGuiKey_KeypadEnter,
    ImGuiKey_KeypadEqual,
    ImGuiKey_COUNT,
};

enum ImGuiStyleVar_
{
    ImGuiStyleVar_Alpha,
    ImGuiStyleVar_DisabledAlpha,
    ImGuiStyleVar_WindowPadding,
    ImGuiStyleVar_WindowRounding,
    ImGuiStyleVar_WindowBorderSize,
    ImGuiStyleVar_WindowMinSize,
    ImGuiStyleVar_WindowTitleAlign,
    ImGuiStyleVar_ChildRounding,
    ImGuiStyleVar_ChildBorderSize,
    ImGuiStyleVar_PopupRounding,
    ImGuiStyleVar_PopupBorderSize,
    ImGuiStyleVar_FramePadding,
    ImGuiStyleVar_FrameRounding,
    ImGuiStyleVar_FrameBorderSize,
    ImGuiStyleVar_ItemSpacing,
    ImGuiStyleVar_ItemInnerSpacing,
    ImGuiStyleVar_IndentSpacing,
    ImGuiStyleVar_CellPadding,
    ImGuiStyleVar_ScrollbarSize,
    ImGuiStyleVar_ScrollbarRounding,
    ImGuiStyleVar_GrabMinSize,
    ImGuiStyleVar_GrabRounding,
    ImGuiStyleVar_TabRounding,
    ImGuiStyleVar_ButtonTextAlign,
    ImGuiStyleVar_SelectableTextAlign,
    ImGuiStyleVar_COUNT
};

enum ImGuiCond_
{
    ImGuiCond_None          = 0,
    ImGuiCond_Always        = 1 << 0,
    ImGuiCond_Once          = 1 << 1,
    ImGuiCond_FirstUseEver  = 1 << 2,
    ImGuiCond_Appearing     = 1 << 3,
};

enum ImGuiConfigFlags_
{
    ImGuiConfigFlags_None                   = 0,
    ImGuiConfigFlags_NavEnableKeyboard      = 1 << 0,
    ImGuiConfigFlags_NavEnableGamepad       = 1 << 1,
    ImGuiConfigFlags_NavEnableSetMousePos   = 1 << 2,
    ImGuiConfigFlags_NavNoCaptureKeyboard   = 1 << 3,
    ImGuiConfigFlags_NoMouse                = 1 << 4,
    ImGuiConfigFlags_NoMouseCursorChange    = 1 << 5,
    ImGuiConfigFlags_IsSRGB                 = 1 << 20,
    ImGuiConfigFlags_IsTouchScreen          = 1 << 21,
};

enum ImGuiBackendFlags_
{
    ImGuiBackendFlags_None                  = 0,
    ImGuiBackendFlags_HasGamepad            = 1 << 0,
    ImGuiBackendFlags_HasMouseCursors       = 1 << 1,
    ImGuiBackendFlags_HasSetMousePos        = 1 << 2,
    ImGuiBackendFlags_RendererHasVtxOffset  = 1 << 3,
};

enum ImGuiTreeNodeFlags_
{
    ImGuiTreeNodeFlags_None                 = 0,
    ImGuiTreeNodeFlags_Selected             = 1 << 0,
    ImGuiTreeNodeFlags_Framed               = 1 << 1,
    ImGuiTreeNodeFlags_AllowItemOverlap     = 1 << 2,
    ImGuiTreeNodeFlags_NoTreePushOnOpen     = 1 << 3,
    ImGuiTreeNodeFlags_NoAutoOpenOnLog      = 1 << 4,
    ImGuiTreeNodeFlags_DefaultOpen          = 1 << 5,
    ImGuiTreeNodeFlags_OpenOnDoubleClick    = 1 << 6,
    ImGuiTreeNodeFlags_OpenOnArrow          = 1 << 7,
    ImGuiTreeNodeFlags_Leaf                 = 1 << 8,
    ImGuiTreeNodeFlags_Bullet               = 1 << 9,
    ImGuiTreeNodeFlags_FramePadding         = 1 << 10,
    ImGuiTreeNodeFlags_SpanAvailWidth       = 1 << 11,
    ImGuiTreeNodeFlags_SpanFullWidth        = 1 << 12,
    ImGuiTreeNodeFlags_NavLeftJumpsBackHere = 1 << 13,
    ImGuiTreeNodeFlags_CollapsingHeader     = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_NoAutoOpenOnLog,
};

enum ImGuiMouseCursor_
{
    ImGuiMouseCursor_None = -1,
    ImGuiMouseCursor_Arrow = 0,
    ImGuiMouseCursor_TextInput,
    ImGuiMouseCursor_ResizeAll,
    ImGuiMouseCursor_ResizeNS,
    ImGuiMouseCursor_ResizeEW,
    ImGuiMouseCursor_ResizeNESW,
    ImGuiMouseCursor_ResizeNWSE,
    ImGuiMouseCursor_Hand,
    ImGuiMouseCursor_NotAllowed,
    ImGuiMouseCursor_COUNT
};

// ImGuiStyle
struct ImGuiStyle
{
    float       Alpha;
    float       DisabledAlpha;
    ImVec2      WindowPadding;
    float       WindowRounding;
    float       WindowBorderSize;
    ImVec2      WindowMinSize;
    ImVec2      WindowTitleAlign;
    int         WindowMenuButtonPosition;
    float       ChildRounding;
    float       ChildBorderSize;
    float       PopupRounding;
    float       PopupBorderSize;
    ImVec2      FramePadding;
    float       FrameRounding;
    float       FrameBorderSize;
    ImVec2      ItemSpacing;
    ImVec2      ItemInnerSpacing;
    ImVec2      CellPadding;
    ImVec2      TouchExtraPadding;
    float       IndentSpacing;
    float       ColumnsMinSpacing;
    float       ScrollbarSize;
    float       ScrollbarRounding;
    float       GrabMinSize;
    float       GrabRounding;
    float       LogSliderDeadzone;
    float       TabRounding;
    float       TabBorderSize;
    float       TabMinWidthForCloseButton;
    int         ColorButtonPosition;
    ImVec2      ButtonTextAlign;
    ImVec2      SelectableTextAlign;
    ImVec2      DisplayWindowPadding;
    ImVec2      DisplaySafeAreaPadding;
    float       MouseCursorScale;
    bool        AntiAliasedLines;
    bool        AntiAliasedLinesUseTex;
    bool        AntiAliasedFill;
    float       CurveTessellationTol;
    float       CircleTessellationMaxError;
    ImVec4      Colors[ImGuiCol_COUNT];

    IMGUI_API ImGuiStyle();
    IMGUI_API void ScaleAllSizes(float scale_factor);
};

// ImGuiIO
struct ImGuiIO
{
    ImGuiConfigFlags   ConfigFlags;
    ImGuiBackendFlags  BackendFlags;
    ImVec2      DisplaySize;
    float       DeltaTime;
    float       IniSavingRate;
    const char* IniFilename;
    const char* LogFilename;
    float       MouseDoubleClickTime;
    float       MouseDoubleClickMaxDist;
    float       MouseDragThreshold;
    float       KeyRepeatDelay;
    float       KeyRepeatRate;
    float       HoverDelayNormal;
    float       HoverDelayShort;
    void*       UserData;

    ImFontAtlas* Fonts;
    float       FontGlobalScale;
    bool        FontAllowUserScaling;
    ImFont*     FontDefault;
    ImVec2      DisplayFramebufferScale;

    bool        MouseDrawCursor;
    bool        ConfigMacOSXBehaviors;
    bool        ConfigInputTrickleEventQueue;
    bool        ConfigInputTextCursorBlink;
    bool        ConfigInputTextEnterKeepActive;
    bool        ConfigDragClickToInputText;
    bool        ConfigWindowsResizeFromEdges;
    bool        ConfigWindowsMoveFromTitleBarOnly;
    float       ConfigMemoryCompactTimer;

    const char* BackendPlatformName;
    const char* BackendRendererName;
    void*       BackendPlatformUserData;
    void*       BackendRendererUserData;
    void*       BackendLanguageUserData;

    // Input
    ImVec2      MousePos;
    bool        MouseDown[5];
    float       MouseWheel;
    float       MouseWheelH;
    bool        KeyCtrl;
    bool        KeyShift;
    bool        KeyAlt;
    bool        KeySuper;

    IMGUI_API void AddKeyEvent(ImGuiKey key, bool down);
    IMGUI_API void AddMousePosEvent(float x, float y);
    IMGUI_API void AddMouseButtonEvent(int button, bool down);
    IMGUI_API void AddMouseWheelEvent(float wheel_x, float wheel_y);
    IMGUI_API void AddInputCharacter(unsigned int c);
    IMGUI_API void ClearInputCharacters();

    // Output
    bool        WantCaptureMouse;
    bool        WantCaptureKeyboard;
    bool        WantTextInput;
    bool        WantSetMousePos;
    bool        WantSaveIniSettings;
    bool        NavActive;
    bool        NavVisible;
    float       Framerate;
    int         MetricsRenderVertices;
    int         MetricsRenderIndices;
    int         MetricsRenderWindows;
    int         MetricsActiveWindows;
    int         MetricsActiveAllocations;
    ImVec2      MouseDelta;

    IMGUI_API ImGuiIO();
};

// Draw structures
struct ImDrawVert
{
    ImVec2  pos;
    ImVec2  uv;
    ImU32   col;
};

struct ImDrawCmd
{
    ImVec4          ClipRect;
    ImTextureID     TextureId;
    unsigned int    VtxOffset;
    unsigned int    IdxOffset;
    unsigned int    ElemCount;
    void            (*UserCallback)(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    void*           UserCallbackData;

    ImDrawCmd() { memset(this, 0, sizeof(*this)); }
};

struct ImDrawData
{
    bool            Valid;
    int             CmdListsCount;
    int             TotalIdxCount;
    int             TotalVtxCount;
    ImDrawList**    CmdLists;
    ImVec2          DisplayPos;
    ImVec2          DisplaySize;
    ImVec2          FramebufferScale;

    void Clear() { memset(this, 0, sizeof(*this)); }
    IMGUI_API void DeIndexAllBuffers();
    IMGUI_API void ScaleClipRects(const ImVec2& fb_scale);
};

// ImGui namespace
namespace ImGui
{
    // Context creation
    IMGUI_API ImGuiContext* CreateContext(ImFontAtlas* shared_font_atlas = nullptr);
    IMGUI_API void          DestroyContext(ImGuiContext* ctx = nullptr);
    IMGUI_API ImGuiContext* GetCurrentContext();
    IMGUI_API void          SetCurrentContext(ImGuiContext* ctx);

    // Main
    IMGUI_API ImGuiIO&      GetIO();
    IMGUI_API ImGuiStyle&   GetStyle();
    IMGUI_API void          NewFrame();
    IMGUI_API void          EndFrame();
    IMGUI_API void          Render();
    IMGUI_API ImDrawData*   GetDrawData();

    // Demo, Debug, Information
    IMGUI_API void          ShowDemoWindow(bool* p_open = nullptr);
    IMGUI_API void          ShowMetricsWindow(bool* p_open = nullptr);
    IMGUI_API void          ShowDebugLogWindow(bool* p_open = nullptr);
    IMGUI_API void          ShowStackToolWindow(bool* p_open = nullptr);
    IMGUI_API void          ShowAboutWindow(bool* p_open = nullptr);
    IMGUI_API void          ShowStyleEditor(ImGuiStyle* style = nullptr);
    IMGUI_API bool          ShowStyleSelector(const char* label);
    IMGUI_API void          ShowFontSelector(const char* label);
    IMGUI_API void          ShowUserGuide();
    IMGUI_API const char*   GetVersion();
    IMGUI_API bool          DebugCheckVersionAndDataLayout(const char* version, size_t sz_io, size_t sz_style, size_t sz_vec2, size_t sz_vec4, size_t sz_vert, size_t sz_idx);

    // Styles
    IMGUI_API void          StyleColorsDark(ImGuiStyle* dst = nullptr);
    IMGUI_API void          StyleColorsLight(ImGuiStyle* dst = nullptr);
    IMGUI_API void          StyleColorsClassic(ImGuiStyle* dst = nullptr);

    // Windows
    IMGUI_API bool          Begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
    IMGUI_API void          End();

    // Child Windows
    IMGUI_API bool          BeginChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0);
    IMGUI_API bool          BeginChild(ImU32 id, const ImVec2& size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0);
    IMGUI_API void          EndChild();

    // Windows Utilities
    IMGUI_API bool          IsWindowAppearing();
    IMGUI_API bool          IsWindowCollapsed();
    IMGUI_API bool          IsWindowFocused(ImGuiFocusedFlags flags = 0);
    IMGUI_API bool          IsWindowHovered(ImGuiHoveredFlags flags = 0);
    IMGUI_API ImDrawList*   GetWindowDrawList();
    IMGUI_API ImVec2        GetWindowPos();
    IMGUI_API ImVec2        GetWindowSize();
    IMGUI_API float         GetWindowWidth();
    IMGUI_API float         GetWindowHeight();

    // Window manipulation
    IMGUI_API void          SetNextWindowPos(const ImVec2& pos, ImGuiCond cond = 0, const ImVec2& pivot = ImVec2(0, 0));
    IMGUI_API void          SetNextWindowSize(const ImVec2& size, ImGuiCond cond = 0);
    IMGUI_API void          SetNextWindowSizeConstraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeCallback custom_callback = nullptr, void* custom_callback_data = nullptr);
    IMGUI_API void          SetNextWindowContentSize(const ImVec2& size);
    IMGUI_API void          SetNextWindowCollapsed(bool collapsed, ImGuiCond cond = 0);
    IMGUI_API void          SetNextWindowFocus();
    IMGUI_API void          SetNextWindowBgAlpha(float alpha);

    // Content region
    IMGUI_API ImVec2        GetContentRegionAvail();
    IMGUI_API ImVec2        GetContentRegionMax();
    IMGUI_API ImVec2        GetWindowContentRegionMin();
    IMGUI_API ImVec2        GetWindowContentRegionMax();

    // Windows Scrolling
    IMGUI_API float         GetScrollX();
    IMGUI_API float         GetScrollY();
    IMGUI_API void          SetScrollX(float scroll_x);
    IMGUI_API void          SetScrollY(float scroll_y);
    IMGUI_API float         GetScrollMaxX();
    IMGUI_API float         GetScrollMaxY();
    IMGUI_API void          SetScrollHereX(float center_x_ratio = 0.5f);
    IMGUI_API void          SetScrollHereY(float center_y_ratio = 0.5f);
    IMGUI_API void          SetScrollFromPosX(float local_x, float center_x_ratio = 0.5f);
    IMGUI_API void          SetScrollFromPosY(float local_y, float center_y_ratio = 0.5f);

    // Parameters stacks (shared)
    IMGUI_API void          PushFont(ImFont* font);
    IMGUI_API void          PopFont();
    IMGUI_API void          PushStyleColor(ImGuiCol idx, ImU32 col);
    IMGUI_API void          PushStyleColor(ImGuiCol idx, const ImVec4& col);
    IMGUI_API void          PopStyleColor(int count = 1);
    IMGUI_API void          PushStyleVar(ImGuiStyleVar idx, float val);
    IMGUI_API void          PushStyleVar(ImGuiStyleVar idx, const ImVec2& val);
    IMGUI_API void          PopStyleVar(int count = 1);
    IMGUI_API void          PushAllowKeyboardFocus(bool allow_keyboard_focus);
    IMGUI_API void          PopAllowKeyboardFocus();
    IMGUI_API void          PushButtonRepeat(bool repeat);
    IMGUI_API void          PopButtonRepeat();

    // Parameters stacks (current window)
    IMGUI_API void          PushItemWidth(float item_width);
    IMGUI_API void          PopItemWidth();
    IMGUI_API void          SetNextItemWidth(float item_width);
    IMGUI_API float         CalcItemWidth();
    IMGUI_API void          PushTextWrapPos(float wrap_local_pos_x = 0.0f);
    IMGUI_API void          PopTextWrapPos();

    // Style read access
    IMGUI_API ImFont*       GetFont();
    IMGUI_API float         GetFontSize();
    IMGUI_API ImVec2        GetFontTexUvWhitePixel();
    IMGUI_API ImU32         GetColorU32(ImGuiCol idx, float alpha_mul = 1.0f);
    IMGUI_API ImU32         GetColorU32(const ImVec4& col);
    IMGUI_API ImU32         GetColorU32(ImU32 col);
    IMGUI_API const ImVec4& GetStyleColorVec4(ImGuiCol idx);

    // Cursor / Layout
    IMGUI_API void          Separator();
    IMGUI_API void          SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
    IMGUI_API void          NewLine();
    IMGUI_API void          Spacing();
    IMGUI_API void          Dummy(const ImVec2& size);
    IMGUI_API void          Indent(float indent_w = 0.0f);
    IMGUI_API void          Unindent(float indent_w = 0.0f);
    IMGUI_API void          BeginGroup();
    IMGUI_API void          EndGroup();
    IMGUI_API ImVec2        GetCursorPos();
    IMGUI_API float         GetCursorPosX();
    IMGUI_API float         GetCursorPosY();
    IMGUI_API void          SetCursorPos(const ImVec2& local_pos);
    IMGUI_API void          SetCursorPosX(float local_x);
    IMGUI_API void          SetCursorPosY(float local_y);
    IMGUI_API ImVec2        GetCursorStartPos();
    IMGUI_API ImVec2        GetCursorScreenPos();
    IMGUI_API void          SetCursorScreenPos(const ImVec2& pos);
    IMGUI_API void          AlignTextToFramePadding();
    IMGUI_API float         GetTextLineHeight();
    IMGUI_API float         GetTextLineHeightWithSpacing();
    IMGUI_API float         GetFrameHeight();
    IMGUI_API float         GetFrameHeightWithSpacing();

    // ID stack/scopes
    IMGUI_API void          PushID(const char* str_id);
    IMGUI_API void          PushID(const char* str_id_begin, const char* str_id_end);
    IMGUI_API void          PushID(const void* ptr_id);
    IMGUI_API void          PushID(int int_id);
    IMGUI_API void          PopID();
    IMGUI_API ImU32         GetID(const char* str_id);
    IMGUI_API ImU32         GetID(const char* str_id_begin, const char* str_id_end);
    IMGUI_API ImU32         GetID(const void* ptr_id);

    // Widgets: Text
    IMGUI_API void          TextUnformatted(const char* text, const char* text_end = nullptr);
    IMGUI_API void          Text(const char* fmt, ...);
    IMGUI_API void          TextV(const char* fmt, va_list args);
    IMGUI_API void          TextColored(const ImVec4& col, const char* fmt, ...);
    IMGUI_API void          TextColoredV(const ImVec4& col, const char* fmt, va_list args);
    IMGUI_API void          TextDisabled(const char* fmt, ...);
    IMGUI_API void          TextDisabledV(const char* fmt, va_list args);
    IMGUI_API void          TextWrapped(const char* fmt, ...);
    IMGUI_API void          TextWrappedV(const char* fmt, va_list args);
    IMGUI_API void          LabelText(const char* label, const char* fmt, ...);
    IMGUI_API void          LabelTextV(const char* label, const char* fmt, va_list args);
    IMGUI_API void          BulletText(const char* fmt, ...);
    IMGUI_API void          BulletTextV(const char* fmt, va_list args);

    // Widgets: Main
    IMGUI_API bool          Button(const char* label, const ImVec2& size = ImVec2(0, 0));
    IMGUI_API bool          SmallButton(const char* label);
    IMGUI_API bool          InvisibleButton(const char* str_id, const ImVec2& size, ImGuiButtonFlags flags = 0);
    IMGUI_API bool          ArrowButton(const char* str_id, ImGuiDir dir);
    IMGUI_API bool          Checkbox(const char* label, bool* v);
    IMGUI_API bool          CheckboxFlags(const char* label, int* flags, int flags_value);
    IMGUI_API bool          CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
    IMGUI_API bool          RadioButton(const char* label, bool active);
    IMGUI_API bool          RadioButton(const char* label, int* v, int v_button);
    IMGUI_API void          ProgressBar(float fraction, const ImVec2& size_arg = ImVec2(-FLT_MIN, 0), const char* overlay = nullptr);
    IMGUI_API void          Bullet();

    // Widgets: Combo Box
    IMGUI_API bool          BeginCombo(const char* label, const char* preview_value, ImGuiWindowFlags flags = 0);
    IMGUI_API void          EndCombo();
    IMGUI_API bool          Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
    IMGUI_API bool          Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
    IMGUI_API bool          Combo(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items = -1);

    // Widgets: Drags
    IMGUI_API bool          DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          DragFloat2(const char* label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          DragFloat3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          DragFloat4(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          DragInt(const char* label, int* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          DragInt2(const char* label, int v[2], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          DragInt3(const char* label, int v[3], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          DragInt4(const char* label, int v[4], float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);

    // Widgets: Sliders
    IMGUI_API bool          SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderAngle(const char* label, float* v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f, const char* format = "%.0f deg", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool          VSliderInt(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);

    // Widgets: Input with Keyboard
    IMGUI_API bool          InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
    IMGUI_API bool          InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
    IMGUI_API bool          InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
    IMGUI_API bool          InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputFloat2(const char* label, float v[2], const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputFloat3(const char* label, float v[3], const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputFloat4(const char* label, float v[4], const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputInt(const char* label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputInt2(const char* label, int v[2], ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputInt3(const char* label, int v[3], ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputInt4(const char* label, int v[4], ImGuiInputTextFlags flags = 0);
    IMGUI_API bool          InputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0);

    // Widgets: Color Editor/Picker
    IMGUI_API bool          ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
    IMGUI_API bool          ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0);
    IMGUI_API bool          ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
    IMGUI_API bool          ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags = 0, const float* ref_col = nullptr);
    IMGUI_API bool          ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
    IMGUI_API void          SetColorEditOptions(ImGuiColorEditFlags flags);

    // Widgets: Trees
    IMGUI_API bool          TreeNode(const char* label);
    IMGUI_API bool          TreeNode(const char* str_id, const char* fmt, ...);
    IMGUI_API bool          TreeNode(const void* ptr_id, const char* fmt, ...);
    IMGUI_API bool          TreeNodeV(const char* str_id, const char* fmt, va_list args);
    IMGUI_API bool          TreeNodeV(const void* ptr_id, const char* fmt, va_list args);
    IMGUI_API bool          TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags = 0);
    IMGUI_API bool          TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...);
    IMGUI_API bool          TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...);
    IMGUI_API bool          TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args);
    IMGUI_API bool          TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args);
    IMGUI_API void          TreePush(const char* str_id);
    IMGUI_API void          TreePush(const void* ptr_id = nullptr);
    IMGUI_API void          TreePop();
    IMGUI_API float         GetTreeNodeToLabelSpacing();
    IMGUI_API bool          CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);
    IMGUI_API bool          CollapsingHeader(const char* label, bool* p_visible, ImGuiTreeNodeFlags flags = 0);
    IMGUI_API void          SetNextItemOpen(bool is_open, ImGuiCond cond = 0);

    // Widgets: Selectables
    IMGUI_API bool          Selectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
    IMGUI_API bool          Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));

    // Widgets: List Boxes
    IMGUI_API bool          BeginListBox(const char* label, const ImVec2& size = ImVec2(0, 0));
    IMGUI_API void          EndListBox();
    IMGUI_API bool          ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items = -1);
    IMGUI_API bool          ListBox(const char* label, int* current_item, bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items = -1);

    // Widgets: Menus
    IMGUI_API bool          BeginMenuBar();
    IMGUI_API void          EndMenuBar();
    IMGUI_API bool          BeginMainMenuBar();
    IMGUI_API void          EndMainMenuBar();
    IMGUI_API bool          BeginMenu(const char* label, bool enabled = true);
    IMGUI_API void          EndMenu();
    IMGUI_API bool          MenuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
    IMGUI_API bool          MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled = true);

    // Tooltips
    IMGUI_API void          BeginTooltip();
    IMGUI_API void          EndTooltip();
    IMGUI_API void          SetTooltip(const char* fmt, ...);
    IMGUI_API void          SetTooltipV(const char* fmt, va_list args);

    // Popups, Modals
    IMGUI_API bool          BeginPopup(const char* str_id, ImGuiWindowFlags flags = 0);
    IMGUI_API bool          BeginPopupModal(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
    IMGUI_API void          EndPopup();
    IMGUI_API void          OpenPopup(const char* str_id, ImGuiPopupFlags popup_flags = 0);
    IMGUI_API void          OpenPopup(ImU32 id, ImGuiPopupFlags popup_flags = 0);
    IMGUI_API void          OpenPopupOnItemClick(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 1);
    IMGUI_API void          CloseCurrentPopup();
    IMGUI_API bool          BeginPopupContextItem(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 1);
    IMGUI_API bool          BeginPopupContextWindow(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 1);
    IMGUI_API bool          BeginPopupContextVoid(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 1);
    IMGUI_API bool          IsPopupOpen(const char* str_id, ImGuiPopupFlags flags = 0);

    // Tables
    IMGUI_API bool          BeginTable(const char* str_id, int column, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f);
    IMGUI_API void          EndTable();
    IMGUI_API void          TableNextRow(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f);
    IMGUI_API bool          TableNextColumn();
    IMGUI_API bool          TableSetColumnIndex(int column_n);

    // Tab Bars, Tabs
    IMGUI_API bool          BeginTabBar(const char* str_id, ImGuiTabBarFlags flags = 0);
    IMGUI_API void          EndTabBar();
    IMGUI_API bool          BeginTabItem(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0);
    IMGUI_API void          EndTabItem();
    IMGUI_API bool          TabItemButton(const char* label, ImGuiTabItemFlags flags = 0);
    IMGUI_API void          SetTabItemClosed(const char* tab_or_docked_window_label);

    // Logging/Capture
    IMGUI_API void          LogToTTY(int auto_open_depth = -1);
    IMGUI_API void          LogToFile(int auto_open_depth = -1, const char* filename = nullptr);
    IMGUI_API void          LogToClipboard(int auto_open_depth = -1);
    IMGUI_API void          LogFinish();
    IMGUI_API void          LogButtons();
    IMGUI_API void          LogText(const char* fmt, ...);
    IMGUI_API void          LogTextV(const char* fmt, va_list args);

    // Clipping
    IMGUI_API void          PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect);
    IMGUI_API void          PopClipRect();

    // Focus, Activation
    IMGUI_API void          SetItemDefaultFocus();
    IMGUI_API void          SetKeyboardFocusHere(int offset = 0);

    // Item/Widgets Utilities
    IMGUI_API bool          IsItemHovered(ImGuiHoveredFlags flags = 0);
    IMGUI_API bool          IsItemActive();
    IMGUI_API bool          IsItemFocused();
    IMGUI_API bool          IsItemClicked(ImGuiMouseButton mouse_button = 0);
    IMGUI_API bool          IsItemVisible();
    IMGUI_API bool          IsItemEdited();
    IMGUI_API bool          IsItemActivated();
    IMGUI_API bool          IsItemDeactivated();
    IMGUI_API bool          IsItemDeactivatedAfterEdit();
    IMGUI_API bool          IsItemToggledOpen();
    IMGUI_API bool          IsAnyItemHovered();
    IMGUI_API bool          IsAnyItemActive();
    IMGUI_API bool          IsAnyItemFocused();
    IMGUI_API ImVec2        GetItemRectMin();
    IMGUI_API ImVec2        GetItemRectMax();
    IMGUI_API ImVec2        GetItemRectSize();
    IMGUI_API void          SetItemAllowOverlap();

    // Miscellaneous Utilities
    IMGUI_API bool          IsRectVisible(const ImVec2& size);
    IMGUI_API bool          IsRectVisible(const ImVec2& rect_min, const ImVec2& rect_max);
    IMGUI_API double        GetTime();
    IMGUI_API int           GetFrameCount();
    IMGUI_API ImDrawList*   GetBackgroundDrawList();
    IMGUI_API ImDrawList*   GetForegroundDrawList();
    IMGUI_API const char*   GetStyleColorName(ImGuiCol idx);
    IMGUI_API void          SetStateStorage(ImGuiStorage* storage);
    IMGUI_API ImGuiStorage* GetStateStorage();
    IMGUI_API bool          BeginChildFrame(ImU32 id, const ImVec2& size, ImGuiWindowFlags flags = 0);
    IMGUI_API void          EndChildFrame();

    // Text Utilities
    IMGUI_API ImVec2        CalcTextSize(const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);

    // Color Utilities
    IMGUI_API ImVec4        ColorConvertU32ToFloat4(ImU32 in);
    IMGUI_API ImU32         ColorConvertFloat4ToU32(const ImVec4& in);
    IMGUI_API void          ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v);
    IMGUI_API void          ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b);

    // Inputs Utilities: Keyboard
    IMGUI_API bool          IsKeyDown(ImGuiKey key);
    IMGUI_API bool          IsKeyPressed(ImGuiKey key, bool repeat = true);
    IMGUI_API bool          IsKeyReleased(ImGuiKey key);
    IMGUI_API int           GetKeyPressedAmount(ImGuiKey key, float repeat_delay, float rate);
    IMGUI_API const char*   GetKeyName(ImGuiKey key);
    IMGUI_API void          SetNextFrameWantCaptureKeyboard(bool want_capture_keyboard);

    // Inputs Utilities: Mouse
    IMGUI_API bool          IsMouseDown(ImGuiMouseButton button);
    IMGUI_API bool          IsMouseClicked(ImGuiMouseButton button, bool repeat = false);
    IMGUI_API bool          IsMouseReleased(ImGuiMouseButton button);
    IMGUI_API bool          IsMouseDoubleClicked(ImGuiMouseButton button);
    IMGUI_API int           GetMouseClickedCount(ImGuiMouseButton button);
    IMGUI_API bool          IsMouseHoveringRect(const ImVec2& r_min, const ImVec2& r_max, bool clip = true);
    IMGUI_API bool          IsMousePosValid(const ImVec2* mouse_pos = nullptr);
    IMGUI_API bool          IsAnyMouseDown();
    IMGUI_API ImVec2        GetMousePos();
    IMGUI_API ImVec2        GetMousePosOnOpeningCurrentPopup();
    IMGUI_API bool          IsMouseDragging(ImGuiMouseButton button, float lock_threshold = -1.0f);
    IMGUI_API ImVec2        GetMouseDragDelta(ImGuiMouseButton button = 0, float lock_threshold = -1.0f);
    IMGUI_API void          ResetMouseDragDelta(ImGuiMouseButton button = 0);
    IMGUI_API ImGuiMouseCursor GetMouseCursor();
    IMGUI_API void          SetMouseCursor(ImGuiMouseCursor cursor_type);
    IMGUI_API void          SetNextFrameWantCaptureMouse(bool want_capture_mouse);

    // Clipboard Utilities
    IMGUI_API const char*   GetClipboardText();
    IMGUI_API void          SetClipboardText(const char* text);

    // Settings/.Ini Utilities
    IMGUI_API void          LoadIniSettingsFromDisk(const char* ini_filename);
    IMGUI_API void          LoadIniSettingsFromMemory(const char* ini_data, size_t ini_size = 0);
    IMGUI_API void          SaveIniSettingsToDisk(const char* ini_filename);
    IMGUI_API const char*   SaveIniSettingsToMemory(size_t* out_ini_size = nullptr);

    // Memory Allocators
    IMGUI_API void          SetAllocatorFunctions(ImGuiMemAllocFunc alloc_func, ImGuiMemFreeFunc free_func, void* user_data = nullptr);
    IMGUI_API void          GetAllocatorFunctions(ImGuiMemAllocFunc* p_alloc_func, ImGuiMemFreeFunc* p_free_func, void** p_user_data);
    IMGUI_API void*         MemAlloc(size_t size);
    IMGUI_API void          MemFree(void* ptr);

} // namespace ImGui

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

struct ImDrawList
{
    ImDrawCmd*              CmdBuffer;
    ImDrawIdx*              IdxBuffer;
    ImDrawVert*             VtxBuffer;
    ImDrawListFlags         Flags;

    IMGUI_API void  PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false);
    IMGUI_API void  PushClipRectFullScreen();
    IMGUI_API void  PopClipRect();
    IMGUI_API void  PushTextureID(ImTextureID texture_id);
    IMGUI_API void  PopTextureID();
    inline ImVec2   GetClipRectMin() const { return ImVec2(0, 0); }
    inline ImVec2   GetClipRectMax() const { return ImVec2(0, 0); }

    IMGUI_API void  AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f);
    IMGUI_API void  AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f);
    IMGUI_API void  AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0);
    IMGUI_API void  AddRectFilledMultiColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left);
    IMGUI_API void  AddQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness = 1.0f);
    IMGUI_API void  AddQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col);
    IMGUI_API void  AddTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness = 1.0f);
    IMGUI_API void  AddTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col);
    IMGUI_API void  AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments = 0, float thickness = 1.0f);
    IMGUI_API void  AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments = 0);
    IMGUI_API void  AddNgon(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness = 1.0f);
    IMGUI_API void  AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments);
    IMGUI_API void  AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = nullptr);
    IMGUI_API void  AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = nullptr, float wrap_width = 0.0f, const ImVec4* cpu_fine_clip_rect = nullptr);
    IMGUI_API void  AddPolyline(const ImVec2* points, int num_points, ImU32 col, ImDrawFlags flags, float thickness);
    IMGUI_API void  AddConvexPolyFilled(const ImVec2* points, int num_points, ImU32 col);
    IMGUI_API void  AddBezierCubic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments = 0);
    IMGUI_API void  AddBezierQuadratic(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness, int num_segments = 0);
    IMGUI_API void  AddImage(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2(0, 0), const ImVec2& uv_max = ImVec2(1, 1), ImU32 col = 0xFFFFFFFF);
    IMGUI_API void  AddImageQuad(ImTextureID user_texture_id, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1 = ImVec2(0, 0), const ImVec2& uv2 = ImVec2(1, 0), const ImVec2& uv3 = ImVec2(1, 1), const ImVec2& uv4 = ImVec2(0, 1), ImU32 col = 0xFFFFFFFF);
    IMGUI_API void  AddImageRounded(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawFlags flags = 0);

    // Stateful path API
    IMGUI_API void  PathClear();
    IMGUI_API void  PathLineTo(const ImVec2& pos);
    IMGUI_API void  PathLineToMergeDuplicate(const ImVec2& pos);
    IMGUI_API void  PathFillConvex(ImU32 col);
    IMGUI_API void  PathStroke(ImU32 col, ImDrawFlags flags = 0, float thickness = 1.0f);
    IMGUI_API void  PathArcTo(const ImVec2& center, float radius, float a_min, float a_max, int num_segments = 0);
    IMGUI_API void  PathArcToFast(const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12);
    IMGUI_API void  PathBezierCubicCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments = 0);
    IMGUI_API void  PathBezierQuadraticCurveTo(const ImVec2& p2, const ImVec2& p3, int num_segments = 0);
    IMGUI_API void  PathRect(const ImVec2& rect_min, const ImVec2& rect_max, float rounding = 0.0f, ImDrawFlags flags = 0);
};

//-----------------------------------------------------------------------------
// Font API
//-----------------------------------------------------------------------------

struct ImFontConfig
{
    void*           FontData;
    int             FontDataSize;
    bool            FontDataOwnedByAtlas;
    int             FontNo;
    float           SizePixels;
    int             OversampleH;
    int             OversampleV;
    bool            PixelSnapH;
    ImVec2          GlyphExtraSpacing;
    ImVec2          GlyphOffset;
    const ImWchar*  GlyphRanges;
    float           GlyphMinAdvanceX;
    float           GlyphMaxAdvanceX;
    bool            MergeMode;
    unsigned int    FontBuilderFlags;
    float           RasterizerMultiply;
    ImWchar         EllipsisChar;

    IMGUI_API ImFontConfig();
};

struct ImFontAtlas
{
    IMGUI_API ImFontAtlas();
    IMGUI_API ~ImFontAtlas();
    IMGUI_API ImFont*           AddFont(const ImFontConfig* font_cfg);
    IMGUI_API ImFont*           AddFontDefault(const ImFontConfig* font_cfg = nullptr);
    IMGUI_API ImFont*           AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg = nullptr, const ImWchar* glyph_ranges = nullptr);
    IMGUI_API ImFont*           AddFontFromMemoryTTF(void* font_data, int font_size, float size_pixels, const ImFontConfig* font_cfg = nullptr, const ImWchar* glyph_ranges = nullptr);
    IMGUI_API ImFont*           AddFontFromMemoryCompressedTTF(const void* compressed_font_data, int compressed_font_size, float size_pixels, const ImFontConfig* font_cfg = nullptr, const ImWchar* glyph_ranges = nullptr);
    IMGUI_API ImFont*           AddFontFromMemoryCompressedBase85TTF(const char* compressed_font_data_base85, float size_pixels, const ImFontConfig* font_cfg = nullptr, const ImWchar* glyph_ranges = nullptr);
    IMGUI_API void              ClearInputData();
    IMGUI_API void              ClearTexData();
    IMGUI_API void              ClearFonts();
    IMGUI_API void              Clear();
    IMGUI_API bool              Build();
    IMGUI_API void              GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel = nullptr);
    IMGUI_API void              GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel = nullptr);
    IMGUI_API bool              IsBuilt() const { return Fonts.Size > 0 && TexReady; }
    IMGUI_API void              SetTexID(ImTextureID id) { TexID = id; }

    IMGUI_API const ImWchar*    GetGlyphRangesDefault();
    IMGUI_API const ImWchar*    GetGlyphRangesGreek();
    IMGUI_API const ImWchar*    GetGlyphRangesKorean();
    IMGUI_API const ImWchar*    GetGlyphRangesJapanese();
    IMGUI_API const ImWchar*    GetGlyphRangesChineseFull();
    IMGUI_API const ImWchar*    GetGlyphRangesChineseSimplifiedCommon();
    IMGUI_API const ImWchar*    GetGlyphRangesCyrillic();
    IMGUI_API const ImWchar*    GetGlyphRangesThai();
    IMGUI_API const ImWchar*    GetGlyphRangesVietnamese();

    bool                        Locked;
    ImFontAtlasFlags            Flags;
    ImTextureID                 TexID;
    int                         TexDesiredWidth;
    int                         TexGlyphPadding;
    bool                        TexReady;
    bool                        TexPixelsUseColors;
    unsigned char*              TexPixelsAlpha8;
    unsigned int*               TexPixelsRGBA32;
    int                         TexWidth;
    int                         TexHeight;
    ImVec2                      TexUvScale;
    ImVec2                      TexUvWhitePixel;
    struct { int Size; ImFont** Data; } Fonts;
};

struct ImFont
{
    float                       FontSize;
    float                       Scale;
    ImVec2                      DisplayOffset;
    ImFontAtlas*                ContainerAtlas;
    float                       Ascent, Descent;
    bool                        DirtyLookupTables;
    float                       FallbackAdvanceX;
    ImWchar                     FallbackChar;
    ImWchar                     EllipsisChar;

    IMGUI_API ImFont();
    IMGUI_API ~ImFont();
    IMGUI_API const ImFontGlyph* FindGlyph(ImWchar c) const;
    IMGUI_API const ImFontGlyph* FindGlyphNoFallback(ImWchar c) const;
    IMGUI_API float             GetCharAdvance(ImWchar c) const;
    IMGUI_API bool              IsLoaded() const { return ContainerAtlas != nullptr; }
    IMGUI_API const char*       GetDebugName() const;
    IMGUI_API ImVec2            CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end = nullptr, const char** remaining = nullptr) const;
    IMGUI_API void              RenderChar(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, ImWchar c) const;
    IMGUI_API void              RenderText(ImDrawList* draw_list, float size, const ImVec2& pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width = 0.0f, bool cpu_fine_clip = false) const;
};

#endif // #ifndef IMGUI_DISABLE
