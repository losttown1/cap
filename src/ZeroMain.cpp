// ZeroMain.cpp - Zero Elite Main Entry Point
// DirectX 11 Transparent Overlay with ImGui
// FIXED: Input blocking, menu rendering, radar display

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <iostream>
#include <chrono>
#include <thread>

// ImGui includes
#include "../include/imgui.h"
#include "../include/imgui_impl_dx11.h"
#include "../include/imgui_impl_win32.h"

// Zero Elite includes
#include "ZeroUI.hpp"
#include "DMA_Engine.hpp"

// Link DirectX and system libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global variables
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static HWND                     g_hWnd = nullptr;
static bool                     g_bRunning = true;
static WNDCLASSEXW              g_wc = {};

// Overlay state
static bool g_OverlayVisible = true;   // F1 to toggle ALL overlay
static bool g_MenuVisible = true;      // INSERT to toggle menu only

// Debounce
static bool g_F1Pressed = false;
static bool g_InsertPressed = false;
static bool g_EndPressed = false;

// Overlay configuration
constexpr const wchar_t* WINDOW_CLASS_NAME = L"ZeroOverlay";
constexpr const wchar_t* WINDOW_TITLE = L"";

// Function declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Create DirectX 11 device and swap chain
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevelArray, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
    );

    if (FAILED(hr)) {
        std::cerr << "[ERROR] D3D11CreateDeviceAndSwapChain failed: 0x" << std::hex << hr << std::endl;
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Set window click-through state
void SetClickThrough(bool clickThrough)
{
    LONG_PTR exStyle = GetWindowLongPtrW(g_hWnd, GWL_EXSTYLE);
    
    if (clickThrough) {
        exStyle |= WS_EX_TRANSPARENT;
    } else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    
    SetWindowLongPtrW(g_hWnd, GWL_EXSTYLE, exStyle);
}

// Window procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Simple key check with debounce
bool KeyPressed(int vKey, bool& state)
{
    bool down = (GetAsyncKeyState(vKey) & 0x8000) != 0;
    if (down && !state) {
        state = true;
        return true;
    }
    if (!down) state = false;
    return false;
}

// Minimize console window after delay (not hide, so program keeps running)
void MinimizeConsoleAfterDelay(int seconds)
{
    Sleep(seconds * 1000);
    HWND console = GetConsoleWindow();
    if (console) ShowWindow(console, SW_MINIMIZE);
}

// Main entry point
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    // Console for debugging (will hide after 3 seconds)
    AllocConsole();
    FILE* f; freopen_s(&f, "CONOUT$", "w", stdout); freopen_s(&f, "CONOUT$", "w", stderr);

    std::cout << "============================================" << std::endl;
    std::cout << "   PROJECT ZERO - BO6 Radar Overlay" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;
    std::cout << "CONTROLS:" << std::endl;
    std::cout << "  F1     = Hide/Show ALL overlay" << std::endl;
    std::cout << "  INSERT = Hide/Show menu only" << std::endl;
    std::cout << "  END    = Exit program" << std::endl;
    std::cout << std::endl;
    std::cout << ">>> Console will minimize in 3 seconds <<<" << std::endl;
    std::cout << std::endl;

    // Register window class
    g_wc.cbSize = sizeof(WNDCLASSEXW);
    g_wc.style = CS_HREDRAW | CS_VREDRAW;
    g_wc.lpfnWndProc = WndProc;
    g_wc.hInstance = hInstance;
    g_wc.lpszClassName = WINDOW_CLASS_NAME;
    RegisterClassExW(&g_wc);

    // Get screen size
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    // Create transparent overlay window
    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        WINDOW_CLASS_NAME, WINDOW_TITLE,
        WS_POPUP,
        0, 0, screenW, screenH,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hWnd) {
        std::cerr << "[ERROR] CreateWindowExW failed" << std::endl;
        return 1;
    }

    // Make window transparent
    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // Extend frame into client area for transparency
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(g_hWnd, &margins);

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    std::cout << "[OK] Window created: " << screenW << "x" << screenH << std::endl;

    // Initialize DirectX
    if (!CreateDeviceD3D(g_hWnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(WINDOW_CLASS_NAME, hInstance);
        return 1;
    }
    std::cout << "[OK] DirectX 11 initialized" << std::endl;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    
    // Set display size
    io.DisplaySize = ImVec2((float)screenW, (float)screenH);

    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    std::cout << "[OK] ImGui initialized" << std::endl;

    // Initialize UI and DMA
    InitializeZeroUI();
    
    if (InitializeZeroDMA()) {
        std::cout << "[OK] DMA initialized" << std::endl;
    } else {
        std::cout << "[WARN] DMA failed - overlay only mode" << std::endl;
    }

    std::cout << std::endl;
    std::cout << ">>> Overlay running! <<<" << std::endl;
    std::cout << std::endl;

    // Minimize console after 3 seconds (runs in background)
    std::thread(MinimizeConsoleAfterDelay, 3).detach();

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    
    while (g_bRunning)
    {
        // Process messages
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) g_bRunning = false;
        }
        if (!g_bRunning) break;

        // Handle hotkeys
        if (KeyPressed(VK_F1, g_F1Pressed)) {
            g_OverlayVisible = !g_OverlayVisible;
            std::cout << "[HOTKEY] Overlay: " << (g_OverlayVisible ? "VISIBLE" : "HIDDEN") << std::endl;
        }
        
        if (KeyPressed(VK_INSERT, g_InsertPressed)) {
            g_MenuVisible = !g_MenuVisible;
            std::cout << "[HOTKEY] Menu: " << (g_MenuVisible ? "OPEN" : "CLOSED") << std::endl;
        }
        
        if (KeyPressed(VK_END, g_EndPressed)) {
            std::cout << "[HOTKEY] Exit requested" << std::endl;
            g_bRunning = false;
            break;
        }

        // Update click-through: menu visible = accept input, otherwise click-through
        SetClickThrough(!g_MenuVisible || !g_OverlayVisible);

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Only render if overlay is visible
        if (g_OverlayVisible)
        {
            // Draw radar (always if enabled)
            if (esp_enabled) {
                RenderRadarOverlay();
            }
            
            // Draw menu if visible
            if (g_MenuVisible) {
                RenderZeroMenu();
            }
        }

        // End frame
        ImGui::EndFrame();
        ImGui::Render();

        // Render
        const float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        
        g_pSwapChain->Present(1, 0);
        
        Sleep(1);
    }

    // Cleanup
    std::cout << std::endl << "[INFO] Shutting down..." << std::endl;
    
    ShutdownZeroDMA();
    ShutdownZeroUI();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(g_hWnd);
    UnregisterClassW(WINDOW_CLASS_NAME, hInstance);
    
    std::cout << "[INFO] Goodbye!" << std::endl;
    FreeConsole();
    
    return 0;
}

// Console entry point
int main() { return wWinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOWDEFAULT); }
