#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

struct OverlaySettings {
  bool showMenu;
  bool espEnabled;
  bool aimbotEnabled;
  float refreshRate;
};

class ZeroUI {
 public:
  bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);
  void Shutdown();
  void NewFrame();
  void Render();
  void ToggleMenu();
  const OverlaySettings& GetSettings() const;

 private:
  OverlaySettings settings_{true, false, false, 60.0f};
  bool initialized_ = false;
};
