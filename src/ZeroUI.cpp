#include "ZeroUI.h"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <wrl/client.h>

#include <d2d1_1.h>
#include <dwrite.h>

#include "DMA_Engine.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

namespace {

struct D2DResources {
  ComPtr<ID2D1Factory1> factory;
  ComPtr<ID2D1Device> device;
  ComPtr<ID2D1DeviceContext> context;
  ComPtr<ID2D1Bitmap1> targetBitmap;
  ComPtr<IDWriteFactory> dwriteFactory;
  ComPtr<IDWriteTextFormat> textFormat;
  ComPtr<ID2D1SolidColorBrush> brush;
};

D2DResources g_d2d;
ComPtr<ID3D11Device> g_d3dDevice;
ComPtr<ID3D11DeviceContext> g_d3dContext;
ComPtr<IDXGISwapChain> g_swapChain;
HWND g_hwnd = nullptr;
bool g_d2dFrameBegun = false;

bool CreateD2DTarget() {
  if (!g_d2d.context || !g_swapChain) {
    return false;
  }

  g_d2d.targetBitmap.Reset();

  ComPtr<IDXGISurface> surface;
  if (FAILED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface)))) {
    return false;
  }

  FLOAT dpiX = 96.0f;
  FLOAT dpiY = 96.0f;
  if (g_d2d.factory) {
    g_d2d.factory->GetDesktopDpi(&dpiX, &dpiY);
  }

  D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
      dpiX, dpiY);

  if (FAILED(g_d2d.context->CreateBitmapFromDxgiSurface(surface.Get(), &props, &g_d2d.targetBitmap))) {
    return false;
  }

  g_d2d.context->SetTarget(g_d2d.targetBitmap.Get());
  g_d2d.context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
  g_d2d.context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
  return true;
}

std::wstring ToWide(const char* text) {
  if (!text) {
    return L"";
  }
  int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
  if (length <= 0) {
    return L"";
  }
  std::wstring wide(static_cast<size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), length);
  if (!wide.empty() && wide.back() == L'\0') {
    wide.pop_back();
  }
  return wide;
}

}  // namespace

bool ZeroUI::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain) {
  if (initialized_) {
    return true;
  }

  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;

  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(device, context);
  ImGui_ImplDX11_SetSwapChain(swapChain);

  settings_.showMenu = true;
  settings_.espEnabled = false;
  settings_.aimbotEnabled = false;
  settings_.refreshRate = 60.0f;

  initialized_ = true;
  return true;
}

void ZeroUI::Shutdown() {
  if (!initialized_) {
    return;
  }
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
  initialized_ = false;
}

void ZeroUI::NewFrame() {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void ZeroUI::Render() {
  if (settings_.showMenu) {
    ImGui::SetNextWindowPos(ImGui::ImVec2(40.0f, 40.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImGui::ImVec2(420.0f, 260.0f), ImGuiCond_Always);
    if (ImGui::Begin("Zero Elite - Overlay Control", &settings_.showMenu,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
      ImGui::Text("DMA Target PID: %u", DMAEngine::kTargetPid);
      ImGui::Text("DMA Handle: 0x%llX", static_cast<unsigned long long>(DMAEngine::kTargetHandle));
      ImGui::Checkbox("ESP", &settings_.espEnabled);
      ImGui::Checkbox("Aimbot", &settings_.aimbotEnabled);
      ImGui::SliderFloat("Refresh Rate", &settings_.refreshRate, 15.0f, 240.0f, "%.0f Hz");
      ImGui::Text("Toggle menu with INSERT.");
    }
    ImGui::End();
  }

  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ZeroUI::ToggleMenu() {
  settings_.showMenu = !settings_.showMenu;
}

const OverlaySettings& ZeroUI::GetSettings() const {
  return settings_;
}

namespace ImGui {

namespace {

IO g_io = {};
ImDrawData g_drawData = {};

bool g_prevMouseDown[5] = {};
bool g_mouseClicked[5] = {};
bool g_frameActive = false;
bool g_windowOpen = false;
bool g_hasNextWindowPos = false;
bool g_hasNextWindowSize = false;
ImVec2 g_nextWindowPos;
ImVec2 g_nextWindowSize;
uint32_t g_activeId = 0;

struct WindowState {
  ImVec2 pos;
  ImVec2 size;
  ImVec2 cursor;
  float lineHeight;
  float padding;
  float spacing;
};

WindowState g_window = {};

uint32_t HashLabel(const char* label) {
  uint32_t hash = 2166136261u;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(label); *p; ++p) {
    hash ^= *p;
    hash *= 16777619u;
  }
  return hash;
}

bool IsMouseHoveringRect(const ImVec2& min, const ImVec2& max) {
  return g_io.MousePos.x >= min.x && g_io.MousePos.x <= max.x && g_io.MousePos.y >= min.y &&
         g_io.MousePos.y <= max.y;
}

void DrawFilledRect(const ImVec2& pos, const ImVec2& size, const D2D1_COLOR_F& color) {
  if (!g_d2d.context || !g_d2d.brush) {
    return;
  }
  g_d2d.brush->SetColor(color);
  D2D1_RECT_F rect = D2D1::RectF(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
  g_d2d.context->FillRectangle(rect, g_d2d.brush.Get());
}

void DrawRect(const ImVec2& pos, const ImVec2& size, const D2D1_COLOR_F& color, float thickness) {
  if (!g_d2d.context || !g_d2d.brush) {
    return;
  }
  g_d2d.brush->SetColor(color);
  D2D1_RECT_F rect = D2D1::RectF(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
  g_d2d.context->DrawRectangle(rect, g_d2d.brush.Get(), thickness);
}

void DrawTextLabel(const ImVec2& pos, const char* text, const D2D1_COLOR_F& color) {
  if (!g_d2d.context || !g_d2d.brush || !g_d2d.textFormat) {
    return;
  }
  std::wstring wide = ToWide(text);
  if (wide.empty()) {
    return;
  }
  g_d2d.brush->SetColor(color);
  D2D1_RECT_F layout = D2D1::RectF(pos.x, pos.y, g_io.DisplaySize.x, g_io.DisplaySize.y);
  g_d2d.context->DrawTextW(wide.c_str(), static_cast<UINT32>(wide.size()), g_d2d.textFormat.Get(), layout,
                           g_d2d.brush.Get());
}

}  // namespace

IO& GetIO() {
  return g_io;
}

void CreateContext() {
  g_io = {};
  g_io.DisplaySize = ImVec2(0.0f, 0.0f);
  g_io.DeltaTime = 1.0f / 60.0f;
  g_io.MousePos = ImVec2(-1.0f, -1.0f);
  g_io.IniFilename = nullptr;
}

void DestroyContext() {
  g_io = {};
}

void NewFrame() {
  g_frameActive = true;
  g_windowOpen = false;
  g_mouseClicked[0] = g_io.MouseDown[0] && !g_prevMouseDown[0];
  g_mouseClicked[1] = g_io.MouseDown[1] && !g_prevMouseDown[1];
  g_mouseClicked[2] = g_io.MouseDown[2] && !g_prevMouseDown[2];
  g_mouseClicked[3] = g_io.MouseDown[3] && !g_prevMouseDown[3];
  g_mouseClicked[4] = g_io.MouseDown[4] && !g_prevMouseDown[4];

  if (!g_io.MouseDown[0]) {
    g_activeId = 0;
  }
}

void Render() {
  for (int i = 0; i < 5; ++i) {
    g_prevMouseDown[i] = g_io.MouseDown[i];
  }
  g_frameActive = false;
}

ImDrawData* GetDrawData() {
  return &g_drawData;
}

void SetNextWindowPos(const ImVec2& pos, int, const ImVec2&) {
  g_nextWindowPos = pos;
  g_hasNextWindowPos = true;
}

void SetNextWindowSize(const ImVec2& size, int) {
  g_nextWindowSize = size;
  g_hasNextWindowSize = true;
}

bool Begin(const char* name, bool* p_open, int) {
  if (p_open && !*p_open) {
    return false;
  }

  ImVec2 pos = g_hasNextWindowPos ? g_nextWindowPos : ImVec2(40.0f, 40.0f);
  ImVec2 size = g_hasNextWindowSize ? g_nextWindowSize : ImVec2(420.0f, 260.0f);
  g_hasNextWindowPos = false;
  g_hasNextWindowSize = false;

  g_window.pos = pos;
  g_window.size = size;
  g_window.padding = 12.0f;
  g_window.spacing = 8.0f;
  g_window.lineHeight = 24.0f;
  g_window.cursor = ImVec2(pos.x + g_window.padding, pos.y + 42.0f);

  const D2D1_COLOR_F kBg = D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.92f);
  const D2D1_COLOR_F kBorder = D2D1::ColorF(0.18f, 0.22f, 0.3f, 0.95f);
  const D2D1_COLOR_F kHeader = D2D1::ColorF(0.1f, 0.18f, 0.28f, 0.98f);
  const D2D1_COLOR_F kText = D2D1::ColorF(0.95f, 0.96f, 0.98f, 1.0f);

  DrawFilledRect(pos, size, kBg);
  DrawRect(pos, size, kBorder, 1.0f);
  DrawFilledRect(pos, ImVec2(size.x, 32.0f), kHeader);
  DrawTextLabel(ImVec2(pos.x + g_window.padding, pos.y + 6.0f), name, kText);

  g_windowOpen = true;
  return true;
}

void End() {
  g_windowOpen = false;
}

void Text(const char* fmt, ...) {
  if (!g_windowOpen) {
    return;
  }

  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  const D2D1_COLOR_F kText = D2D1::ColorF(0.93f, 0.95f, 0.98f, 1.0f);
  DrawTextLabel(g_window.cursor, buffer, kText);
  g_window.cursor.y += g_window.lineHeight + g_window.spacing;
}

bool Checkbox(const char* label, bool* v) {
  if (!g_windowOpen || !v) {
    return false;
  }

  const float rowHeight = g_window.lineHeight;
  const float contentWidth = g_window.size.x - g_window.padding * 2.0f;
  const ImVec2 rowMin = g_window.cursor;
  const ImVec2 rowMax = ImVec2(rowMin.x + contentWidth, rowMin.y + rowHeight);

  const D2D1_COLOR_F kRowBg = D2D1::ColorF(0.12f, 0.14f, 0.18f, 0.9f);
  const D2D1_COLOR_F kRowHover = D2D1::ColorF(0.16f, 0.19f, 0.26f, 0.95f);
  const D2D1_COLOR_F kBorder = D2D1::ColorF(0.22f, 0.26f, 0.34f, 0.95f);
  const D2D1_COLOR_F kAccent = D2D1::ColorF(0.26f, 0.62f, 1.0f, 1.0f);
  const D2D1_COLOR_F kText = D2D1::ColorF(0.93f, 0.95f, 0.98f, 1.0f);

  const bool hovered = IsMouseHoveringRect(rowMin, rowMax);
  if (hovered && g_mouseClicked[0]) {
    *v = !*v;
  }

  DrawFilledRect(rowMin, ImVec2(contentWidth, rowHeight), hovered ? kRowHover : kRowBg);

  const float boxSize = 16.0f;
  const ImVec2 boxPos = ImVec2(rowMin.x + 6.0f, rowMin.y + (rowHeight - boxSize) * 0.5f);
  DrawRect(boxPos, ImVec2(boxSize, boxSize), kBorder, 1.0f);
  if (*v) {
    DrawFilledRect(ImVec2(boxPos.x + 3.0f, boxPos.y + 3.0f), ImVec2(boxSize - 6.0f, boxSize - 6.0f),
                   kAccent);
  }

  DrawTextLabel(ImVec2(boxPos.x + boxSize + 8.0f, rowMin.y + 3.0f), label, kText);
  g_window.cursor.y += rowHeight + g_window.spacing;
  return hovered && g_mouseClicked[0];
}

bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, int) {
  if (!g_windowOpen || !v || v_min >= v_max) {
    return false;
  }

  const float rowHeight = g_window.lineHeight;
  const float contentWidth = g_window.size.x - g_window.padding * 2.0f;
  const float labelWidth = 120.0f;
  const float sliderHeight = 10.0f;
  const float sliderWidth = std::max(120.0f, contentWidth - labelWidth - 56.0f);

  ImVec2 rowPos = g_window.cursor;
  ImVec2 sliderPos = ImVec2(rowPos.x + labelWidth + 10.0f, rowPos.y + (rowHeight - sliderHeight) * 0.5f);
  ImVec2 sliderMax = ImVec2(sliderPos.x + sliderWidth, rowPos.y + rowHeight);

  const D2D1_COLOR_F kBarBg = D2D1::ColorF(0.12f, 0.14f, 0.18f, 0.9f);
  const D2D1_COLOR_F kBorder = D2D1::ColorF(0.22f, 0.26f, 0.34f, 0.95f);
  const D2D1_COLOR_F kAccent = D2D1::ColorF(0.26f, 0.62f, 1.0f, 1.0f);
  const D2D1_COLOR_F kText = D2D1::ColorF(0.93f, 0.95f, 0.98f, 1.0f);

  DrawTextLabel(rowPos, label, kText);
  DrawFilledRect(sliderPos, ImVec2(sliderWidth, sliderHeight), kBarBg);
  DrawRect(sliderPos, ImVec2(sliderWidth, sliderHeight), kBorder, 1.0f);

  const uint32_t id = HashLabel(label);
  const bool hovered = IsMouseHoveringRect(sliderPos, sliderMax);
  if (hovered && g_mouseClicked[0]) {
    g_activeId = id;
  }

  if (g_activeId == id && g_io.MouseDown[0]) {
    float t = (g_io.MousePos.x - sliderPos.x) / sliderWidth;
    t = std::max(0.0f, std::min(1.0f, t));
    *v = v_min + t * (v_max - v_min);
  }

  float fillT = (*v - v_min) / (v_max - v_min);
  fillT = std::max(0.0f, std::min(1.0f, fillT));
  DrawFilledRect(sliderPos, ImVec2(sliderWidth * fillT, sliderHeight), kAccent);

  char valueText[64];
  snprintf(valueText, sizeof(valueText), format ? format : "%.3f", *v);
  DrawTextLabel(ImVec2(sliderPos.x + sliderWidth + 8.0f, rowPos.y + 3.0f), valueText, kText);

  g_window.cursor.y += rowHeight + g_window.spacing;
  return g_activeId == id;
}

}  // namespace ImGui

bool ImGui_ImplWin32_Init(void* hwnd) {
  g_hwnd = static_cast<HWND>(hwnd);
  return g_hwnd != nullptr;
}

void ImGui_ImplWin32_Shutdown() {
  g_hwnd = nullptr;
}

void ImGui_ImplWin32_NewFrame() {
  ImGui::IO& io = ImGui::GetIO();

  RECT rect = {};
  if (g_hwnd && GetClientRect(g_hwnd, &rect)) {
    io.DisplaySize = ImGui::ImVec2(static_cast<float>(rect.right - rect.left),
                                   static_cast<float>(rect.bottom - rect.top));
  }

  POINT mousePos = {};
  if (g_hwnd && GetCursorPos(&mousePos) && ScreenToClient(g_hwnd, &mousePos)) {
    io.MousePos = ImGui::ImVec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
  } else {
    io.MousePos = ImGui::ImVec2(-1.0f, -1.0f);
  }

  io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
  io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

  static LARGE_INTEGER ticksPerSecond = {};
  static LARGE_INTEGER lastTime = {};
  if (ticksPerSecond.QuadPart == 0) {
    QueryPerformanceFrequency(&ticksPerSecond);
    QueryPerformanceCounter(&lastTime);
  }

  LARGE_INTEGER currentTime = {};
  QueryPerformanceCounter(&currentTime);
  if (lastTime.QuadPart > 0) {
    io.DeltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) /
                   static_cast<float>(ticksPerSecond.QuadPart);
  } else {
    io.DeltaTime = 1.0f / 60.0f;
  }
  lastTime = currentTime;
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (!hWnd) {
    return 0;
  }

  ImGui::IO& io = ImGui::GetIO();
  switch (msg) {
    case WM_LBUTTONDOWN:
      io.MouseDown[0] = true;
      return 1;
    case WM_LBUTTONUP:
      io.MouseDown[0] = false;
      return 1;
    case WM_RBUTTONDOWN:
      io.MouseDown[1] = true;
      return 1;
    case WM_RBUTTONUP:
      io.MouseDown[1] = false;
      return 1;
    case WM_MOUSEWHEEL:
      io.MouseWheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / 120.0f;
      return 1;
    case WM_MOUSEMOVE:
      io.MousePos = ImGui::ImVec2(static_cast<float>(GET_X_LPARAM(lParam)),
                                  static_cast<float>(GET_Y_LPARAM(lParam)));
      return 1;
    default:
      break;
  }
  return 0;
}

bool ImGui_ImplDX11_Init(ID3D11Device* device, ID3D11DeviceContext* device_context) {
  if (!device || !device_context) {
    return false;
  }

  g_d3dDevice = device;
  g_d3dContext = device_context;

  if (!g_d2d.factory) {
    D2D1_FACTORY_OPTIONS options = {};
#if defined(_DEBUG)
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options,
                                 reinterpret_cast<void**>(g_d2d.factory.GetAddressOf())))) {
      return false;
    }
  }

  ComPtr<IDXGIDevice> dxgiDevice;
  if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
    return false;
  }

  if (FAILED(g_d2d.factory->CreateDevice(dxgiDevice.Get(), &g_d2d.device))) {
    return false;
  }
  if (FAILED(g_d2d.device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &g_d2d.context))) {
    return false;
  }

  if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                 reinterpret_cast<IUnknown**>(g_d2d.dwriteFactory.GetAddressOf())))) {
    return false;
  }

  if (FAILED(g_d2d.dwriteFactory->CreateTextFormat(
          L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
          DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &g_d2d.textFormat))) {
    return false;
  }
  g_d2d.textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

  if (FAILED(g_d2d.context->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
                                                  &g_d2d.brush))) {
    return false;
  }

  return true;
}

void ImGui_ImplDX11_Shutdown() {
  g_d2d.targetBitmap.Reset();
  g_d2d.brush.Reset();
  g_d2d.textFormat.Reset();
  g_d2d.dwriteFactory.Reset();
  g_d2d.context.Reset();
  g_d2d.device.Reset();
  g_d2d.factory.Reset();

  g_swapChain.Reset();
  g_d3dContext.Reset();
  g_d3dDevice.Reset();
  g_d2dFrameBegun = false;
}

void ImGui_ImplDX11_NewFrame() {
  if (!g_d2d.context) {
    return;
  }

  if (!g_d2d.targetBitmap) {
    CreateD2DTarget();
  }

  if (!g_d2d.targetBitmap) {
    return;
  }

  g_d2d.context->BeginDraw();
  g_d2d.context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
  g_d2dFrameBegun = true;
}

void ImGui_ImplDX11_RenderDrawData(ImGui::ImDrawData*) {
  if (!g_d2d.context || !g_d2dFrameBegun) {
    return;
  }

  HRESULT hr = g_d2d.context->EndDraw();
  g_d2dFrameBegun = false;
  if (hr == D2DERR_RECREATE_TARGET) {
    ImGui_ImplDX11_InvalidateDeviceObjects();
  }
}

void ImGui_ImplDX11_InvalidateDeviceObjects() {
  if (g_d2d.context) {
    g_d2d.context->SetTarget(nullptr);
  }
  g_d2d.targetBitmap.Reset();
}

void ImGui_ImplDX11_SetSwapChain(IDXGISwapChain* swap_chain) {
  g_swapChain = swap_chain;
  ImGui_ImplDX11_InvalidateDeviceObjects();
  CreateD2DTarget();
}
