#pragma once

#include <cstdarg>
#include <cstdint>

namespace ImGui {

struct ImVec2 {
  float x;
  float y;
  ImVec2() : x(0.0f), y(0.0f) {}
  ImVec2(float _x, float _y) : x(_x), y(_y) {}
};

struct ImVec4 {
  float x;
  float y;
  float z;
  float w;
  ImVec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
  ImVec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

struct ImDrawData {
  void* Reserved;
};

struct IO {
  ImVec2 DisplaySize;
  float DeltaTime;
  bool MouseDown[5];
  ImVec2 MousePos;
  float MouseWheel;
  bool WantCaptureMouse;
  bool WantCaptureKeyboard;
  const char* IniFilename;
};

IO& GetIO();

void CreateContext();
void DestroyContext();
void NewFrame();
void Render();
ImDrawData* GetDrawData();

void SetNextWindowPos(const ImVec2& pos, int cond = 0, const ImVec2& pivot = ImVec2(0.0f, 0.0f));
void SetNextWindowSize(const ImVec2& size, int cond = 0);

bool Begin(const char* name, bool* p_open = nullptr, int flags = 0);
void End();

void Text(const char* fmt, ...);
bool Checkbox(const char* label, bool* v);
bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", int flags = 0);

}  // namespace ImGui

enum ImGuiCond_ {
  ImGuiCond_None = 0,
  ImGuiCond_Always = 1 << 0,
  ImGuiCond_Once = 1 << 1
};

enum ImGuiWindowFlags_ {
  ImGuiWindowFlags_None = 0,
  ImGuiWindowFlags_NoTitleBar = 1 << 0,
  ImGuiWindowFlags_NoResize = 1 << 1,
  ImGuiWindowFlags_NoMove = 1 << 2,
  ImGuiWindowFlags_NoScrollbar = 1 << 3,
  ImGuiWindowFlags_NoCollapse = 1 << 4
};
