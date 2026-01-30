// PROJECT ZERO | BO6 DMA - Professional Overlay
// Auto-Launch Menu, Mouse Control, Scatter Reads, Pattern Scanner

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
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
    // Aimbot
    bool aimbotEnabled = false;
    float aimbotFOV = 120.0f;
    float aimbotSmooth = 5.0f;
    int aimbotBone = 0;  // 0=Head, 1=Neck, 2=Chest
    bool aimbotVisCheck = true;
    bool aimbotShowFOV = true;
    float aimbotMaxDist = 200.0f;
    
    // Visuals / ESP
    bool espEnabled = true;
    bool espBox = true;
    bool espBoxCorner = false;
    bool espSkeleton = false;
    bool espHealth = true;
    bool espName = true;
    bool espDistance = true;
    bool espSnapline = false;
    int espSnaplinePos = 1;  // 0=Top, 1=Center, 2=Bottom
    bool espTeammates = false;
    
    // Radar
    bool radarEnabled = true;
    float radarSize = 220.0f;
    float radarZoom = 1.5f;
    bool radarShowEnemy = true;
    bool radarShowTeam = true;
    
    // Misc
    bool noRecoil = false;
    bool rapidFire = false;
    bool crosshair = true;
    int crosshairType = 0;  // 0=Cross, 1=Dot, 2=Circle
    float crosshairSize = 8.0f;
    bool streamProof = false;
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
static IDWriteTextFormat* g_FontBold = nullptr;
static IDWriteTextFormat* g_FontSmall = nullptr;

// State - MENU VISIBLE BY DEFAULT
static std::atomic<bool> g_Running(true);
static bool g_MenuVisible = true;  // ALWAYS SHOW ON START
static int g_CurrentTab = 0;
static POINT g_Mouse = {0, 0};
static bool g_MouseDown = false;
static bool g_MouseClicked = false;
static int g_DragSlider = -1;

// ============================================================================
// COLORS (Red/Dark Theme)
// ============================================================================
namespace Colors {
    D2D1_COLOR_F Background = {0.05f, 0.05f, 0.05f, 0.97f};
    D2D1_COLOR_F Panel = {0.08f, 0.08f, 0.08f, 1.0f};
    D2D1_COLOR_F Border = {0.15f, 0.15f, 0.15f, 1.0f};
    D2D1_COLOR_F Red = {0.9f, 0.15f, 0.15f, 1.0f};
    D2D1_COLOR_F RedDark = {0.5f, 0.1f, 0.1f, 1.0f};
    D2D1_COLOR_F RedBright = {1.0f, 0.2f, 0.2f, 1.0f};
    D2D1_COLOR_F Green = {0.15f, 0.9f, 0.3f, 1.0f};
    D2D1_COLOR_F Blue = {0.3f, 0.5f, 1.0f, 1.0f};
    D2D1_COLOR_F White = {1.0f, 1.0f, 1.0f, 1.0f};
    D2D1_COLOR_F Gray = {0.4f, 0.4f, 0.4f, 1.0f};
    D2D1_COLOR_F GrayLight = {0.6f, 0.6f, 0.6f, 1.0f};
    D2D1_COLOR_F GrayDark = {0.2f, 0.2f, 0.2f, 1.0f};
    D2D1_COLOR_F Yellow = {1.0f, 0.9f, 0.2f, 1.0f};
    D2D1_COLOR_F Transparent = {0, 0, 0, 0};
}

// ============================================================================
// INPUT THREAD
// ============================================================================
void InputThread()
{
    bool insertWasDown = false;
    
    while (g_Running)
    {
        // INSERT to toggle menu
        bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (insertDown && !insertWasDown)
        {
            g_MenuVisible = !g_MenuVisible;
            
            // Update click-through
            LONG_PTR style = GetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE);
            if (g_MenuVisible)
                style &= ~WS_EX_TRANSPARENT;
            else
                style |= WS_EX_TRANSPARENT;
            SetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE, style);
        }
        insertWasDown = insertDown;
        
        // END to exit
        if (GetAsyncKeyState(VK_END) & 0x8000)
        {
            g_Running = false;
        }
        
        Sleep(10);
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
        PlayerManager::UpdateWithScatter();
        Sleep(8);  // ~120Hz update
    }
}

// ============================================================================
// WINDOW SETUP
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
        g_DragSlider = -1;
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
    
    // Create transparent overlay - NOT click-through by default (menu visible)
    g_Hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"",
        WS_POPUP,
        0, 0, g_ScreenW, g_ScreenH,
        nullptr, nullptr, wc.hInstance, nullptr);
    
    if (!g_Hwnd) return false;
    
    SetLayeredWindowAttributes(g_Hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(g_Hwnd, &margins);
    
    ShowWindow(g_Hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_Hwnd);
    
    return true;
}

// ============================================================================
// GRAPHICS INIT
// ============================================================================
bool InitGraphics()
{
    // D3D11 - V-Sync OFF
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
    
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        flags, nullptr, 0, D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_D3DDevice, nullptr, &g_D3DContext)))
        return false;
    
    ID3D11Texture2D* backBuffer;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_D3DDevice->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
    backBuffer->Release();
    
    // D2D
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_D2DFactory);
    
    IDXGISurface* surface;
    g_SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);
    
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    g_D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &g_D2DTarget);
    surface->Release();
    
    g_D2DTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &g_Brush);
    
    // DWrite fonts
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_DWriteFactory);
    
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"", &g_Font);
    
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"", &g_FontBold);
    
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 11.0f, L"", &g_FontSmall);
    
    return true;
}

void Cleanup()
{
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
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================
void DrawText(const wchar_t* text, float x, float y, D2D1_COLOR_F col, IDWriteTextFormat* font = nullptr)
{
    if (!font) font = g_Font;
    g_Brush->SetColor(col);
    D2D1_RECT_F r = {x, y, x + 500, y + 50};
    g_D2DTarget->DrawText(text, (UINT32)wcslen(text), font, r, g_Brush);
}

void FillRect(float x, float y, float w, float h, D2D1_COLOR_F col)
{
    g_Brush->SetColor(col);
    g_D2DTarget->FillRectangle({x, y, x + w, y + h}, g_Brush);
}

void DrawRect(float x, float y, float w, float h, D2D1_COLOR_F col, float thick = 1.0f)
{
    g_Brush->SetColor(col);
    g_D2DTarget->DrawRectangle({x, y, x + w, y + h}, g_Brush, thick);
}

void DrawRoundedRect(float x, float y, float w, float h, float r, D2D1_COLOR_F col, bool fill)
{
    g_Brush->SetColor(col);
    D2D1_ROUNDED_RECT rr = {{x, y, x + w, y + h}, r, r};
    if (fill) g_D2DTarget->FillRoundedRectangle(rr, g_Brush);
    else g_D2DTarget->DrawRoundedRectangle(rr, g_Brush, 2.0f);
}

void DrawCircle(float cx, float cy, float r, D2D1_COLOR_F col, bool fill, float thick = 1.0f)
{
    g_Brush->SetColor(col);
    D2D1_ELLIPSE e = {{cx, cy}, r, r};
    if (fill) g_D2DTarget->FillEllipse(e, g_Brush);
    else g_D2DTarget->DrawEllipse(e, g_Brush, thick);
}

void DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F col, float thick = 1.0f)
{
    g_Brush->SetColor(col);
    g_D2DTarget->DrawLine({x1, y1}, {x2, y2}, g_Brush, thick);
}

bool InRect(float x, float y, float w, float h)
{
    return g_Mouse.x >= x && g_Mouse.x <= x + w && g_Mouse.y >= y && g_Mouse.y <= y + h;
}

// ============================================================================
// UI WIDGETS
// ============================================================================
static int g_WidgetID = 0;

bool Toggle(const wchar_t* label, bool* value, float x, float y)
{
    int id = g_WidgetID++;
    float toggleW = 40, toggleH = 20;
    float totalW = 220;
    
    bool hovered = InRect(x, y, totalW, toggleH + 4);
    bool clicked = hovered && g_MouseClicked;
    
    if (clicked) *value = !*value;
    
    // Label
    DrawText(label, x, y + 2, hovered ? Colors::White : Colors::GrayLight);
    
    // Toggle track
    float tx = x + 170;
    D2D1_COLOR_F trackCol = *value ? Colors::Red : Colors::GrayDark;
    DrawRoundedRect(tx, y + 2, toggleW, toggleH, toggleH / 2, trackCol, true);
    
    // Toggle knob
    float knobX = *value ? tx + toggleW - toggleH + 2 : tx + 2;
    DrawCircle(knobX + toggleH / 2 - 2, y + 2 + toggleH / 2, toggleH / 2 - 3, Colors::White, true);
    
    return clicked;
}

void Slider(const wchar_t* label, float* value, float minV, float maxV, const wchar_t* fmt, float x, float y, int sliderID)
{
    g_WidgetID++;
    float sliderW = 150, sliderH = 8;
    float labelW = 100;
    
    float sx = x + labelW;
    float sy = y + 8;
    
    bool hovered = InRect(sx - 5, y, sliderW + 10, 24);
    
    if (hovered && g_MouseDown) g_DragSlider = sliderID;
    
    if (g_DragSlider == sliderID && g_MouseDown)
    {
        float pct = ((float)g_Mouse.x - sx) / sliderW;
        pct = pct < 0 ? 0 : (pct > 1 ? 1 : pct);
        *value = minV + pct * (maxV - minV);
    }
    
    // Label
    DrawText(label, x, y, Colors::GrayLight);
    
    // Value
    wchar_t buf[32];
    swprintf_s(buf, fmt, *value);
    DrawText(buf, sx + sliderW + 10, y, Colors::Red);
    
    // Track
    FillRect(sx, sy, sliderW, sliderH, Colors::GrayDark);
    
    // Fill
    float pct = (*value - minV) / (maxV - minV);
    FillRect(sx, sy, sliderW * pct, sliderH, Colors::Red);
    
    // Knob
    float kx = sx + sliderW * pct;
    DrawCircle(kx, sy + sliderH / 2, 7, (hovered || g_DragSlider == sliderID) ? Colors::White : Colors::Gray, true);
}

void Combo(const wchar_t* label, int* value, const wchar_t** options, int count, float x, float y)
{
    g_WidgetID++;
    float comboW = 140, comboH = 24;
    float labelW = 100;
    
    float cx = x + labelW;
    bool hovered = InRect(cx, y, comboW, comboH);
    
    if (hovered && g_MouseClicked) *value = (*value + 1) % count;
    
    DrawText(label, x, y + 3, Colors::GrayLight);
    
    DrawRoundedRect(cx, y, comboW, comboH, 4, hovered ? Colors::Gray : Colors::GrayDark, true);
    DrawText(options[*value], cx + 10, y + 3, Colors::White);
    DrawText(L"▼", cx + comboW - 20, y + 3, Colors::Gray);
}

bool TabButton(const wchar_t* label, float x, float y, float w, float h, bool selected)
{
    g_WidgetID++;
    bool hovered = InRect(x, y, w, h);
    bool clicked = hovered && g_MouseClicked;
    
    D2D1_COLOR_F col = selected ? Colors::Red : (hovered ? Colors::RedDark : Colors::GrayDark);
    DrawRoundedRect(x, y, w, h, 6, col, true);
    
    // Center text approximately
    DrawText(label, x + 12, y + h / 2 - 8, Colors::White);
    
    return clicked;
}

// ============================================================================
// RENDER RADAR (Standalone Window)
// ============================================================================
void RenderRadar()
{
    if (!g_Settings.radarEnabled) return;
    
    std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
    
    float size = g_Settings.radarSize;
    float rx = (float)g_ScreenW - size - 20;
    float ry = 20;
    float cx = rx + size / 2;
    float cy = ry + size / 2;
    
    // Background
    DrawRoundedRect(rx, ry, size, size, 8, Colors::Background, true);
    DrawRoundedRect(rx, ry, size, size, 8, Colors::Red, false);
    
    // Grid
    D2D1_COLOR_F gridCol = {0.15f, 0.15f, 0.2f, 0.8f};
    DrawLine(cx, ry + 8, cx, ry + size - 8, gridCol, 1);
    DrawLine(rx + 8, cy, rx + size - 8, cy, gridCol, 1);
    
    for (int i = 1; i <= 3; i++)
    {
        float r = (size * 0.42f) * ((float)i / 3.0f);
        DrawCircle(cx, cy, r, gridCol, false, 1);
    }
    
    // Players
    PlayerData& local = PlayerManager::GetLocalPlayer();
    auto& players = PlayerManager::GetPlayers();
    
    for (const PlayerData& p : players)
    {
        if (!p.valid || !p.isAlive) continue;
        if (p.isEnemy && !g_Settings.radarShowEnemy) continue;
        if (!p.isEnemy && !g_Settings.radarShowTeam) continue;
        
        Vec2 radarPos;
        float scale = (size * 0.4f) / (100.0f / g_Settings.radarZoom);
        WorldToRadar(p.origin, local.origin, local.yaw, radarPos, cx, cy, scale);
        
        // Clamp to radar bounds
        float maxDist = size * 0.42f;
        Vec2 delta = {radarPos.x - cx, radarPos.y - cy};
        float dist = delta.Length();
        if (dist > maxDist)
        {
            radarPos.x = cx + delta.x / dist * maxDist;
            radarPos.y = cy + delta.y / dist * maxDist;
        }
        
        D2D1_COLOR_F col = p.isEnemy ? Colors::Red : Colors::Blue;
        
        // Glow
        D2D1_COLOR_F glow = col;
        glow.a = 0.3f;
        DrawCircle(radarPos.x, radarPos.y, 7, glow, true);
        
        // Dot
        DrawCircle(radarPos.x, radarPos.y, 4, col, true);
        
        // Direction arrow
        float yawRad = p.yaw * 3.14159f / 180.0f;
        float dx = sinf(yawRad) * 10;
        float dy = -cosf(yawRad) * 10;
        DrawLine(radarPos.x, radarPos.y, radarPos.x + dx, radarPos.y + dy, col, 2);
    }
    
    // Local player (center)
    DrawCircle(cx, cy, 6, Colors::Green, true);
    float yawRad = local.yaw * 3.14159f / 180.0f;
    DrawLine(cx, cy, cx + sinf(yawRad) * 15, cy - cosf(yawRad) * 15, Colors::Green, 2.5f);
    
    // Labels
    DrawText(L"RADAR", rx + 8, ry + size + 3, Colors::White, g_FontSmall);
    
    wchar_t buf[32];
    swprintf_s(buf, L"%d players", PlayerManager::GetAliveCount());
    DrawText(buf, rx + size - 70, ry + size + 3, Colors::Gray, g_FontSmall);
}

// ============================================================================
// RENDER ESP
// ============================================================================
void RenderESP()
{
    if (!g_Settings.espEnabled) return;
    
    std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
    
    auto& players = PlayerManager::GetPlayers();
    Vec2 screenCenter = {(float)g_ScreenW / 2, (float)g_ScreenH / 2};
    
    for (PlayerData& p : players)
    {
        if (!p.valid || !p.isAlive) continue;
        if (!p.isEnemy && !g_Settings.espTeammates) continue;
        
        // Simulate screen position (in real impl, use WorldToScreen with view matrix)
        float simScale = 500.0f / (p.distance + 10.0f);
        p.screenPos.x = screenCenter.x + p.origin.x * simScale * 0.5f;
        p.screenPos.y = screenCenter.y - p.origin.y * simScale * 0.5f;
        p.screenHeight = 100.0f * simScale;
        p.screenWidth = p.screenHeight * 0.4f;
        p.onScreen = (p.screenPos.x > 0 && p.screenPos.x < g_ScreenW &&
                      p.screenPos.y > 0 && p.screenPos.y < g_ScreenH);
        
        if (!p.onScreen) continue;
        
        D2D1_COLOR_F col = p.isEnemy ? Colors::Red : Colors::Blue;
        if (p.isVisible) col = Colors::Yellow;
        
        float bx = p.screenPos.x - p.screenWidth / 2;
        float by = p.screenPos.y - p.screenHeight;
        float bw = p.screenWidth;
        float bh = p.screenHeight;
        
        // Box ESP
        if (g_Settings.espBox)
        {
            if (g_Settings.espBoxCorner)
            {
                float cornerLen = bw * 0.25f;
                // Top left
                DrawLine(bx, by, bx + cornerLen, by, col, 2);
                DrawLine(bx, by, bx, by + cornerLen, col, 2);
                // Top right
                DrawLine(bx + bw, by, bx + bw - cornerLen, by, col, 2);
                DrawLine(bx + bw, by, bx + bw, by + cornerLen, col, 2);
                // Bottom left
                DrawLine(bx, by + bh, bx + cornerLen, by + bh, col, 2);
                DrawLine(bx, by + bh, bx, by + bh - cornerLen, col, 2);
                // Bottom right
                DrawLine(bx + bw, by + bh, bx + bw - cornerLen, by + bh, col, 2);
                DrawLine(bx + bw, by + bh, bx + bw, by + bh - cornerLen, col, 2);
            }
            else
            {
                DrawRect(bx, by, bw, bh, col, 2);
            }
        }
        
        // Health bar
        if (g_Settings.espHealth)
        {
            float hbW = 4;
            float hbH = bh;
            float hbX = bx - hbW - 3;
            float hbY = by;
            float hpPct = (float)p.health / (float)p.maxHealth;
            
            // Background
            FillRect(hbX, hbY, hbW, hbH, Colors::GrayDark);
            
            // Health fill (green to red gradient based on HP)
            D2D1_COLOR_F hpCol = {1.0f - hpPct, hpPct, 0.1f, 1.0f};
            FillRect(hbX, hbY + hbH * (1 - hpPct), hbW, hbH * hpPct, hpCol);
        }
        
        // Name
        if (g_Settings.espName)
        {
            wchar_t nameBuf[64];
            mbstowcs_s(nullptr, nameBuf, p.name, 63);
            DrawText(nameBuf, bx, by - 16, col, g_FontSmall);
        }
        
        // Distance
        if (g_Settings.espDistance)
        {
            wchar_t distBuf[16];
            swprintf_s(distBuf, L"%.0fm", p.distance);
            DrawText(distBuf, bx, by + bh + 2, Colors::White, g_FontSmall);
        }
        
        // Snaplines
        if (g_Settings.espSnapline)
        {
            float startY;
            switch (g_Settings.espSnaplinePos)
            {
            case 0: startY = 0; break;
            case 1: startY = (float)g_ScreenH / 2; break;
            default: startY = (float)g_ScreenH; break;
            }
            DrawLine((float)g_ScreenW / 2, startY, p.screenPos.x, p.screenPos.y, col, 1);
        }
    }
}

// ============================================================================
// RENDER CROSSHAIR
// ============================================================================
void RenderCrosshair()
{
    if (!g_Settings.crosshair) return;
    
    float cx = (float)g_ScreenW / 2;
    float cy = (float)g_ScreenH / 2;
    float size = g_Settings.crosshairSize;
    
    switch (g_Settings.crosshairType)
    {
    case 0:  // Cross
        DrawLine(cx - size, cy, cx + size, cy, Colors::White, 2);
        DrawLine(cx, cy - size, cx, cy + size, Colors::White, 2);
        break;
    case 1:  // Dot
        DrawCircle(cx, cy, 3, Colors::White, true);
        break;
    case 2:  // Circle
        DrawCircle(cx, cy, size, Colors::White, false, 2);
        DrawCircle(cx, cy, 2, Colors::White, true);
        break;
    }
}

// ============================================================================
// RENDER AIMBOT FOV
// ============================================================================
void RenderAimbotFOV()
{
    if (!g_Settings.aimbotEnabled || !g_Settings.aimbotShowFOV) return;
    
    float cx = (float)g_ScreenW / 2;
    float cy = (float)g_ScreenH / 2;
    
    D2D1_COLOR_F fovCol = Colors::Red;
    fovCol.a = 0.3f;
    DrawCircle(cx, cy, g_Settings.aimbotFOV, fovCol, false, 1.5f);
}

// ============================================================================
// RENDER MENU
// ============================================================================
void RenderMenu()
{
    if (!g_MenuVisible) return;
    
    g_WidgetID = 0;
    
    float mx = 40, my = 40;
    float mw = 520, mh = 480;
    
    // Background
    DrawRoundedRect(mx, my, mw, mh, 10, Colors::Background, true);
    DrawRoundedRect(mx, my, mw, mh, 10, Colors::RedDark, false);
    
    // Title bar
    DrawRoundedRect(mx + 2, my + 2, mw - 4, 42, 8, Colors::RedDark, true);
    DrawText(L"PROJECT ZERO | BO6 DMA", mx + 15, my + 10, Colors::White, g_FontBold);
    
    // Status indicator
    DrawText(L"STATUS:", mx + 320, my + 13, Colors::Gray);
    const char* status = DMAEngine::GetStatus();
    wchar_t statusW[32];
    mbstowcs_s(nullptr, statusW, status, 31);
    
    // Green if ONLINE, red/yellow otherwise
    D2D1_COLOR_F statusCol = Colors::Red;
    if (strcmp(status, "ONLINE") == 0) statusCol = Colors::Green;
    else if (strcmp(status, "SIMULATION") == 0) statusCol = Colors::Yellow;
    
    DrawText(statusW, mx + 395, my + 13, statusCol, g_FontBold);
    
    // Tabs
    float ty = my + 52;
    float tw = 95, th = 30;
    const wchar_t* tabs[] = {L"AIMBOT", L"VISUALS", L"RADAR", L"MISC"};
    
    for (int i = 0; i < 4; i++)
    {
        float tx = mx + 15 + i * (tw + 8);
        if (TabButton(tabs[i], tx, ty, tw, th, g_CurrentTab == i))
            g_CurrentTab = i;
    }
    
    // Content area
    float cx = mx + 20;
    float cy = ty + 45;
    float lineH = 30;
    int line = 0;
    
    // ==================== AIMBOT TAB ====================
    if (g_CurrentTab == 0)
    {
        Toggle(L"Enable Aimbot", &g_Settings.aimbotEnabled, cx, cy + line * lineH); line++;
        
        if (g_Settings.aimbotEnabled)
        {
            line++;
            Slider(L"FOV", &g_Settings.aimbotFOV, 10, 300, L"%.0f", cx, cy + line * lineH, 1); line++;
            Slider(L"Smoothness", &g_Settings.aimbotSmooth, 1, 20, L"%.1f", cx, cy + line * lineH, 2); line++;
            Slider(L"Max Distance", &g_Settings.aimbotMaxDist, 10, 500, L"%.0fm", cx, cy + line * lineH, 3); line++;
            
            line++;
            const wchar_t* bones[] = {L"Head", L"Neck", L"Chest"};
            Combo(L"Target Bone", &g_Settings.aimbotBone, bones, 3, cx, cy + line * lineH); line++;
            
            line++;
            Toggle(L"Visibility Check", &g_Settings.aimbotVisCheck, cx, cy + line * lineH); line++;
            Toggle(L"Show FOV Circle", &g_Settings.aimbotShowFOV, cx, cy + line * lineH); line++;
        }
    }
    
    // ==================== VISUALS TAB ====================
    else if (g_CurrentTab == 1)
    {
        Toggle(L"Enable ESP", &g_Settings.espEnabled, cx, cy + line * lineH); line++;
        
        if (g_Settings.espEnabled)
        {
            line++;
            Toggle(L"Box ESP", &g_Settings.espBox, cx, cy + line * lineH); line++;
            if (g_Settings.espBox)
            {
                Toggle(L"Corner Style", &g_Settings.espBoxCorner, cx + 20, cy + line * lineH); line++;
            }
            Toggle(L"Skeleton ESP", &g_Settings.espSkeleton, cx, cy + line * lineH); line++;
            Toggle(L"Health Bars", &g_Settings.espHealth, cx, cy + line * lineH); line++;
            Toggle(L"Name Tags", &g_Settings.espName, cx, cy + line * lineH); line++;
            Toggle(L"Distance", &g_Settings.espDistance, cx, cy + line * lineH); line++;
            Toggle(L"Snaplines", &g_Settings.espSnapline, cx, cy + line * lineH); line++;
            
            if (g_Settings.espSnapline)
            {
                const wchar_t* snapPos[] = {L"Top", L"Center", L"Bottom"};
                Combo(L"Snapline From", &g_Settings.espSnaplinePos, snapPos, 3, cx + 20, cy + line * lineH); line++;
            }
            
            line++;
            Toggle(L"Show Teammates", &g_Settings.espTeammates, cx, cy + line * lineH); line++;
        }
    }
    
    // ==================== RADAR TAB ====================
    else if (g_CurrentTab == 2)
    {
        Toggle(L"Enable Radar", &g_Settings.radarEnabled, cx, cy + line * lineH); line++;
        
        line++;
        Slider(L"Size", &g_Settings.radarSize, 150, 350, L"%.0f px", cx, cy + line * lineH, 10); line++;
        Slider(L"Zoom", &g_Settings.radarZoom, 0.5f, 4.0f, L"%.1fx", cx, cy + line * lineH, 11); line++;
        
        line++;
        Toggle(L"Show Enemies", &g_Settings.radarShowEnemy, cx, cy + line * lineH); line++;
        Toggle(L"Show Teammates", &g_Settings.radarShowTeam, cx, cy + line * lineH); line++;
        
        line++; line++;
        wchar_t infoBuf[64];
        swprintf_s(infoBuf, L"Players: %d  |  Enemies: %d", 
                   PlayerManager::GetAliveCount(), PlayerManager::GetEnemyCount());
        DrawText(infoBuf, cx, cy + line * lineH, Colors::White);
    }
    
    // ==================== MISC TAB ====================
    else if (g_CurrentTab == 3)
    {
        Toggle(L"No Recoil", &g_Settings.noRecoil, cx, cy + line * lineH); line++;
        Toggle(L"Rapid Fire", &g_Settings.rapidFire, cx, cy + line * lineH); line++;
        
        line++;
        Toggle(L"Crosshair", &g_Settings.crosshair, cx, cy + line * lineH); line++;
        if (g_Settings.crosshair)
        {
            const wchar_t* types[] = {L"Cross", L"Dot", L"Circle"};
            Combo(L"Style", &g_Settings.crosshairType, types, 3, cx + 20, cy + line * lineH); line++;
            Slider(L"Size", &g_Settings.crosshairSize, 3, 20, L"%.0f", cx + 20, cy + line * lineH, 20); line++;
        }
        
        line++;
        Toggle(L"Stream Proof", &g_Settings.streamProof, cx, cy + line * lineH); line++;
        
        line++; line++;
        DrawText(L"Controls", cx, cy + line * lineH, Colors::Gray); line++;
        DrawText(L"INSERT - Toggle Menu", cx, cy + line * lineH, Colors::White, g_FontSmall); line++;
        DrawText(L"END - Exit Program", cx, cy + line * lineH, Colors::White, g_FontSmall); line++;
        
        line++; line++;
        DrawText(L"PROJECT ZERO v2.1", cx, cy + line * lineH, Colors::Red);
    }
    
    g_MouseClicked = false;
}

// ============================================================================
// MAIN
// ============================================================================
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // Initialize DMA
    DMAEngine::Initialize();
    PatternScanner::UpdateAllOffsets();
    
    // Create overlay window
    if (!CreateOverlayWindow()) return 1;
    if (!InitGraphics()) return 1;
    
    // Start threads
    std::thread inputThread(InputThread);
    std::thread updateThread(UpdateThread);
    
    // Main loop
    while (g_Running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        // Clear
        float clearColor[4] = {0, 0, 0, 0};
        g_D3DContext->ClearRenderTargetView(g_RenderTarget, clearColor);
        
        // Render
        g_D2DTarget->BeginDraw();
        g_D2DTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
        
        RenderESP();
        RenderAimbotFOV();
        RenderCrosshair();
        RenderRadar();
        RenderMenu();
        
        g_D2DTarget->EndDraw();
        
        // Present (V-Sync OFF)
        g_SwapChain->Present(0, 0);
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
