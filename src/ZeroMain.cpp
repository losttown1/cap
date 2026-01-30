// PROJECT ZERO - Professional DMA Radar Overlay
// Modern implementation similar to market DMA tools
// Full transparent overlay with working radar and menu

#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <thread>
#include <chrono>
#include <iostream>

#include "../include/imgui.h"
#include "../include/imgui_impl_win32.h"
#include "../include/imgui_impl_dx11.h"

#include "ZeroUI.hpp"
#include "DMA_Engine.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Globals
ID3D11Device* g_Device = nullptr;
ID3D11DeviceContext* g_Context = nullptr;
IDXGISwapChain* g_SwapChain = nullptr;
ID3D11RenderTargetView* g_RenderTarget = nullptr;
HWND g_Hwnd = nullptr;
WNDCLASSEXW g_WndClass = {};

int g_Width = 0;
int g_Height = 0;
bool g_Running = true;
bool g_ShowMenu = true;
bool g_ShowOverlay = true;

// Key states for debounce
bool g_KeyF1 = false;
bool g_KeyInsert = false;
bool g_KeyEnd = false;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DESTROY:
        g_Running = false;
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (g_Device && wParam != SIZE_MINIMIZED)
        {
            if (g_RenderTarget) { g_RenderTarget->Release(); g_RenderTarget = nullptr; }
            g_SwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* backBuffer;
            g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
            if (backBuffer) {
                g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
                backBuffer->Release();
            }
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool InitWindow()
{
    g_Width = GetSystemMetrics(SM_CXSCREEN);
    g_Height = GetSystemMetrics(SM_CYSCREEN);

    g_WndClass = {};
    g_WndClass.cbSize = sizeof(WNDCLASSEXW);
    g_WndClass.style = CS_CLASSDC;
    g_WndClass.lpfnWndProc = WndProc;
    g_WndClass.hInstance = GetModuleHandleW(nullptr);
    g_WndClass.lpszClassName = L"ProjectZeroOverlay";
    RegisterClassExW(&g_WndClass);

    g_Hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        g_WndClass.lpszClassName,
        L"",
        WS_POPUP,
        0, 0, g_Width, g_Height,
        nullptr, nullptr, g_WndClass.hInstance, nullptr
    );

    if (!g_Hwnd) return false;

    // Make fully transparent
    SetLayeredWindowAttributes(g_Hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    
    MARGINS margin = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(g_Hwnd, &margin);

    ShowWindow(g_Hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_Hwnd);

    return true;
}

bool InitD3D()
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = g_Width;
    sd.BufferDesc.Height = g_Height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_Hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_SwapChain, &g_Device, &featureLevel, &g_Context)))
    {
        return false;
    }

    ID3D11Texture2D* backBuffer;
    g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) {
        g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
        backBuffer->Release();
    }

    return true;
}

void Cleanup()
{
    if (g_RenderTarget) g_RenderTarget->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_Context) g_Context->Release();
    if (g_Device) g_Device->Release();
    DestroyWindow(g_Hwnd);
    UnregisterClassW(g_WndClass.lpszClassName, g_WndClass.hInstance);
}

bool KeyPressed(int vk, bool& state)
{
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    if (down && !state) { state = true; return true; }
    if (!down) state = false;
    return false;
}

void SetClickthrough(bool clickthrough)
{
    LONG_PTR style = GetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE);
    if (clickthrough)
        style |= WS_EX_TRANSPARENT;
    else
        style &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE, style);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // Console
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << R"(
  ____  ____   ___      _ _____ ____ _____   __________ ____   ___  
 |  _ \|  _ \ / _ \    | | ____/ ___|_   _| |__  / ____|  _ \ / _ \ 
 | |_) | |_) | | | |_  | |  _|| |     | |     / /|  _| | |_) | | | |
 |  __/|  _ <| |_| | |_| | |__| |___  | |    / /_| |___|  _ <| |_| |
 |_|   |_| \_\\___/ \___/|_____\____| |_|   /____|_____|_| \_\\___/ 
                                                                    
)" << std::endl;
    std::cout << "  [F1]     - Toggle Overlay" << std::endl;
    std::cout << "  [INSERT] - Toggle Menu" << std::endl;
    std::cout << "  [END]    - Exit" << std::endl;
    std::cout << std::endl;

    // Minimize console after 3 seconds
    std::thread([]() {
        Sleep(3000);
        ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
    }).detach();

    if (!InitWindow())
    {
        std::cout << "[ERROR] Failed to create window" << std::endl;
        return 1;
    }
    std::cout << "[OK] Window: " << g_Width << "x" << g_Height << std::endl;

    if (!InitD3D())
    {
        std::cout << "[ERROR] Failed to init DirectX" << std::endl;
        Cleanup();
        return 1;
    }
    std::cout << "[OK] DirectX 11" << std::endl;

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(g_Hwnd);
    ImGui_ImplDX11_Init(g_Device, g_Context);
    
    InitializeZeroUI();
    std::cout << "[OK] ImGui" << std::endl;

    if (InitializeZeroDMA())
        std::cout << "[OK] DMA Engine" << std::endl;
    else
        std::cout << "[--] DMA Simulation Mode" << std::endl;

    std::cout << std::endl << ">>> RUNNING <<<" << std::endl;

    // Main loop
    while (g_Running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                g_Running = false;
        }
        if (!g_Running) break;

        // Hotkeys
        if (KeyPressed(VK_END, g_KeyEnd))
        {
            g_Running = false;
            break;
        }
        
        if (KeyPressed(VK_F1, g_KeyF1))
        {
            g_ShowOverlay = !g_ShowOverlay;
            std::cout << "[F1] Overlay: " << (g_ShowOverlay ? "ON" : "OFF") << std::endl;
        }
        
        if (KeyPressed(VK_INSERT, g_KeyInsert))
        {
            g_ShowMenu = !g_ShowMenu;
            SetClickthrough(!g_ShowMenu);
            std::cout << "[INS] Menu: " << (g_ShowMenu ? "OPEN" : "CLOSED") << std::endl;
        }

        // Render
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_ShowOverlay)
        {
            // Radar always draws if esp enabled
            if (esp_enabled)
                RenderRadarOverlay();

            // Menu
            if (g_ShowMenu)
                RenderZeroMenu();
        }

        ImGui::Render();

        const float clear[4] = { 0, 0, 0, 0 };
        g_Context->OMSetRenderTargets(1, &g_RenderTarget, nullptr);
        g_Context->ClearRenderTargetView(g_RenderTarget, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_SwapChain->Present(1, 0);
    }

    // Cleanup
    ShutdownZeroDMA();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    Cleanup();
    FreeConsole();

    return 0;
}

int main() { return wWinMain(nullptr, nullptr, nullptr, 0); }
