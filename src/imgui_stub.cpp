#include "../include/imgui.h"
#include <cstdio>

// Stub implementation for ImGui
namespace ImGui {
    struct Context {
        IO io;
    } g_Ctx;

    void CreateContext() {}
    void DestroyContext() {}
    void StyleColorsDark() {}
    void NewFrame() {}
    void Render() {}
    void Text(const char* fmt, ...) {}
    void Checkbox(const char* label, bool* v) {}
    void SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, int flags) {}
    bool Begin(const char* name, bool* p_open, int flags) { return true; }
    void End() {}
    
    IO& GetIO() {
        return g_Ctx.io;
    }

    struct DrawData {
    };
    DrawData* GetDrawData() { return nullptr; }
    
    void SetNextWindowPos(const struct ImVec2& pos, int cond, const struct ImVec2& pivot) {}
    void TextColored(const struct ImVec4& col, const char* fmt, ...) {}
}

// Struct definitions that might be needed
struct ImVec2 { float x, y; };
struct ImVec4 { float x, y, z, w; };
