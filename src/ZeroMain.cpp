// ZeroMain.cpp - Zero Elite Main Entry Point
// DirectX 11 Transparent Overlay with ImGui

#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <iostream>

// ImGui includes
#include "../include/imgui.h"
#include "../include/imgui_impl_dx11.h"
#include "../include/imgui_impl_win32.h"

// Zero Elite includes
#include "ZeroUI.h"
#include "DMA_Engine.h"

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

// Overlay configuration
constexpr const wchar_t* WINDOW_CLASS_NAME = L"ZeroEliteOverlayClass";
constexpr const wchar_t* WINDOW_TITLE = L"Zero Elite Overlay";

// Function declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CreateOverlayWindow();
void SetupWindowForOverlay(HWND hWnd);

// Create DirectX 11 device and swap chain
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain description
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

    UINT createDeviceFlags = 0;
    #ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = { 
        D3D_FEATURE_LEVEL_11_0, 
        D3D_FEATURE_LEVEL_10_0 
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    );

    if (FAILED(hr))
    {
        std::cerr << "[Zero Elite] Failed to create D3D11 device. HRESULT: 0x" << std::hex << hr << std::endl;
        return false;
    }

    CreateRenderTarget();
    return true;
}

// Cleanup DirectX resources
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) 
    { 
        g_pSwapChain->Release(); 
        g_pSwapChain = nullptr; 
    }
    if (g_pd3dDeviceContext) 
    { 
        g_pd3dDeviceContext->Release(); 
        g_pd3dDeviceContext = nullptr; 
    }
    if (g_pd3dDevice) 
    { 
        g_pd3dDevice->Release(); 
        g_pd3dDevice = nullptr; 
    }
}

// Create render target view
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

// Cleanup render target
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) 
    { 
        g_mainRenderTargetView->Release(); 
        g_mainRenderTargetView = nullptr; 
    }
}

// Setup window for transparent overlay
void SetupWindowForOverlay(HWND hWnd)
{
    // Make window transparent
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hWnd, &margins);

    // Set window to always on top
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // Set layered window attributes for click-through when menu is hidden
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);
}

// Create the overlay window
bool CreateOverlayWindow()
{
    // Register window class
    g_wc = {};
    g_wc.cbSize = sizeof(WNDCLASSEXW);
    g_wc.style = CS_HREDRAW | CS_VREDRAW;
    g_wc.lpfnWndProc = WndProc;
    g_wc.cbClsExtra = 0;
    g_wc.cbWndExtra = 0;
    g_wc.hInstance = GetModuleHandle(nullptr);
    g_wc.hIcon = nullptr;
    g_wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    g_wc.hbrBackground = nullptr;
    g_wc.lpszMenuName = nullptr;
    g_wc.lpszClassName = WINDOW_CLASS_NAME;
    g_wc.hIconSm = nullptr;

    if (!RegisterClassExW(&g_wc))
    {
        std::cerr << "[Zero Elite] Failed to register window class" << std::endl;
        return false;
    }

    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create window with WS_EX_LAYERED and WS_EX_TRANSPARENT for transparent overlay
    g_hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_POPUP,
        0, 0,
        screenWidth, screenHeight,
        nullptr,
        nullptr,
        g_wc.hInstance,
        nullptr
    );

    if (!g_hWnd)
    {
        std::cerr << "[Zero Elite] Failed to create overlay window" << std::endl;
        UnregisterClassW(WINDOW_CLASS_NAME, g_wc.hInstance);
        return false;
    }

    // Setup overlay transparency
    SetupWindowForOverlay(g_hWnd);

    // Show the window
    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    std::cout << "[Zero Elite] Overlay window created successfully" << std::endl;
    std::cout << "[Zero Elite] Screen resolution: " << screenWidth << "x" << screenHeight << std::endl;

    return true;
}

// Update window clickthrough state based on menu visibility
void UpdateWindowClickthrough()
{
    if (ZeroElite::ZeroUI::IsMenuVisible())
    {
        // Menu visible - allow input
        SetWindowLongW(g_hWnd, GWL_EXSTYLE, 
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
    }
    else
    {
        // Menu hidden - click through
        SetWindowLongW(g_hWnd, GWL_EXSTYLE, 
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
    }
}

// Window procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Forward to ImGui
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
            {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, 
                    static_cast<UINT>(LOWORD(lParam)), 
                    static_cast<UINT>(HIWORD(lParam)), 
                    DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;

        case WM_KEYDOWN:
            // INSERT key to toggle menu
            if (wParam == VK_INSERT)
            {
                ZeroElite::ZeroUI::ToggleMenu();
                UpdateWindowClickthrough();
            }
            // ESC to close menu
            else if (wParam == VK_ESCAPE)
            {
                if (ZeroElite::ZeroUI::IsMenuVisible())
                {
                    ZeroElite::ZeroUI::SetMenuVisible(false);
                    UpdateWindowClickthrough();
                }
            }
            // F1 to toggle ESP
            else if (wParam == VK_F1)
            {
                // ESP toggle handled in ZeroUI
            }
            // END key to exit
            else if (wParam == VK_END)
            {
                g_bRunning = false;
            }
            return 0;

        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
            {
                return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Main entry point
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Allocate console for debug output
    #ifdef _DEBUG
    AllocConsole();
    FILE* pFile = nullptr;
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    freopen_s(&pFile, "CONOUT$", "w", stderr);
    #endif

    std::cout << "========================================" << std::endl;
    std::cout << "        Zero Elite - Game Overlay       " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Create overlay window
    if (!CreateOverlayWindow())
    {
        MessageBoxW(nullptr, L"Failed to create overlay window", L"Zero Elite Error", MB_ICONERROR);
        return 1;
    }

    // Initialize DirectX 11
    if (!CreateDeviceD3D(g_hWnd))
    {
        CleanupDeviceD3D();
        DestroyWindow(g_hWnd);
        UnregisterClassW(WINDOW_CLASS_NAME, g_wc.hInstance);
        MessageBoxW(nullptr, L"Failed to create DirectX 11 device", L"Zero Elite Error", MB_ICONERROR);
        return 1;
    }

    std::cout << "[Zero Elite] DirectX 11 initialized successfully" << std::endl;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // Disable imgui.ini file

    std::cout << "[Zero Elite] ImGui context created" << std::endl;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Initialize Zero Elite UI
    ZeroElite::ZeroUI::Initialize();

    // Initialize DMA Engine with target PID and handle
    if (ZeroElite::DMAEngine::Initialize(ZeroElite::TARGET_PID, 
                                          reinterpret_cast<HANDLE>(ZeroElite::TARGET_HANDLE)))
    {
        std::cout << "[Zero Elite] DMA Engine initialized" << std::endl;
    }
    else
    {
        std::cout << "[Zero Elite] DMA Engine initialization failed (continuing without DMA)" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "[Zero Elite] Overlay is now running!" << std::endl;
    std::cout << "[Zero Elite] Press INSERT to toggle menu" << std::endl;
    std::cout << "[Zero Elite] Press END to exit" << std::endl;
    std::cout << std::endl;

    // Clear background color (transparent)
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Main loop
    MSG msg = {};
    while (g_bRunning)
    {
        // Process Windows messages
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
            {
                g_bRunning = false;
            }
        }

        if (!g_bRunning)
        {
            break;
        }

        // Handle hotkeys (alternative method for when window doesn't have focus)
        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            ZeroElite::ZeroUI::ToggleMenu();
            UpdateWindowClickthrough();
        }

        if (GetAsyncKeyState(VK_END) & 1)
        {
            g_bRunning = false;
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render Zero Elite UI
        ZeroElite::ZeroUI::Render();

        // End ImGui frame
        ImGui::EndFrame();
        ImGui::Render();

        // Clear and render
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        
        // Render ImGui draw data
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present the frame
        int refreshRate = ZeroElite::ZeroUI::GetRefreshRate();
        UINT syncInterval = (refreshRate >= 60) ? 1 : 0;
        g_pSwapChain->Present(syncInterval, 0);

        // Optional: Limit frame rate when menu is hidden to reduce CPU usage
        if (!ZeroElite::ZeroUI::IsMenuVisible())
        {
            Sleep(1000 / refreshRate);
        }
    }

    std::cout << std::endl;
    std::cout << "[Zero Elite] Shutting down..." << std::endl;

    // Cleanup
    ZeroElite::DMAEngine::Shutdown();
    ZeroElite::ZeroUI::Shutdown();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_hWnd);
    UnregisterClassW(WINDOW_CLASS_NAME, g_wc.hInstance);

    std::cout << "[Zero Elite] Shutdown complete. Goodbye!" << std::endl;

    #ifdef _DEBUG
    FreeConsole();
    #endif

    return 0;
}

// Console subsystem entry point (for debugging)
int main(int argc, char* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    
    return wWinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineW(), SW_SHOWDEFAULT);
}
