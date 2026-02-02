// ZeroMain.cpp - Blurry-Style Professional v5.0
// The Master Overlay - Real Hardware - Real DMA Data

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <atomic>
#include <thread>
#include <iostream>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

#include "DMA_Engine.hpp"

// External Functions
extern void RunStartupDiagnostics();
extern const char* GetKMBoxStatus();
extern const char* GetKMBoxFirmware();
extern bool IsKMBoxConnected();
extern bool IsHardwareScanComplete();
extern bool IsDMAOnline();
extern const char* GetDMADiagnostic();
extern bool IsDMAMemoryReadable();
extern void SetBlackOverlayEnabled(bool enabled);
extern bool IsBlackOverlayEnabled();
extern std::vector<PlayerData> GetRealEntitiesFromDMA();

// ============================================================================
// HARDWARE STATUS
// ============================================================================
static std::atomic<bool> g_DMA_Online(false);
static std::atomic<bool> g_KMBox_Locked(false);
static std::atomic<bool> g_HWScanDone(false);

// ============================================================================
// THE MASTER OVERLAY (Black Stage)
// ============================================================================
static HWND g_MasterOverlay = nullptr;
static int g_ScreenW = 0;
static int g_ScreenH = 0;
static bool g_OverlayCreated = false;
static bool g_OverlayTransparent = false;

// ============================================================================
// DIRECTX
// ============================================================================
static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D11RenderTargetView* g_RenderTarget = nullptr;
static ID2D1Factory* g_D2DFactory = nullptr;
static ID2D1RenderTarget* g_D2DTarget = nullptr;
static ID2D1SolidColorBrush* g_Brush = nullptr;
static IDWriteFactory* g_DWriteFactory = nullptr;
static IDWriteTextFormat* g_TextFormat = nullptr;
static IDWriteTextFormat* g_SmallFont = nullptr;

// ============================================================================
// APP STATE
// ============================================================================
static bool g_Running = true;
static bool g_MenuVisible = true;
static int g_ActiveTab = 0;
static POINT g_Mouse = {0, 0};
static bool g_MouseDown = false;

// ============================================================================
// SETTINGS
// ============================================================================
struct Settings {
    bool blackOverlayEnabled = true;
    bool espEnabled = true;
    bool espBoxes = true;
    bool espHealth = true;
    bool espNames = true;
    bool espDistance = true;
    bool radarEnabled = true;
    float radarSize = 180.0f;
    float radarZoom = 1.5f;
    bool aimbotEnabled = false;
    float aimbotFOV = 30.0f;
    float aimbotSmooth = 5.0f;
    bool fovCircle = true;
    bool crosshair = true;
} g_Settings;

// ============================================================================
// COLORS
// ============================================================================
namespace Col {
    const D2D1_COLOR_F Black = {0, 0, 0, 1.0f};
    const D2D1_COLOR_F Red = {1.0f, 0.2f, 0.2f, 1.0f};
    const D2D1_COLOR_F Green = {0.2f, 1.0f, 0.2f, 1.0f};
    const D2D1_COLOR_F Yellow = {1.0f, 1.0f, 0.2f, 1.0f};
    const D2D1_COLOR_F White = {1.0f, 1.0f, 1.0f, 1.0f};
    const D2D1_COLOR_F Gray = {0.5f, 0.5f, 0.5f, 1.0f};
    const D2D1_COLOR_F DarkBG = {0.08f, 0.08f, 0.08f, 0.95f};
    const D2D1_COLOR_F Accent = {0.8f, 0.1f, 0.1f, 1.0f};
    const D2D1_COLOR_F Blue = {0.2f, 0.4f, 1.0f, 1.0f};
    const D2D1_COLOR_F TabBG = {0.15f, 0.15f, 0.15f, 1.0f};
    const D2D1_COLOR_F Orange = {1.0f, 0.6f, 0.2f, 1.0f};
}

// ============================================================================
// WINDOW PROC
// ============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_MOUSEMOVE:
        g_Mouse.x = LOWORD(lParam);
        g_Mouse.y = HIWORD(lParam);
        break;
    case WM_LBUTTONDOWN:
        g_MouseDown = true;
        break;
    case WM_LBUTTONUP:
        g_MouseDown = false;
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
// CREATE PROFESSIONAL OVERLAY (The Master Overlay)
// ============================================================================
bool CreateProfessionalOverlay()
{
    if (g_OverlayCreated)
        return true;
    
    // Get screen resolution
    g_ScreenW = GetSystemMetrics(SM_CXSCREEN);
    g_ScreenH = GetSystemMetrics(SM_CYSCREEN);
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);  // Solid Black
    wc.lpszClassName = L"ZeroMasterOverlay";
    
    if (!RegisterClassExW(&wc))
        return false;
    
    // Create SOLID BLACK window covering entire screen
    g_MasterOverlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        wc.lpszClassName, L"Zero Elite", WS_POPUP,
        0, 0, g_ScreenW, g_ScreenH,
        nullptr, nullptr, wc.hInstance, nullptr);
    
    if (!g_MasterOverlay)
        return false;
    
    // Make BLACK color transparent for see-through
    SetLayeredWindowAttributes(g_MasterOverlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    ShowWindow(g_MasterOverlay, SW_SHOW);
    UpdateWindow(g_MasterOverlay);
    
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(g_MasterOverlay, &margins);
    
    g_OverlayCreated = true;
    g_OverlayTransparent = true;
    SetBlackOverlayEnabled(true);
    
    return true;
}

// ============================================================================
// INIT GRAPHICS
// ============================================================================
bool InitGraphics()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = g_ScreenW;
    scd.BufferDesc.Height = g_ScreenH;
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = g_MasterOverlay;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
        &scd, &g_SwapChain, &g_Device, nullptr, &g_Context)))
        return false;
    
    ID3D11Texture2D* bb = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    g_Device->CreateRenderTargetView(bb, nullptr, &g_RenderTarget);
    bb->Release();
    
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_D2DFactory);
    
    IDXGISurface* surface = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);
    g_D2DFactory->CreateDxgiSurfaceRenderTarget(surface,
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
        &g_D2DTarget);
    surface->Release();
    
    g_D2DTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &g_Brush);
    
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_DWriteFactory);
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &g_TextFormat);
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &g_SmallFont);
    
    return true;
}

// ============================================================================
// INPUT THREAD
// ============================================================================
void InputThread()
{
    bool insertWasDown = false;
    while (g_Running)
    {
        bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (insertDown && !insertWasDown)
        {
            g_MenuVisible = !g_MenuVisible;
            if (g_MasterOverlay)
            {
                LONG_PTR style = GetWindowLongPtrW(g_MasterOverlay, GWL_EXSTYLE);
                if (g_MenuVisible)
                    style &= ~WS_EX_TRANSPARENT;
                else
                    style |= WS_EX_TRANSPARENT;
                SetWindowLongPtrW(g_MasterOverlay, GWL_EXSTYLE, style);
            }
        }
        insertWasDown = insertDown;
        
        if (GetAsyncKeyState(VK_END) & 0x8000)
            g_Running = false;
        
        Sleep(16);
    }
}

// ============================================================================
// DRAWING PRIMITIVES
// ============================================================================
inline void SetBrush(const D2D1_COLOR_F& c) { if (g_Brush) g_Brush->SetColor(c); }

void Text(const wchar_t* txt, float x, float y, const D2D1_COLOR_F& col, bool useSmall = false)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetBrush(col);
    D2D1_RECT_F r = {x, y, x + 500, y + 30};
    g_D2DTarget->DrawTextW(txt, (UINT32)wcslen(txt), useSmall ? g_SmallFont : g_TextFormat, r, g_Brush);
}

void FillRect(float x, float y, float w, float h, const D2D1_COLOR_F& c)
{
    if (!g_D2DTarget) return;
    SetBrush(c);
    g_D2DTarget->FillRectangle(D2D1::RectF(x, y, x + w, y + h), g_Brush);
}

void DrawRect(float x, float y, float w, float h, const D2D1_COLOR_F& c, float stroke = 2.0f)
{
    if (!g_D2DTarget) return;
    SetBrush(c);
    g_D2DTarget->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), g_Brush, stroke);
}

void DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& c, float stroke = 1.0f)
{
    if (!g_D2DTarget) return;
    SetBrush(c);
    g_D2DTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), g_Brush, stroke);
}

void DrawCircle(float cx, float cy, float r, const D2D1_COLOR_F& c, float stroke = 1.0f)
{
    if (!g_D2DTarget) return;
    SetBrush(c);
    g_D2DTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), g_Brush, stroke);
}

void FillCircle(float cx, float cy, float r, const D2D1_COLOR_F& c)
{
    if (!g_D2DTarget) return;
    SetBrush(c);
    g_D2DTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), g_Brush);
}

void RoundedRect(float x, float y, float w, float h, float r, const D2D1_COLOR_F& c)
{
    if (!g_D2DTarget) return;
    SetBrush(c);
    D2D1_ROUNDED_RECT rr = {D2D1::RectF(x, y, x + w, y + h), r, r};
    g_D2DTarget->FillRoundedRectangle(rr, g_Brush);
}

bool HitTest(float rx, float ry, float rw, float rh)
{
    return g_Mouse.x >= rx && g_Mouse.x <= rx + rw && g_Mouse.y >= ry && g_Mouse.y <= ry + rh;
}

// ============================================================================
// UI WIDGETS
// ============================================================================
bool Toggle(const wchar_t* label, float x, float y, bool* value)
{
    const float w = 40, h = 20;
    RoundedRect(x, y, w, h, h / 2, *value ? Col::Accent : Col::Gray);
    float knobX = *value ? x + w - h + 2 : x + 2;
    FillCircle(knobX + (h - 4) / 2, y + h / 2, (h - 4) / 2, Col::White);
    Text(label, x + w + 10, y, Col::White, true);
    
    if (g_MouseDown && HitTest(x, y, w + 200, h))
    {
        *value = !*value;
        g_MouseDown = false;
        return true;
    }
    return false;
}

bool Slider(const wchar_t* label, float x, float y, float* value, float minV, float maxV)
{
    const float w = 150, h = 6;
    Text(label, x, y - 18, Col::White, true);
    FillRect(x, y + 7, w, h, Col::Gray);
    float pct = (*value - minV) / (maxV - minV);
    FillRect(x, y + 7, w * pct, h, Col::Accent);
    FillCircle(x + w * pct, y + 10, 8, Col::White);
    
    wchar_t val[16];
    swprintf_s(val, L"%.1f", *value);
    Text(val, x + w + 10, y - 2, Col::Gray, true);
    
    if (g_MouseDown && HitTest(x - 5, y, w + 10, 20))
    {
        float newPct = ((float)g_Mouse.x - x) / w;
        *value = minV + max(0.0f, min(1.0f, newPct)) * (maxV - minV);
        return true;
    }
    return false;
}

// ============================================================================
// DRAW BOX ON OVERLAY (not desktop!)
// ============================================================================
void DrawBox(float x, float y, float w, float h, const D2D1_COLOR_F& col)
{
    if (!g_Settings.blackOverlayEnabled) return;  // NO drawing if overlay disabled
    DrawRect(x - w/2, y - h, w, h, col, 2.0f);
}

// ============================================================================
// RENDER ESP - Real DMA Data Only
// ============================================================================
void RenderESP()
{
    if (!g_Settings.blackOverlayEnabled) return;
    if (!g_Settings.espEnabled) return;
    if (!g_DMA_Online.load()) return;
    if (!IsDMAMemoryReadable()) return;
    
    auto& players = PlayerManager::GetPlayers();
    if (players.empty()) return;
    
    auto& local = PlayerManager::GetLocalPlayer();
    if (!local.valid) return;
    
    float centerX = (float)g_ScreenW / 2;
    float centerY = (float)g_ScreenH / 2;
    
    for (auto& player : players)
    {
        if (!player.valid || !player.isAlive) continue;
        if (player.origin.x == 0 && player.origin.y == 0) continue;
        
        // WorldToScreen conversion
        float angle = atan2f(player.origin.y - local.origin.y, player.origin.x - local.origin.x);
        float localAngle = local.yaw * 3.14159f / 180.0f;
        float relAngle = angle - localAngle;
        
        float dist = player.distance;
        float screenDist = min(dist * 3.0f, (float)g_ScreenH / 2 - 50);
        float screenX = centerX + cosf(relAngle) * screenDist;
        float screenY = centerY - sinf(relAngle) * screenDist;
        
        if (screenX < 0 || screenX > g_ScreenW || screenY < 0 || screenY > g_ScreenH)
            continue;
        
        float boxH = max(30.0f, 80.0f - dist * 0.3f);
        float boxW = boxH * 0.4f;
        
        D2D1_COLOR_F color = player.isEnemy ? Col::Red : Col::Green;
        
        // Draw inside Overlay only
        if (g_Settings.espBoxes)
            DrawBox(screenX, screenY, boxW, boxH, color);
        
        if (g_Settings.espHealth)
        {
            float healthPct = (float)player.health / (float)player.maxHealth;
            float barH = boxH * healthPct;
            D2D1_COLOR_F hpColor = {1.0f - healthPct, healthPct, 0, 1.0f};
            FillRect(screenX - boxW / 2 - 6, screenY - barH, 4, barH, hpColor);
        }
        
        if (g_Settings.espNames)
        {
            wchar_t nameW[32];
            mbstowcs_s(nullptr, nameW, player.name, 31);
            Text(nameW, screenX - 30, screenY - boxH - 18, Col::White, true);
        }
        
        if (g_Settings.espDistance)
        {
            wchar_t distStr[16];
            swprintf_s(distStr, L"%.0fm", player.distance);
            Text(distStr, screenX - 15, screenY + 5, Col::Yellow, true);
        }
    }
}

// ============================================================================
// RENDER RADAR
// ============================================================================
void RenderRadar()
{
    if (!g_Settings.blackOverlayEnabled) return;
    if (!g_Settings.radarEnabled) return;
    
    float size = g_Settings.radarSize;
    float cx = 20 + size / 2;
    float cy = 20 + size / 2;
    
    FillCircle(cx, cy, size / 2, D2D1_COLOR_F{0.1f, 0.1f, 0.1f, 0.85f});
    DrawCircle(cx, cy, size / 2, Col::Accent, 2.0f);
    DrawLine(cx - size / 2, cy, cx + size / 2, cy, D2D1_COLOR_F{0.3f, 0.3f, 0.3f, 0.5f}, 1.0f);
    DrawLine(cx, cy - size / 2, cx, cy + size / 2, D2D1_COLOR_F{0.3f, 0.3f, 0.3f, 0.5f}, 1.0f);
    FillCircle(cx, cy, 4, Col::Blue);
    
    if (!g_DMA_Online.load())
    {
        Text(L"DMA OFFLINE", cx - 35, cy + size / 2 + 5, Col::Red, true);
        return;
    }
    
    if (!IsDMAMemoryReadable())
    {
        Text(L"MEMORY ERROR", cx - 40, cy + size / 2 + 5, Col::Red, true);
        return;
    }
    
    auto& players = PlayerManager::GetPlayers();
    auto& local = PlayerManager::GetLocalPlayer();
    
    if (players.empty() || !local.valid)
    {
        Text(L"NO DATA", cx - 25, cy + size / 2 + 5, Col::Yellow, true);
        return;
    }
    
    for (auto& p : players)
    {
        if (!p.valid || !p.isAlive) continue;
        if (p.origin.x == 0 && p.origin.y == 0) continue;
        
        Vec2 radarPos;
        WorldToRadar(p.origin, local.origin, local.yaw, radarPos, cx, cy, size);
        
        float dx = radarPos.x - cx, dy = radarPos.y - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > size / 2 - 5)
        {
            float scale = (size / 2 - 5) / dist;
            radarPos.x = cx + dx * scale;
            radarPos.y = cy + dy * scale;
        }
        
        FillCircle(radarPos.x, radarPos.y, 4, p.isEnemy ? Col::Red : Col::Green);
    }
    
    wchar_t countStr[32];
    swprintf_s(countStr, L"Players: %d", (int)players.size());
    Text(countStr, cx - 30, cy + size / 2 + 5, Col::White, true);
}

// ============================================================================
// RENDER CROSSHAIR & FOV
// ============================================================================
void RenderCrosshair()
{
    if (!g_Settings.blackOverlayEnabled || !g_Settings.crosshair) return;
    float cx = (float)g_ScreenW / 2, cy = (float)g_ScreenH / 2;
    DrawLine(cx - 8, cy, cx + 8, cy, Col::White, 2.0f);
    DrawLine(cx, cy - 8, cx, cy + 8, Col::White, 2.0f);
}

void RenderFOVCircle()
{
    if (!g_Settings.blackOverlayEnabled || !g_Settings.aimbotEnabled || !g_Settings.fovCircle) return;
    float cx = (float)g_ScreenW / 2, cy = (float)g_ScreenH / 2;
    DrawCircle(cx, cy, g_Settings.aimbotFOV * 5, D2D1_COLOR_F{1, 1, 1, 0.3f}, 1.0f);
}

// ============================================================================
// RENDER MENU
// ============================================================================
void RenderMenu()
{
    if (!g_MenuVisible) return;
    
    const float menuW = 450, menuH = 480;
    float menuX = (float)g_ScreenW / 2 - menuW / 2;
    float menuY = (float)g_ScreenH / 2 - menuH / 2;
    
    RoundedRect(menuX, menuY, menuW, menuH, 10, Col::DarkBG);
    FillRect(menuX, menuY, menuW, 40, Col::Accent);
    Text(L"PROJECT ZERO | v5.0 Blurry-Style", menuX + 15, menuY + 8, Col::White, false);
    
    bool dmaOnline = g_DMA_Online.load();
    bool memOK = IsDMAMemoryReadable();
    FillCircle(menuX + menuW - 25, menuY + 20, 8, (dmaOnline && memOK) ? Col::Green : Col::Red);
    
    float tabY = menuY + 50;
    const wchar_t* tabNames[] = {L"ESP", L"RADAR", L"AIMBOT", L"SETTINGS", L"STATUS"};
    for (int i = 0; i < 5; i++)
    {
        float tabX = menuX + 10 + i * 86;
        RoundedRect(tabX, tabY, 82, 30, 5, (i == g_ActiveTab) ? Col::Accent : Col::TabBG);
        Text(tabNames[i], tabX + 15, tabY + 5, Col::White, true);
        if (g_MouseDown && HitTest(tabX, tabY, 82, 30)) { g_ActiveTab = i; g_MouseDown = false; }
    }
    
    float cX = menuX + 25, cY = tabY + 50;
    
    switch (g_ActiveTab)
    {
    case 0:
        Toggle(L"Enable ESP", cX, cY, &g_Settings.espEnabled);
        Toggle(L"Boxes", cX, cY + 35, &g_Settings.espBoxes);
        Toggle(L"Health Bars", cX, cY + 70, &g_Settings.espHealth);
        Toggle(L"Names", cX, cY + 105, &g_Settings.espNames);
        Toggle(L"Distance", cX, cY + 140, &g_Settings.espDistance);
        Toggle(L"Crosshair", cX, cY + 175, &g_Settings.crosshair);
        break;
        
    case 1:
        Toggle(L"Enable Radar", cX, cY, &g_Settings.radarEnabled);
        Slider(L"Size", cX, cY + 55, &g_Settings.radarSize, 100, 300);
        Slider(L"Zoom", cX, cY + 110, &g_Settings.radarZoom, 0.5f, 3.0f);
        break;
        
    case 2:
        Toggle(L"Enable Aimbot", cX, cY, &g_Settings.aimbotEnabled);
        Toggle(L"FOV Circle", cX, cY + 35, &g_Settings.fovCircle);
        Slider(L"FOV", cX, cY + 90, &g_Settings.aimbotFOV, 5, 100);
        Slider(L"Smooth", cX, cY + 145, &g_Settings.aimbotSmooth, 1, 20);
        {
            bool km = g_KMBox_Locked.load();
            wchar_t s[64]; swprintf_s(s, L"KMBox: %S", GetKMBoxStatus());
            Text(s, cX, cY + 200, km ? Col::Green : Col::Red, true);
            if (km) {
                wchar_t p[64]; swprintf_s(p, L"Port: %S", HardwareController::GetLockedPort());
                Text(p, cX, cY + 225, Col::Gray, true);
            }
        }
        break;
        
    case 3:
        Text(L"=== OVERLAY CONTROL ===", cX, cY, Col::Orange, true);
        if (Toggle(L"Enable Black Overlay", cX, cY + 30, &g_Settings.blackOverlayEnabled))
            SetBlackOverlayEnabled(g_Settings.blackOverlayEnabled);
        Text(L"Disabling stops ALL drawing.", cX, cY + 60, Col::Gray, true);
        
        Text(L"=== REQUIRED LIBS ===", cX, cY + 100, Col::Orange, true);
        Text(L"vmmdll.lib, leechcore.lib, FTD3XX.lib", cX, cY + 125, Col::White, true);
        break;
        
    case 4:
        {
            wchar_t ds[64]; swprintf_s(ds, L"DMA: %s", g_DMA_Online.load() ? L"ONLINE" : L"OFFLINE");
            Text(ds, cX, cY, g_DMA_Online.load() ? Col::Green : Col::Red, true);
            
            wchar_t diag[128]; swprintf_s(diag, L"Diagnostic: %S", GetDMADiagnostic());
            Text(diag, cX, cY + 25, IsDMAMemoryReadable() ? Col::Green : Col::Red, true);
            
            wchar_t ks[64]; swprintf_s(ks, L"KMBox: %S", GetKMBoxStatus());
            Text(ks, cX, cY + 55, g_KMBox_Locked.load() ? Col::Green : Col::Red, true);
            
            bool scanDone = g_HWScanDone.load();
            Text(scanDone ? L"Hardware: READY" : L"Hardware: SCANNING...", cX, cY + 85, scanDone ? Col::Green : Col::Yellow, true);
            
            wchar_t ovl[64]; swprintf_s(ovl, L"Overlay: %s", g_Settings.blackOverlayEnabled ? L"ACTIVE" : L"DISABLED");
            Text(ovl, cX, cY + 115, g_Settings.blackOverlayEnabled ? Col::Green : Col::Yellow, true);
            
            wchar_t rs[64]; swprintf_s(rs, L"Resolution: %dx%d", g_ScreenW, g_ScreenH);
            Text(rs, cX, cY + 145, Col::White, true);
            
            auto& players = PlayerManager::GetPlayers();
            wchar_t ps[64]; swprintf_s(ps, L"Players: %d", (int)players.size());
            Text(ps, cX, cY + 175, players.empty() ? Col::Yellow : Col::Green, true);
            
            Text(L"INSERT - Menu | END - Exit", cX, cY + 220, Col::Gray, true);
        }
        break;
    }
}

// ============================================================================
// CLEANUP
// ============================================================================
void Cleanup()
{
    if (g_SmallFont) g_SmallFont->Release();
    if (g_TextFormat) g_TextFormat->Release();
    if (g_DWriteFactory) g_DWriteFactory->Release();
    if (g_Brush) g_Brush->Release();
    if (g_D2DTarget) g_D2DTarget->Release();
    if (g_D2DFactory) g_D2DFactory->Release();
    if (g_RenderTarget) g_RenderTarget->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_Context) g_Context->Release();
    if (g_Device) g_Device->Release();
    DMAEngine::Shutdown();
}

// ============================================================================
// MAIN - Blurry-Style Architecture
// ============================================================================
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // Show diagnostic console
    Logger::ShowConsole();
    
    // --- [ 1. STARTUP DIAGNOSTICS ] ---
    RunStartupDiagnostics();
    g_DMA_Online = IsDMAOnline();
    g_KMBox_Locked = IsKMBoxConnected();
    g_HWScanDone = true;
    
    // --- [ 2. CREATE PROFESSIONAL OVERLAY (The Master Overlay) ] ---
    if (!CreateProfessionalOverlay())
    {
        std::cout << "[-] Failed to create overlay" << std::endl;
        return 0;
    }
    std::cout << "[+] Master Overlay: CREATED" << std::endl;
    
    // --- [ 3. INIT GRAPHICS ] ---
    if (!InitGraphics())
    {
        std::cout << "[-] Failed to init graphics" << std::endl;
        return 0;
    }
    std::cout << "[+] DirectX/Direct2D: READY" << std::endl;
    
    // Hide console after diagnostics
    Sleep(1500);
    Logger::HideConsole();
    
    // Init player manager
    PlayerManager::Initialize();
    
    // Start threads
    std::thread inputThread(InputThread);
    std::thread updateThread([]() {
        while (g_Running) {
            PlayerManager::Update();
            Sleep(16);
        }
    });
    
    // --- [ 4. REAL-TIME MEMORY LOOP ] ---
    MSG msg = {};
    while (g_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) g_Running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        if (!g_Running) break;
        
        float clearColor[4] = {0, 0, 0, 1};
        g_Context->ClearRenderTargetView(g_RenderTarget, clearColor);
        
        g_D2DTarget->BeginDraw();
        g_D2DTarget->Clear(Col::Black);
        
        // ONLY DRAW ON OVERLAY - NO DESKTOP GHOSTS
        if (g_Settings.blackOverlayEnabled)
        {
            RenderESP();
            RenderRadar();
            RenderCrosshair();
            RenderFOVCircle();
        }
        
        RenderMenu();
        
        g_D2DTarget->EndDraw();
        g_SwapChain->Present(0, 0);
        
        Sleep(1);
    }
    
    g_Running = false;
    inputThread.join();
    updateThread.join();
    Cleanup();
    
    return 0;
}
