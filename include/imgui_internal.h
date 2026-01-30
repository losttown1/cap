// dear imgui, v1.89.9
// (internal structures/api)
// Zero Elite - Minimal Internal Header

#pragma once

#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#ifndef IMGUI_DISABLE

//-----------------------------------------------------------------------------
// [SECTION] Forward declarations
//-----------------------------------------------------------------------------

struct ImRect;
struct ImDrawDataBuilder;
struct ImGuiColorMod;
struct ImGuiContext;
struct ImGuiContextHook;
struct ImGuiDataTypeInfo;
struct ImGuiGroupData;
struct ImGuiInputTextState;
struct ImGuiLastItemData;
struct ImGuiMenuColumns;
struct ImGuiNavItemData;
struct ImGuiMetricsConfig;
struct ImGuiNextWindowData;
struct ImGuiNextItemData;
struct ImGuiOldColumnData;
struct ImGuiOldColumns;
struct ImGuiPopupData;
struct ImGuiSettingsHandler;
struct ImGuiStackSizes;
struct ImGuiStyleMod;
struct ImGuiTabBar;
struct ImGuiTabItem;
struct ImGuiTable;
struct ImGuiTableColumn;
struct ImGuiTableInstanceData;
struct ImGuiTableTempData;
struct ImGuiTableSettings;
struct ImGuiTableColumnsSettings;
struct ImGuiWindow;
struct ImGuiWindowTempData;
struct ImGuiWindowSettings;

//-----------------------------------------------------------------------------
// [SECTION] Generic helpers
//-----------------------------------------------------------------------------

#define IM_PI                       3.14159265358979323846f
#define IM_NEWLINE                  "\n"

// Helpers: Hashing
IMGUI_API ImU32         ImHashData(const void* data, size_t data_size, ImU32 seed = 0);
IMGUI_API ImU32         ImHashStr(const char* data, size_t data_size = 0, ImU32 seed = 0);

// Helpers: Bit manipulation
inline bool             ImIsPowerOfTwo(int v)           { return v != 0 && (v & (v - 1)) == 0; }
inline bool             ImIsPowerOfTwo(ImU64 v)         { return v != 0 && (v & (v - 1)) == 0; }
inline int              ImUpperPowerOfTwo(int v)        { v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v++; return v; }

// Helpers: String
IMGUI_API int           ImStricmp(const char* str1, const char* str2);
IMGUI_API int           ImStrnicmp(const char* str1, const char* str2, size_t count);
IMGUI_API void          ImStrncpy(char* dst, const char* src, size_t count);
IMGUI_API char*         ImStrdup(const char* str);
IMGUI_API char*         ImStrdupcpy(char* dst, size_t* p_dst_size, const char* src);
IMGUI_API const char*   ImStrchrRange(const char* str_begin, const char* str_end, char c);
IMGUI_API int           ImStrlenW(const ImWchar* str);
IMGUI_API const char*   ImStreolRange(const char* str, const char* str_end);
IMGUI_API const ImWchar*ImStrbolW(const ImWchar* buf_mid_line, const ImWchar* buf_begin);
IMGUI_API const char*   ImStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end);
IMGUI_API void          ImStrTrimBlanks(char* str);
IMGUI_API const char*   ImStrSkipBlank(const char* str);
IMGUI_API int           ImFormatString(char* buf, size_t buf_size, const char* fmt, ...);
IMGUI_API int           ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args);

// Helpers: Scalar operations
inline float  ImMin(float lhs, float rhs)                                { return lhs < rhs ? lhs : rhs; }
inline float  ImMax(float lhs, float rhs)                                { return lhs >= rhs ? lhs : rhs; }
inline float  ImClamp(float v, float mn, float mx)                       { return (v < mn) ? mn : (v > mx) ? mx : v; }
inline int    ImMin(int lhs, int rhs)                                    { return lhs < rhs ? lhs : rhs; }
inline int    ImMax(int lhs, int rhs)                                    { return lhs >= rhs ? lhs : rhs; }
inline int    ImClamp(int v, int mn, int mx)                             { return (v < mn) ? mn : (v > mx) ? mx : v; }

// Helpers: ImVec2/ImVec4 operators
inline ImVec2 ImMin(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y); }
inline ImVec2 ImMax(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(lhs.x >= rhs.x ? lhs.x : rhs.x, lhs.y >= rhs.y ? lhs.y : rhs.y); }
inline ImVec2 ImClamp(const ImVec2& v, const ImVec2& mn, const ImVec2& mx) { return ImVec2((v.x < mn.x) ? mn.x : (v.x > mx.x) ? mx.x : v.x, (v.y < mn.y) ? mn.y : (v.y > mx.y) ? mx.y : v.y); }
inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, float t)          { return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t); }
inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, const ImVec2& t)  { return ImVec2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y); }
inline ImVec4 ImLerp(const ImVec4& a, const ImVec4& b, float t)          { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t); }
inline float  ImSaturate(float f)                                        { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
inline float  ImLengthSqr(const ImVec2& lhs)                             { return (lhs.x * lhs.x) + (lhs.y * lhs.y); }
inline float  ImLengthSqr(const ImVec4& lhs)                             { return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w); }
inline float  ImInvLength(const ImVec2& lhs, float fail_value)           { float d = (lhs.x * lhs.x) + (lhs.y * lhs.y); if (d > 0.0f) return 1.0f / sqrtf(d); return fail_value; }
inline float  ImFloor(float f)                                           { return (float)(int)(f); }
inline float  ImFloorSigned(float f)                                     { return (float)((f >= 0 || (float)(int)f == f) ? (int)f : (int)f - 1); }
inline ImVec2 ImFloor(const ImVec2& v)                                   { return ImVec2((float)(int)(v.x), (float)(int)(v.y)); }
inline int    ImModPositive(int a, int b)                                { return (a + b) % b; }
inline float  ImDot(const ImVec2& a, const ImVec2& b)                    { return a.x * b.x + a.y * b.y; }
inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a)        { return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a); }
inline float  ImLinearSweep(float current, float target, float speed)    { if (current < target) return ImMin(current + speed, target); if (current > target) return ImMax(current - speed, target); return current; }
inline ImVec2 ImMul(const ImVec2& lhs, const ImVec2& rhs)                { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
inline bool   ImIsFloatAboveGuaranteedIntegerPrecision(float f)          { return f <= -16777216 || f >= 16777216; }

// ImRect (2D axis aligned bounding-box)
struct IMGUI_API ImRect
{
    ImVec2      Min;    // Upper-left
    ImVec2      Max;    // Lower-right

    constexpr ImRect()                                        : Min(0.0f, 0.0f), Max(0.0f, 0.0f)  {}
    constexpr ImRect(const ImVec2& min, const ImVec2& max)    : Min(min), Max(max)                {}
    constexpr ImRect(const ImVec4& v)                         : Min(v.x, v.y), Max(v.z, v.w)      {}
    constexpr ImRect(float x1, float y1, float x2, float y2)  : Min(x1, y1), Max(x2, y2)          {}

    ImVec2      GetCenter() const                   { return ImVec2((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f); }
    ImVec2      GetSize() const                     { return ImVec2(Max.x - Min.x, Max.y - Min.y); }
    float       GetWidth() const                    { return Max.x - Min.x; }
    float       GetHeight() const                   { return Max.y - Min.y; }
    float       GetArea() const                     { return (Max.x - Min.x) * (Max.y - Min.y); }
    ImVec2      GetTL() const                       { return Min; }
    ImVec2      GetTR() const                       { return ImVec2(Max.x, Min.y); }
    ImVec2      GetBL() const                       { return ImVec2(Min.x, Max.y); }
    ImVec2      GetBR() const                       { return Max; }
    bool        Contains(const ImVec2& p) const     { return p.x >= Min.x && p.y >= Min.y && p.x < Max.x && p.y < Max.y; }
    bool        Contains(const ImRect& r) const     { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y; }
    bool        Overlaps(const ImRect& r) const     { return r.Min.y <  Max.y && r.Max.y >  Min.y && r.Min.x <  Max.x && r.Max.x >  Min.x; }
    void        Add(const ImVec2& p)                { if (Min.x > p.x)     Min.x = p.x;     if (Min.y > p.y)     Min.y = p.y;     if (Max.x < p.x)     Max.x = p.x;     if (Max.y < p.y)     Max.y = p.y; }
    void        Add(const ImRect& r)                { if (Min.x > r.Min.x) Min.x = r.Min.x; if (Min.y > r.Min.y) Min.y = r.Min.y; if (Max.x < r.Max.x) Max.x = r.Max.x; if (Max.y < r.Max.y) Max.y = r.Max.y; }
    void        Expand(const float amount)          { Min.x -= amount;   Min.y -= amount;   Max.x += amount;   Max.y += amount; }
    void        Expand(const ImVec2& amount)        { Min.x -= amount.x; Min.y -= amount.y; Max.x += amount.x; Max.y += amount.y; }
    void        Translate(const ImVec2& d)          { Min.x += d.x; Min.y += d.y; Max.x += d.x; Max.y += d.y; }
    void        TranslateX(float dx)                { Min.x += dx; Max.x += dx; }
    void        TranslateY(float dy)                { Min.y += dy; Max.y += dy; }
    void        ClipWith(const ImRect& r)           { Min = ImMax(Min, r.Min); Max = ImMin(Max, r.Max); }
    void        ClipWithFull(const ImRect& r)       { Min = ImClamp(Min, r.Min, r.Max); Max = ImClamp(Max, r.Min, r.Max); }
    void        Floor()                             { Min.x = ImFloor(Min.x); Min.y = ImFloor(Min.y); Max.x = ImFloor(Max.x); Max.y = ImFloor(Max.y); }
    bool        IsInverted() const                  { return Min.x > Max.x || Min.y > Max.y; }
    ImVec4      ToVec4() const                      { return ImVec4(Min.x, Min.y, Max.x, Max.y); }
};

#endif // #ifndef IMGUI_DISABLE
