#pragma once
#include <windows.h>
#include <d3d11.h>
#include "../include/imgui.h"
#include "../include/imgui_impl_dx11.h"
#include "../include/imgui_impl_win32.h"

class ZeroUI {
public:
    ZeroUI();
    ~ZeroUI();

    bool Init(HINSTANCE hInstance);
    void Render();
    bool Tick();
    void Cleanup();

    // UI State
    bool bShowMenu = true;
    bool bESPEnabled = false;
    bool bAimbotEnabled = false;
    float fRefreshRate = 60.0f;

private:
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void SetInputHandling(bool interactable);

    HWND hwnd = nullptr;
    WNDCLASSEX wc = {};
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    IDXGISwapChain* g_pSwapChain = nullptr;
    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
};
