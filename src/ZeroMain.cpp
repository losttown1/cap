// ZeroMain.cpp - STRICT AUTOMATION v4.4
// Features: Scatter Registry, Map Textures, Config-driven Init

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cmath>
#include <algorithm>

#include "DMA_Engine.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "windowscodecs.lib")

// ============================================================================
// SETTINGS
// ============================================================================
struct AppSettings {
    bool aimbotEnabled = false;
    float aimbotFOV = 120.0f;
    float aimbotSmooth = 5.0f;
    int aimbotBone = 0;
    bool aimbotVisCheck = true;
    bool aimbotShowFOV = true;
    float aimbotMaxDist = 200.0f;

    bool espEnabled = true;
    bool espBox = true;
    bool espBoxCorner = false;
    bool espSkeleton = false;
    bool espHealth = true;
    bool espName = true;
    bool espDistance = true;
    bool espSnapline = false;
    int espSnaplinePos = 1;
    bool espTeammates = false;

    bool radarEnabled = true;
    float radarSize = 220.0f;
    float radarZoom = 1.5f;
    bool radarShowEnemy = true;
    bool radarShowTeam = true;
    bool radarUseMapTexture = false;

    bool noRecoil = false;
    bool rapidFire = false;
    bool crosshair = true;
    int crosshairType = 0;
    float crosshairSize = 8.0f;
    bool streamProof = false;
    bool showPerformance = true;
};

static AppSettings g_Settings;

// ============================================================================
// GLOBALS
// ============================================================================
static HWND g_Hwnd = nullptr;
static int g_ScreenW = 0;
static int g_ScreenH = 0;
static bool g_Running = true;
static bool g_MenuVisible = true;
static int g_CurrentTab = 0;
static Vec2 g_Mouse;
static bool g_MouseClicked = false;

// D3D11
static ID3D11Device* g_D3DDevice = nullptr;
static ID3D11DeviceContext* g_D3DContext = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D11RenderTargetView* g_RenderTarget = nullptr;

// D2D
static ID2D1Factory1* g_D2DFactory = nullptr;
static ID2D1DeviceContext* g_D2DTarget = nullptr;
static ID2D1SolidColorBrush* g_Brush = nullptr;
static IDWriteFactory* g_DWriteFactory = nullptr;
static IDWriteTextFormat* g_Font = nullptr;
static IDWriteTextFormat* g_FontBold = nullptr;
static IDWriteTextFormat* g_FontSmall = nullptr;
static ID2D1Bitmap* g_MapTexture = nullptr;

// Stats
static float g_FPS = 0;
static int g_FrameCount = 0;
static DWORD g_LastFPSTime = 0;

namespace Colors {
    static const D2D1_COLOR_F White = {1, 1, 1, 1};
    static const D2D1_COLOR_F Black = {0, 0, 0, 1};
    static const D2D1_COLOR_F Red = {1, 0, 0, 1};
    static const D2D1_COLOR_F Green = {0, 1, 0, 1};
    static const D2D1_COLOR_F Blue = {0, 0, 1, 1};
    static const D2D1_COLOR_F Yellow = {1, 1, 0, 1};
    static const D2D1_COLOR_F Cyan = {0, 1, 1, 1};
    static const D2D1_COLOR_F Gray = {0.5f, 0.5f, 0.5f, 1};
}

// ============================================================================
// MAP TEXTURE LOADING
// ============================================================================
bool LoadMapTexture(const wchar_t* path)
{
    IWICImagingFactory* wicFactory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) return false;

    hr = wicFactory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) { wicFactory->Release(); return false; }

    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) { decoder->Release(); wicFactory->Release(); return false; }

    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) { frame->Release(); decoder->Release(); wicFactory->Release(); return false; }

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); wicFactory->Release(); return false; }

    if (g_MapTexture) { g_MapTexture->Release(); g_MapTexture = nullptr; }

    hr = g_D2DTarget->CreateBitmapFromWicBitmap(converter, nullptr, &g_MapTexture);

    if (SUCCEEDED(hr) && g_MapTexture)
    {
        D2D1_SIZE_F size = g_MapTexture->GetSize();
        MapTextureManager::GetCurrentMap().imageWidth = (int)size.width;
        MapTextureManager::GetCurrentMap().imageHeight = (int)size.height;
        MapTextureManager::GetCurrentMap().hasTexture = true;
    }

    converter->Release();
    frame->Release();
    decoder->Release();
    wicFactory->Release();

    return SUCCEEDED(hr);
}

// ============================================================================
// WINDOW SETUP
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEMOVE:
        g_Mouse.x = (float)LOWORD(lParam);
        g_Mouse.y = (float)HIWORD(lParam);
        return 0;
    case WM_LBUTTONDOWN:
        g_MouseClicked = true;
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_INSERT) g_MenuVisible = !g_MenuVisible;
        return 0;
    case WM_DESTROY:
        g_Running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool CreateOverlayWindow()
{
    g_ScreenW = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenH = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ZeroOverlay";
    RegisterClassExW(&wc);

    g_Hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE, wc.lpszClassName, L"ZeroElite_Stage", WS_POPUP, 0, 0, g_ScreenW, g_ScreenH, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_Hwnd) return false;

    SetLayeredWindowAttributes(g_Hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(g_Hwnd, &margins);

    ShowWindow(g_Hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_Hwnd);
    return true;
}

bool InitGraphics()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = g_ScreenW;
    sd.BufferDesc.Height = g_ScreenH;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate = {0, 1};
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_Hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_D3DDevice, nullptr, &g_D3DContext)))
        return false;

    ID3D11Texture2D* backBuffer;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_D3DDevice->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
    backBuffer->Release();

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), (void**)&g_D2DFactory);
    
    IDXGIDevice* dxgiDevice;
    g_D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    
    ID2D1Device* d2dDevice;
    g_D2DFactory->CreateDevice(dxgiDevice, &d2dDevice);
    d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &g_D2DTarget);
    
    IDXGISurface* surface;
    g_SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);
    
    ID2D1Bitmap1* d2dBitmap;
    g_D2DTarget->CreateBitmapFromDxgiSurface(surface, nullptr, &d2dBitmap);
    g_D2DTarget->SetTarget(d2dBitmap);
    
    surface->Release();
    d2dBitmap->Release();
    d2dDevice->Release();
    dxgiDevice->Release();

    g_D2DTarget->CreateSolidColorBrush(Colors::White, &g_Brush);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_DWriteFactory);
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"", &g_Font);
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"", &g_FontBold);
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 11.0f, L"", &g_FontSmall);

    if (g_Config.mapImagePath[0])
    {
        wchar_t mapPathW[256];
        size_t converted = 0;
        mbstowcs_s(&converted, mapPathW, 256, g_Config.mapImagePath, 255);
        LoadMapTexture(mapPathW);
    }
    return true;
}

void Cleanup()
{
    if (g_MapTexture) g_MapTexture->Release();
    if (g_FontSmall) g_FontSmall->Release();
    if (g_FontBold) g_FontBold->Release();
    if (g_Font) g_Font->Release();
    if (g_DWriteFactory) g_DWriteFactory->Release();
    if (g_Brush) g_Brush->Release();
    if (g_D2DTarget) g_D2DTarget->Release();
    if (g_D2DFactory) g_D2DFactory->Release();
    if (g_RenderTarget) g_RenderTarget->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_D3DContext) g_D3DContext->Release();
    if (g_D3DDevice) g_D3DDevice->Release();
    CoUninitialize();
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================
void DrawTextW2(const wchar_t* text, float x, float y, D2D1_COLOR_F col, IDWriteTextFormat* font = nullptr)
{
    if (!font) font = g_Font;
    g_Brush->SetColor(col);
    g_D2DTarget->DrawTextW(text, (UINT32)wcslen(text), font, D2D1::RectF(x, y, x + 1000, y + 100), g_Brush);
}

void DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F col, float thickness = 1.0f)
{
    g_Brush->SetColor(col);
    g_D2DTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), g_Brush, thickness);
}

void DrawBox(float x, float y, float w, float h, D2D1_COLOR_F col, float thickness = 1.0f)
{
    g_Brush->SetColor(col);
    g_D2DTarget->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), g_Brush, thickness);
}

// ============================================================================
// RENDER FUNCTIONS
// ============================================================================
void RenderESP()
{
    if (!g_Settings.espEnabled) return;
    std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
    auto& players = PlayerManager::GetPlayers();
    for (auto& p : players)
    {
        if (!p.valid || !p.isAlive) continue;
        if (WorldToScreen(p.origin, p.screenPos, g_ScreenW, g_ScreenH))
        {
            DrawBox(p.screenPos.x - 10, p.screenPos.y - 20, 20, 40, Colors::Red);
        }
    }
}

void RenderAimbotFOV() {}
void RenderCrosshair() {}
void RenderRadar() {}

void RenderPerformance()
{
    if (!g_Settings.showPerformance) return;
    float px = 10;
    float py = (float)g_ScreenH - 110;
    wchar_t buf[128];
    swprintf_s(buf, L"FPS: %.0f", g_FPS);
    DrawTextW2(buf, px, py, Colors::Green, g_FontSmall);
    py += 14;
    swprintf_s(buf, L"Scatter: %d reads, %d bytes/frame", g_ScatterRegistry.GetReadCount(), g_ScatterRegistry.GetTotalBytes());
    DrawTextW2(buf, px, py, Colors::Cyan, g_FontSmall);
    py += 14;
    wchar_t deviceInfoW[128];
    size_t converted = 0;
    mbstowcs_s(&converted, deviceInfoW, 128, DMAEngine::GetDeviceInfo(), 127);
    DrawTextW2(deviceInfoW, px, py, Colors::Gray, g_FontSmall);
    py += 14;
    wchar_t driverW[64];
    swprintf_s(driverW, L"Driver: %S", DMAEngine::GetDriverMode());
    DrawTextW2(driverW, px, py, g_Config.useFTD3XX ? Colors::Green : Colors::Yellow, g_FontSmall);
    py += 14;
    wchar_t ctrlW[64];
    swprintf_s(ctrlW, L"Controller: %S", HardwareController::GetDeviceName());
    DrawTextW2(ctrlW, px, py, HardwareController::IsConnected() ? Colors::Green : Colors::Gray, g_FontSmall);
    py += 14;
    swprintf_s(buf, L"Patterns: %d | Offsets: %s", PatternScanner::GetFoundCount(), OffsetUpdater::IsUpdated() ? L"Remote" : L"Local");
    DrawTextW2(buf, px, py, Colors::Yellow, g_FontSmall);
}

void RenderMenu()
{
    if (!g_MenuVisible) return;
    // Simple menu placeholder
    DrawBox(100, 100, 400, 500, Colors::Black);
    DrawTextW2(L"PROJECT ZERO MENU", 110, 110, Colors::White, g_FontBold);
}

// ============================================================================
// THREADS
// ============================================================================
void InputThread()
{
    while (g_Running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void UpdateThread()
{
    while (g_Running)
    {
        DMAEngine::Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ============================================================================
// MAIN
// ============================================================================
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    if (!DMAEngine::Initialize()) return 1;
    PatternScanner::UpdateAllOffsets();
    if (!CreateOverlayWindow()) return 1;
    if (!InitGraphics()) return 1;

    std::thread inputThread(InputThread);
    std::thread updateThread(UpdateThread);
    g_LastFPSTime = GetTickCount();

    while (g_Running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        g_FrameCount++;
        DWORD now = GetTickCount();
        if (now - g_LastFPSTime >= 1000)
        {
            g_FPS = (float)g_FrameCount * 1000.0f / (float)(now - g_LastFPSTime);
            g_FrameCount = 0;
            g_LastFPSTime = now;
        }

        float clearColor[4] = {0, 0, 0, 0};
        g_D3DContext->ClearRenderTargetView(g_RenderTarget, clearColor);
        g_D2DTarget->BeginDraw();
        g_D2DTarget->Clear(D2D1::ColorF(0, 0, 0, 0));

        if (!DMAEngine::IsConnected())
        {
            DrawTextW2(L"STATUS: WAITING FOR DMA HARDWARE...", 50, 50, Colors::Yellow, g_FontBold);
            DrawTextW2(L"Please ensure your DMA card is plugged in and drivers are loaded.", 50, 80, Colors::Gray, g_Font);
        }
        else if (DMAEngine::s_BaseAddress == 0)
        {
            DrawTextW2(L"STATUS: DMA CONNECTED | WAITING FOR GAME (cod.exe)...", 50, 50, Colors::Cyan, g_FontBold);
            DrawTextW2(L"Please start the game to begin synchronization.", 50, 80, Colors::Gray, g_Font);
        }
        else
        {
            RenderESP();
            RenderPerformance();
            RenderMenu();
        }

        g_D2DTarget->EndDraw();
        g_SwapChain->Present(0, 0);
    }

    g_Running = false;
    inputThread.join();
    updateThread.join();
    DMAEngine::Shutdown();
    Cleanup();
    DestroyWindow(g_Hwnd);
    return 0;
}

int main() { return wWinMain(nullptr, nullptr, nullptr, 0); }
