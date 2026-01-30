#include <windows.h>

#include <algorithm>
#include <chrono>
#include <thread>

#include <d3d11.h>
#include <dwmapi.h>
#include <dxgi.h>

#include "DMA_Engine.h"
#include "ZeroUI.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

static_assert(sizeof(void*) == 8, "Zero Elite requires an x64 build.");

namespace {

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_deviceContext = nullptr;
IDXGISwapChain* g_swapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

ZeroUI g_ui;
DMAEngine g_dma;

void CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

void CreateRenderTarget() {
  ID3D11Texture2D* backBuffer = nullptr;
  if (SUCCEEDED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);
    backBuffer->Release();
  }
}

bool CreateDeviceD3D(HWND hWnd) {
  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
  if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                    featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_swapChain,
                                    &g_device, &featureLevel, &g_deviceContext) != S_OK) {
    return false;
  }

  CreateRenderTarget();
  return true;
}

void CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (g_swapChain) {
    g_swapChain->Release();
    g_swapChain = nullptr;
  }
  if (g_deviceContext) {
    g_deviceContext->Release();
    g_deviceContext = nullptr;
  }
  if (g_device) {
    g_device->Release();
    g_device = nullptr;
  }
}

void SetClickThrough(HWND hwnd, bool enable) {
  LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  if (enable) {
    exStyle |= WS_EX_TRANSPARENT;
  } else {
    exStyle &= ~WS_EX_TRANSPARENT;
  }
  SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
    return 1;
  }

  switch (msg) {
    case WM_SIZE:
      if (g_device != nullptr && wParam != SIZE_MINIMIZED) {
        CleanupRenderTarget();
        g_swapChain->ResizeBuffers(0, static_cast<UINT>(LOWORD(lParam)),
                                   static_cast<UINT>(HIWORD(lParam)), DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
        ImGui_ImplDX11_InvalidateDeviceObjects();
      }
      return 0;
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU) {
        return 0;
      }
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      break;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

}  // namespace

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_CLASSDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"ZeroEliteOverlay";
  RegisterClassEx(&wc);

  const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  HWND hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, wc.lpszClassName,
                             L"Zero Elite", WS_POPUP, 0, 0, screenWidth, screenHeight, nullptr, nullptr,
                             wc.hInstance, nullptr);

  if (!hwnd) {
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 1;
  }

  SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
  MARGINS margins = {-1};
  DwmExtendFrameIntoClientArea(hwnd, &margins);

  if (!CreateDeviceD3D(hwnd)) {
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 1;
  }

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  g_ui.Initialize(hwnd, g_device, g_deviceContext, g_swapChain);
  g_dma.Initialize();

  bool clickThrough = true;
  SetClickThrough(hwnd, clickThrough);

  MSG msg = {};
  auto lastFrame = std::chrono::steady_clock::now();

  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      continue;
    }

    if (GetAsyncKeyState(VK_INSERT) & 1) {
      g_ui.ToggleMenu();
      clickThrough = !g_ui.GetSettings().showMenu;
      SetClickThrough(hwnd, clickThrough);
    }

    g_deviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    const float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    g_deviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);

    g_ui.NewFrame();
    g_ui.Render();

    g_swapChain->Present(1, 0);

    const float targetHz = std::max(15.0f, g_ui.GetSettings().refreshRate);
    const auto frameTarget = std::chrono::duration<float, std::milli>(1000.0f / targetHz);
    const auto now = std::chrono::steady_clock::now();
    const auto frameElapsed = now - lastFrame;
    if (frameElapsed < frameTarget) {
      const auto sleepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameTarget - frameElapsed);
      if (sleepDuration.count() > 0) {
        std::this_thread::sleep_for(sleepDuration);
      }
    }
    lastFrame = std::chrono::steady_clock::now();
  }

  g_dma.Shutdown();
  g_ui.Shutdown();

  CleanupDeviceD3D();
  DestroyWindow(hwnd);
  UnregisterClass(wc.lpszClassName, wc.hInstance);

  return 0;
}
