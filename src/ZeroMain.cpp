// PROJECT ZERO | BO6 DMA - Clean Overlay v3.2
// Features: Transparent Overlay, No Debug Shapes, Game Sync

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cmath>

#include "DMA_Engine.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

// ============================================================================
// SETTINGS
// ============================================================================
struct AppSettings {
    bool aimbotEnabled = false;
    float aimbotFOV = 120.0f;
    float aimbotSmooth = 5.0f;
    int aimbotBone = 0;
    bool aimbotShowFOV = false;
    
    bool espEnabled = true;
    bool espBox = true;
    bool espHealth = true;
    bool espName = true;
    bool espDistance = true;
    
    bool radarEnabled = true;
    float radarSize = 200.0f;
    float radarZoom = 1.5f;
    
    bool crosshair = false;
    float crosshairSize = 8.0f;
};

static AppSettings g_Settings;

// ============================================================================
// GLOBALS
// ============================================================================
static HWND g_Hwnd = nullptr;
static int g_ScreenW = 0;
static int g_ScreenH = 0;

// D3D11
static ID3D11Device* g_D3DDevice = nullptr;
static ID3D11DeviceContext* g_D3DContext = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D11RenderTargetView* g_RenderTarget = nullptr;

// D2D
static ID2D1Factory* g_D2DFactory = nullptr;
static ID2D1RenderTarget* g_D2DTarget = nullptr;
static ID2D1SolidColorBrush* g_Brush = nullptr;
static IDWriteFactory* g_DWriteFactory = nullptr;
static IDWriteTextFormat* g_Font = nullptr;
static IDWriteTextFormat* g_FontSmall = nullptr;

// State
static std::atomic<bool> g_Running(true);
static std::atomic<bool> g_GameRunning(false);
static bool g_MenuVisible = false;
static int g_CurrentTab = 0;
static POINT g_Mouse = {0, 0};
static bool g_MouseDown = false;
static bool g_MouseClicked = false;

// FPS
static float g_FPS = 0;
static int g_FrameCount = 0;
static DWORD g_LastFPSTime = 0;

// ============================================================================
// COLORS
// ============================================================================
namespace Colors {
    const D2D1_COLOR_F Background = {0.08f, 0.08f, 0.08f, 0.95f};
    const D2D1_COLOR_F Red = {0.9f, 0.15f, 0.15f, 1.0f};
    const D2D1_COLOR_F Green = {0.15f, 0.9f, 0.3f, 1.0f};
    const D2D1_COLOR_F Blue = {0.3f, 0.5f, 1.0f, 1.0f};
    const D2D1_COLOR_F White = {1.0f, 1.0f, 1.0f, 1.0f};
    const D2D1_COLOR_F Gray = {0.5f, 0.5f, 0.5f, 1.0f};
    const D2D1_COLOR_F Yellow = {1.0f, 0.9f, 0.2f, 1.0f};
}

// ============================================================================
// INPUT THREAD - Non-blocking
// ============================================================================
void InputThread()
{
    bool insertWasDown = false;
    bool endWasDown = false;
    
    while (g_Running)
    {
        // Toggle menu with INSERT
        bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (insertDown && !insertWasDown)
        {
            g_MenuVisible = !g_MenuVisible;
            
            // Update window transparency
            if (g_Hwnd)
            {
                LONG_PTR style = GetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE);
                if (g_MenuVisible)
                    style &= ~WS_EX_TRANSPARENT;
                else
                    style |= WS_EX_TRANSPARENT;
                SetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE, style);
            }
        }
        insertWasDown = insertDown;
        
        // Exit with END
        bool endDown = (GetAsyncKeyState(VK_END) & 0x8000) != 0;
        if (endDown && !endWasDown)
        {
            g_Running = false;
        }
        endWasDown = endDown;
        
        Sleep(16);
    }
}

// ============================================================================
// UPDATE THREAD
// ============================================================================
void UpdateThread()
{
    PlayerManager::Initialize();
    
    while (g_Running)
    {
        // Check if game is running
        g_GameRunning = DMAEngine::IsOnline() || DMAEngine::s_SimulationMode;
        
        if (g_GameRunning)
        {
            PlayerManager::Update();
        }
        
        Sleep(16);  // ~60 Hz
    }
}

// ============================================================================
// WINDOW PROC
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEMOVE:
        g_Mouse.x = LOWORD(lParam);
        g_Mouse.y = HIWORD(lParam);
        return 0;
        
    case WM_LBUTTONDOWN:
        g_MouseDown = true;
        g_MouseClicked = true;
        return 0;
        
    case WM_LBUTTONUP:
        g_MouseDown = false;
        return 0;
        
    case WM_DESTROY:
        g_Running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================================
// CREATE OVERLAY - Proper Transparent Window
// ============================================================================
bool CreateOverlayWindow()
{
    // Get exact screen resolution
    g_ScreenW = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenH = GetSystemMetrics(SM_CYSCREEN);
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ZeroOverlayClass";
    
    if (!RegisterClassExW(&wc))
        return false;
    
    // Create with proper styles for transparent overlay
    g_Hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        L"",
        WS_POPUP,
        0, 0, g_ScreenW, g_ScreenH,
        nullptr, nullptr, wc.hInstance, nullptr);
    
    if (!g_Hwnd)
        return false;
    
    // Make window click-through with color key (black = transparent)
    SetLayeredWindowAttributes(g_Hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // Extend frame for DWM composition
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(g_Hwnd, &margins);
    
    // Show window
    ShowWindow(g_Hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_Hwnd);
    
    return true;
}

// ============================================================================
// INIT GRAPHICS
// ============================================================================
bool InitGraphics()
{
    // SwapChain description
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = g_ScreenW;
    sd.BufferDesc.Height = g_ScreenH;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_Hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        flags, nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_SwapChain, &g_D3DDevice, nullptr, &g_D3DContext);
    
    if (FAILED(hr))
        return false;
    
    // Create render target
    ID3D11Texture2D* backBuffer = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_D3DDevice->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
    backBuffer->Release();
    
    // Create D2D factory
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_D2DFactory);
    if (FAILED(hr))
        return false;
    
    // Create D2D render target from swap chain
    IDXGISurface* surface = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);
    
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    hr = g_D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &g_D2DTarget);
    surface->Release();
    
    if (FAILED(hr))
        return false;
    
    // Create brush
    g_D2DTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &g_Brush);
    
    // Create DirectWrite factory and fonts
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_DWriteFactory);
    
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        14.0f, L"", &g_Font);
    
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        11.0f, L"", &g_FontSmall);
    
    return true;
}

// ============================================================================
// CLEANUP
// ============================================================================
void Cleanup()
{
    if (g_FontSmall) g_FontSmall->Release();
    if (g_Font) g_Font->Release();
    if (g_DWriteFactory) g_DWriteFactory->Release();
    if (g_Brush) g_Brush->Release();
    if (g_D2DTarget) g_D2DTarget->Release();
    if (g_D2DFactory) g_D2DFactory->Release();
    if (g_RenderTarget) g_RenderTarget->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_D3DContext) g_D3DContext->Release();
    if (g_D3DDevice) g_D3DDevice->Release();
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================
inline void DrawText2D(const wchar_t* text, float x, float y, D2D1_COLOR_F color, IDWriteTextFormat* font = nullptr)
{
    if (!font) font = g_Font;
    g_Brush->SetColor(color);
    D2D1_RECT_F rect = {x, y, x + 500, y + 50};
    g_D2DTarget->DrawText(text, (UINT32)wcslen(text), font, rect, g_Brush);
}

inline void FillRect2D(float x, float y, float w, float h, D2D1_COLOR_F color)
{
    g_Brush->SetColor(color);
    g_D2DTarget->FillRectangle({x, y, x + w, y + h}, g_Brush);
}

inline void DrawRect2D(float x, float y, float w, float h, D2D1_COLOR_F color, float thickness = 1.0f)
{
    g_Brush->SetColor(color);
    g_D2DTarget->DrawRectangle({x, y, x + w, y + h}, g_Brush, thickness);
}

inline void DrawLine2D(float x1, float y1, float x2, float y2, D2D1_COLOR_F color, float thickness = 1.0f)
{
    g_Brush->SetColor(color);
    g_D2DTarget->DrawLine({x1, y1}, {x2, y2}, g_Brush, thickness);
}

inline void DrawCircle2D(float cx, float cy, float r, D2D1_COLOR_F color, bool fill = false, float thickness = 1.0f)
{
    g_Brush->SetColor(color);
    D2D1_ELLIPSE ellipse = {{cx, cy}, r, r};
    if (fill)
        g_D2DTarget->FillEllipse(ellipse, g_Brush);
    else
        g_D2DTarget->DrawEllipse(ellipse, g_Brush, thickness);
}

inline void DrawRoundedRect2D(float x, float y, float w, float h, float r, D2D1_COLOR_F color, bool fill = true)
{
    g_Brush->SetColor(color);
    D2D1_ROUNDED_RECT rr = {{x, y, x + w, y + h}, r, r};
    if (fill)
        g_D2DTarget->FillRoundedRectangle(rr, g_Brush);
    else
        g_D2DTarget->DrawRoundedRectangle(rr, g_Brush, 1.0f);
}

inline bool InRect(float x, float y, float w, float h)
{
    return g_Mouse.x >= x && g_Mouse.x <= x + w && g_Mouse.y >= y && g_Mouse.y <= y + h;
}

// ============================================================================
// UI WIDGETS
// ============================================================================
bool Toggle(const wchar_t* label, bool* value, float x, float y)
{
    float toggleW = 36, toggleH = 18;
    bool hovered = InRect(x, y, 200, toggleH + 4);
    
    if (hovered && g_MouseClicked)
        *value = !*value;
    
    DrawText2D(label, x, y, hovered ? Colors::White : Colors::Gray);
    
    float tx = x + 150;
    D2D1_COLOR_F trackColor = *value ? Colors::Red : D2D1_COLOR_F{0.3f, 0.3f, 0.3f, 1.0f};
    DrawRoundedRect2D(tx, y, toggleW, toggleH, toggleH / 2, trackColor, true);
    
    float knobX = *value ? tx + toggleW - toggleH + 2 : tx + 2;
    DrawCircle2D(knobX + toggleH / 2 - 2, y + toggleH / 2, toggleH / 2 - 3, Colors::White, true);
    
    return hovered && g_MouseClicked;
}

void Slider(const wchar_t* label, float* value, float minV, float maxV, float x, float y)
{
    float sliderW = 120, sliderH = 6;
    float labelW = 80;
    
    DrawText2D(label, x, y, Colors::Gray);
    
    float sx = x + labelW;
    float sy = y + 6;
    
    bool hovered = InRect(sx - 5, y, sliderW + 10, 20);
    
    if (hovered && g_MouseDown)
    {
        float pct = ((float)g_Mouse.x - sx) / sliderW;
        pct = pct < 0 ? 0 : (pct > 1 ? 1 : pct);
        *value = minV + pct * (maxV - minV);
    }
    
    // Track
    FillRect2D(sx, sy, sliderW, sliderH, D2D1_COLOR_F{0.3f, 0.3f, 0.3f, 1.0f});
    
    // Fill
    float pct = (*value - minV) / (maxV - minV);
    FillRect2D(sx, sy, sliderW * pct, sliderH, Colors::Red);
    
    // Knob
    float kx = sx + sliderW * pct;
    DrawCircle2D(kx, sy + sliderH / 2, 6, Colors::White, true);
    
    // Value text
    wchar_t buf[16];
    swprintf_s(buf, L"%.0f", *value);
    DrawText2D(buf, sx + sliderW + 10, y, Colors::Red, g_FontSmall);
}

// ============================================================================
// RENDER RADAR - Only when game is running
// ============================================================================
void RenderRadar()
{
    if (!g_Settings.radarEnabled || !g_GameRunning) return;
    
    std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
    
    float size = g_Settings.radarSize;
    float rx = (float)g_ScreenW - size - 20;
    float ry = 20;
    float cx = rx + size / 2;
    float cy = ry + size / 2;
    
    // Background
    DrawRoundedRect2D(rx, ry, size, size, 8, Colors::Background, true);
    DrawRoundedRect2D(rx, ry, size, size, 8, Colors::Red, false);
    
    // Grid
    D2D1_COLOR_F gridColor = {0.2f, 0.2f, 0.2f, 0.8f};
    DrawLine2D(cx, ry + 5, cx, ry + size - 5, gridColor);
    DrawLine2D(rx + 5, cy, rx + size - 5, cy, gridColor);
    
    // Range circles
    for (int i = 1; i <= 3; i++)
    {
        float r = (size * 0.4f) * ((float)i / 3.0f);
        DrawCircle2D(cx, cy, r, gridColor, false);
    }
    
    // Players
    auto& players = PlayerManager::GetPlayers();
    auto& local = PlayerManager::GetLocalPlayer();
    
    for (const auto& p : players)
    {
        if (!p.valid || !p.isAlive) continue;
        
        Vec2 radarPos = MapTextureManager::GameToRadarCoords(
            p.origin, local.origin, local.yaw,
            cx, cy, size, g_Settings.radarZoom);
        
        // Clamp to radar bounds
        float maxDist = size * 0.4f;
        Vec2 delta = {radarPos.x - cx, radarPos.y - cy};
        float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);
        if (dist > maxDist)
        {
            radarPos.x = cx + delta.x / dist * maxDist;
            radarPos.y = cy + delta.y / dist * maxDist;
        }
        
        D2D1_COLOR_F color = p.isEnemy ? Colors::Red : Colors::Blue;
        DrawCircle2D(radarPos.x, radarPos.y, 4, color, true);
        
        // Direction indicator
        float yawRad = p.yaw * 3.14159f / 180.0f;
        float dx = sinf(yawRad) * 8;
        float dy = -cosf(yawRad) * 8;
        DrawLine2D(radarPos.x, radarPos.y, radarPos.x + dx, radarPos.y + dy, color, 2);
    }
    
    // Local player (center)
    DrawCircle2D(cx, cy, 5, Colors::Green, true);
    float yawRad = local.yaw * 3.14159f / 180.0f;
    DrawLine2D(cx, cy, cx + sinf(yawRad) * 12, cy - cosf(yawRad) * 12, Colors::Green, 2);
    
    // Label
    DrawText2D(L"RADAR", rx + 5, ry + size + 2, Colors::White, g_FontSmall);
}

// ============================================================================
// RENDER ESP - Only when game is running
// ============================================================================
void RenderESP()
{
    if (!g_Settings.espEnabled || !g_GameRunning) return;
    
    std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
    
    auto& players = PlayerManager::GetPlayers();
    Vec2 screenCenter = {(float)g_ScreenW / 2, (float)g_ScreenH / 2};
    
    for (auto& p : players)
    {
        if (!p.valid || !p.isAlive) continue;
        
        // Simple screen position simulation
        float simScale = 400.0f / (p.distance + 10.0f);
        p.screenPos.x = screenCenter.x + p.origin.x * simScale * 0.5f;
        p.screenPos.y = screenCenter.y - p.origin.y * simScale * 0.5f;
        p.screenHeight = 80.0f * simScale;
        p.screenWidth = p.screenHeight * 0.4f;
        
        // Bounds check
        if (p.screenPos.x < 0 || p.screenPos.x > g_ScreenW ||
            p.screenPos.y < 0 || p.screenPos.y > g_ScreenH)
            continue;
        
        D2D1_COLOR_F color = p.isEnemy ? Colors::Red : Colors::Blue;
        
        float bx = p.screenPos.x - p.screenWidth / 2;
        float by = p.screenPos.y - p.screenHeight;
        float bw = p.screenWidth;
        float bh = p.screenHeight;
        
        // Box
        if (g_Settings.espBox)
        {
            DrawRect2D(bx, by, bw, bh, color, 1.5f);
        }
        
        // Health bar
        if (g_Settings.espHealth)
        {
            float hbW = 3;
            float hbH = bh;
            float hbX = bx - hbW - 2;
            float hpPct = (float)p.health / (float)p.maxHealth;
            
            FillRect2D(hbX, by, hbW, hbH, D2D1_COLOR_F{0.2f, 0.2f, 0.2f, 1.0f});
            D2D1_COLOR_F hpColor = {1.0f - hpPct, hpPct, 0.1f, 1.0f};
            FillRect2D(hbX, by + hbH * (1 - hpPct), hbW, hbH * hpPct, hpColor);
        }
        
        // Name
        if (g_Settings.espName)
        {
            wchar_t nameBuf[64];
            mbstowcs_s(nullptr, nameBuf, p.name, 63);
            DrawText2D(nameBuf, bx, by - 14, color, g_FontSmall);
        }
        
        // Distance
        if (g_Settings.espDistance)
        {
            wchar_t distBuf[16];
            swprintf_s(distBuf, L"%.0fm", p.distance);
            DrawText2D(distBuf, bx, by + bh + 2, Colors::White, g_FontSmall);
        }
    }
}

// ============================================================================
// RENDER CROSSHAIR
// ============================================================================
void RenderCrosshair()
{
    if (!g_Settings.crosshair || !g_GameRunning) return;
    
    float cx = (float)g_ScreenW / 2;
    float cy = (float)g_ScreenH / 2;
    float size = g_Settings.crosshairSize;
    
    DrawLine2D(cx - size, cy, cx + size, cy, Colors::White, 2);
    DrawLine2D(cx, cy - size, cx, cy + size, Colors::White, 2);
}

// ============================================================================
// RENDER MENU
// ============================================================================
void RenderMenu()
{
    if (!g_MenuVisible) return;
    
    float mx = 50, my = 50;
    float mw = 400, mh = 400;
    
    // Background
    DrawRoundedRect2D(mx, my, mw, mh, 10, Colors::Background, true);
    DrawRoundedRect2D(mx, my, mw, mh, 10, D2D1_COLOR_F{0.3f, 0.1f, 0.1f, 1.0f}, false);
    
    // Title bar
    DrawRoundedRect2D(mx + 2, my + 2, mw - 4, 36, 8, D2D1_COLOR_F{0.4f, 0.1f, 0.1f, 1.0f}, true);
    DrawText2D(L"PROJECT ZERO | v3.2", mx + 15, my + 8, Colors::White);
    
    // Status
    const char* status = DMAEngine::GetStatus();
    wchar_t statusW[32];
    mbstowcs_s(nullptr, statusW, status, 31);
    D2D1_COLOR_F statusColor = Colors::Yellow;
    if (strcmp(status, "ONLINE") == 0) statusColor = Colors::Green;
    DrawText2D(statusW, mx + mw - 100, my + 10, statusColor, g_FontSmall);
    
    // Tabs
    float ty = my + 45;
    float tw = 90, th = 26;
    const wchar_t* tabs[] = {L"VISUALS", L"RADAR", L"MISC"};
    
    for (int i = 0; i < 3; i++)
    {
        float tx = mx + 10 + i * (tw + 5);
        bool selected = (g_CurrentTab == i);
        bool hovered = InRect(tx, ty, tw, th);
        
        if (hovered && g_MouseClicked)
            g_CurrentTab = i;
        
        D2D1_COLOR_F tabColor = selected ? Colors::Red : (hovered ? D2D1_COLOR_F{0.4f, 0.15f, 0.15f, 1.0f} : D2D1_COLOR_F{0.2f, 0.2f, 0.2f, 1.0f});
        DrawRoundedRect2D(tx, ty, tw, th, 5, tabColor, true);
        DrawText2D(tabs[i], tx + 15, ty + 5, Colors::White, g_FontSmall);
    }
    
    // Content
    float cx = mx + 20;
    float cy = ty + 40;
    float lineH = 28;
    int line = 0;
    
    if (g_CurrentTab == 0)  // VISUALS
    {
        Toggle(L"Enable ESP", &g_Settings.espEnabled, cx, cy + line * lineH); line++;
        
        if (g_Settings.espEnabled)
        {
            line++;
            Toggle(L"Box", &g_Settings.espBox, cx + 10, cy + line * lineH); line++;
            Toggle(L"Health Bar", &g_Settings.espHealth, cx + 10, cy + line * lineH); line++;
            Toggle(L"Name", &g_Settings.espName, cx + 10, cy + line * lineH); line++;
            Toggle(L"Distance", &g_Settings.espDistance, cx + 10, cy + line * lineH); line++;
        }
    }
    else if (g_CurrentTab == 1)  // RADAR
    {
        Toggle(L"Enable Radar", &g_Settings.radarEnabled, cx, cy + line * lineH); line++;
        line++;
        Slider(L"Size", &g_Settings.radarSize, 150, 300, cx, cy + line * lineH); line++;
        Slider(L"Zoom", &g_Settings.radarZoom, 0.5f, 3.0f, cx, cy + line * lineH); line++;
        
        line++;
        wchar_t playerInfo[64];
        swprintf_s(playerInfo, L"Players: %d | Enemies: %d", 
                   PlayerManager::GetAliveCount(), PlayerManager::GetEnemyCount());
        DrawText2D(playerInfo, cx, cy + line * lineH, Colors::White, g_FontSmall);
    }
    else if (g_CurrentTab == 2)  // MISC
    {
        Toggle(L"Crosshair", &g_Settings.crosshair, cx, cy + line * lineH); line++;
        
        if (g_Settings.crosshair)
        {
            Slider(L"Size", &g_Settings.crosshairSize, 4, 20, cx + 10, cy + line * lineH); line++;
        }
        
        line++;
        line++;
        DrawText2D(L"Controls:", cx, cy + line * lineH, Colors::White); line++;
        DrawText2D(L"INSERT - Toggle Menu", cx + 10, cy + line * lineH, Colors::Gray, g_FontSmall); line++;
        DrawText2D(L"END - Exit Program", cx + 10, cy + line * lineH, Colors::Gray, g_FontSmall); line++;
        
        line++;
        wchar_t fpsText[32];
        swprintf_s(fpsText, L"FPS: %.0f", g_FPS);
        DrawText2D(fpsText, cx, cy + line * lineH, Colors::Green, g_FontSmall);
    }
    
    g_MouseClicked = false;
}

// ============================================================================
// MAIN
// ============================================================================
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // Initialize DMA first
    if (!DMAEngine::Initialize())
    {
        return 1;
    }
    
    // Hide console after init
    HWND consoleWnd = GetConsoleWindow();
    if (consoleWnd)
    {
        ShowWindow(consoleWnd, SW_HIDE);
    }
    
    // Create overlay
    if (!CreateOverlayWindow())
    {
        MessageBoxW(nullptr, L"Failed to create overlay", L"Error", MB_OK);
        return 1;
    }
    
    // Init graphics
    if (!InitGraphics())
    {
        MessageBoxW(nullptr, L"Failed to init DirectX", L"Error", MB_OK);
        return 1;
    }
    
    // Start threads
    std::thread inputThread(InputThread);
    std::thread updateThread(UpdateThread);
    
    g_LastFPSTime = GetTickCount();
    
    // Main loop
    while (g_Running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        // FPS
        g_FrameCount++;
        DWORD now = GetTickCount();
        if (now - g_LastFPSTime >= 1000)
        {
            g_FPS = (float)g_FrameCount * 1000.0f / (float)(now - g_LastFPSTime);
            g_FrameCount = 0;
            g_LastFPSTime = now;
        }
        
        // Clear with transparent black
        float clearColor[4] = {0, 0, 0, 0};
        g_D3DContext->ClearRenderTargetView(g_RenderTarget, clearColor);
        
        // Begin D2D drawing
        g_D2DTarget->BeginDraw();
        g_D2DTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
        
        // Only draw overlays if game is running or menu is visible
        if (g_GameRunning || g_MenuVisible)
        {
            RenderESP();
            RenderCrosshair();
            RenderRadar();
        }
        
        // Always allow menu
        RenderMenu();
        
        g_D2DTarget->EndDraw();
        
        // Present (V-Sync off)
        g_SwapChain->Present(0, 0);
        
        // Small sleep to prevent 100% CPU
        Sleep(1);
    }
    
    // Cleanup
    g_Running = false;
    inputThread.join();
    updateThread.join();
    
    DMAEngine::Shutdown();
    Cleanup();
    DestroyWindow(g_Hwnd);
    
    return 0;
}

int main() { return wWinMain(nullptr, nullptr, nullptr, 0); }
