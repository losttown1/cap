// ZeroMain.cpp - STRICT Black Overlay v4.1
// Solid Black Window + Color Keying (Professional Method)

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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

#include "DMA_Engine.hpp"

// External functions from DMA_Engine.cpp
extern const char* GetKMBoxStatus();
extern bool IsKMBoxConnected();

// ============================================================================
// SETTINGS
// ============================================================================
struct AppSettings {
    bool aimbotEnabled = false;
    float aimbotFOV = 30.0f;
    float aimbotSmooth = 5.0f;
    bool aimbotFOVCircle = true;
    
    bool espEnabled = true;
    bool espBoxes = true;
    bool espHealth = true;
    bool espNames = true;
    bool espDistance = true;
    
    bool radarEnabled = true;
    float radarSize = 180.0f;
    float radarZoom = 1.5f;
    
    bool crosshair = true;
};

static AppSettings g_Settings;

// ============================================================================
// GLOBALS - THE BLACK WINDOW
// ============================================================================
static HWND g_BlackWindow = nullptr;
static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D11RenderTargetView* g_RenderTarget = nullptr;
static ID2D1Factory* g_D2DFactory = nullptr;
static ID2D1RenderTarget* g_D2DTarget = nullptr;
static ID2D1SolidColorBrush* g_Brush = nullptr;
static IDWriteFactory* g_DWriteFactory = nullptr;
static IDWriteTextFormat* g_TextFormat = nullptr;
static IDWriteTextFormat* g_SmallTextFormat = nullptr;

// RESOLUTION SYNC - Detect monitor resolution
static int g_MonitorWidth = 0;
static int g_MonitorHeight = 0;

static bool g_MenuVisible = false;
static bool g_Running = true;
static std::atomic<bool> g_GameRunning(false);
static int g_CurrentTab = 0;

// ============================================================================
// COLORS - Black is the key color (will be transparent)
// ============================================================================
namespace Colors {
    const D2D1_COLOR_F Black = {0, 0, 0, 1.0f};       // This becomes transparent
    const D2D1_COLOR_F Red = {1.0f, 0.2f, 0.2f, 1.0f};
    const D2D1_COLOR_F Green = {0.2f, 1.0f, 0.2f, 1.0f};
    const D2D1_COLOR_F Yellow = {1.0f, 1.0f, 0.2f, 1.0f};
    const D2D1_COLOR_F White = {1.0f, 1.0f, 1.0f, 1.0f};
    const D2D1_COLOR_F Gray = {0.5f, 0.5f, 0.5f, 1.0f};
    const D2D1_COLOR_F DarkGray = {0.15f, 0.15f, 0.15f, 0.95f};
    const D2D1_COLOR_F Accent = {0.8f, 0.1f, 0.1f, 1.0f};
    const D2D1_COLOR_F MenuBG = {0.08f, 0.08f, 0.08f, 0.95f};
    const D2D1_COLOR_F Blue = {0.2f, 0.4f, 1.0f, 1.0f};
}

// ============================================================================
// INPUT STATE
// ============================================================================
static POINT g_MousePos = {0, 0};
static bool g_MouseDown = false;

// ============================================================================
// WINDOW PROCEDURE FOR BLACK WINDOW
// ============================================================================
LRESULT CALLBACK BlackWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_MOUSEMOVE:
        if (g_MenuVisible) {
            g_MousePos.x = LOWORD(lParam);
            g_MousePos.y = HIWORD(lParam);
        }
        break;
    case WM_LBUTTONDOWN:
        if (g_MenuVisible) g_MouseDown = true;
        break;
    case WM_LBUTTONUP:
        if (g_MenuVisible) g_MouseDown = false;
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
// INPUT THREAD
// ============================================================================
void InputThread()
{
    bool insertWasPressed = false;
    while (g_Running)
    {
        bool insertPressed = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (insertPressed && !insertWasPressed)
        {
            g_MenuVisible = !g_MenuVisible;
            if (g_BlackWindow)
            {
                LONG_PTR style = GetWindowLongPtrW(g_BlackWindow, GWL_EXSTYLE);
                if (g_MenuVisible)
                    style &= ~WS_EX_TRANSPARENT;
                else
                    style |= WS_EX_TRANSPARENT;
                SetWindowLongPtrW(g_BlackWindow, GWL_EXSTYLE, style);
            }
        }
        insertWasPressed = insertPressed;
        
        if (GetAsyncKeyState(VK_END) & 0x8000)
            g_Running = false;
        
        Sleep(16);
    }
}

// ============================================================================
// UPDATE THREAD
// ============================================================================
void UpdateThread()
{
    while (g_Running)
    {
        g_GameRunning = DMAEngine::IsOnline();
        if (g_GameRunning)
            PlayerManager::Update();
        Sleep(16);
    }
}

// ============================================================================
// DRAWING HELPERS - ALL DRAWING ON BLACK WINDOW
// ============================================================================
inline void SetColor(const D2D1_COLOR_F& c) { if (g_Brush) g_Brush->SetColor(c); }

void RenderString(const wchar_t* txt, float x, float y, const D2D1_COLOR_F& col, bool small)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetColor(col);
    D2D1_RECT_F r = {x, y, x + 400, y + 30};
    g_D2DTarget->DrawTextW(txt, (UINT32)wcslen(txt), small ? g_SmallTextFormat : g_TextFormat, r, g_Brush);
}

void FillRect_(float x, float y, float w, float h, const D2D1_COLOR_F& c)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetColor(c);
    g_D2DTarget->FillRectangle(D2D1::RectF(x, y, x + w, y + h), g_Brush);
}

void DrawRect_(float x, float y, float w, float h, const D2D1_COLOR_F& c, float stroke)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetColor(c);
    g_D2DTarget->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), g_Brush, stroke);
}

void DrawLine_(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& c, float stroke)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetColor(c);
    g_D2DTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), g_Brush, stroke);
}

void DrawCircle_(float cx, float cy, float r, const D2D1_COLOR_F& c, float stroke)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetColor(c);
    g_D2DTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), g_Brush, stroke);
}

void FillCircle_(float cx, float cy, float r, const D2D1_COLOR_F& c)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetColor(c);
    g_D2DTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), g_Brush);
}

void DrawRoundedRect_(float x, float y, float w, float h, float r, const D2D1_COLOR_F& c)
{
    if (!g_D2DTarget || !g_Brush) return;
    SetColor(c);
    D2D1_ROUNDED_RECT rr = {D2D1::RectF(x, y, x + w, y + h), r, r};
    g_D2DTarget->FillRoundedRectangle(rr, g_Brush);
}

bool InRect(float x, float y, float rx, float ry, float rw, float rh)
{
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

// ============================================================================
// UI WIDGETS
// ============================================================================
bool Toggle(const wchar_t* label, float x, float y, bool* value)
{
    float w = 40, h = 20;
    D2D1_COLOR_F bg = *value ? Colors::Accent : D2D1_COLOR_F{0.3f, 0.3f, 0.3f, 1.0f};
    DrawRoundedRect_(x, y, w, h, h/2, bg);
    float knobX = *value ? x + w - h + 2 : x + 2;
    FillCircle_(knobX + (h-4)/2, y + h/2, (h-4)/2, Colors::White);
    RenderString(label, x + w + 10, y, Colors::White, true);
    
    if (g_MouseDown && InRect((float)g_MousePos.x, (float)g_MousePos.y, x, y, w + 150, h))
    {
        *value = !*value;
        g_MouseDown = false;
        return true;
    }
    return false;
}

bool Slider(const wchar_t* label, float x, float y, float* value, float minV, float maxV)
{
    float w = 150, h = 6;
    RenderString(label, x, y - 18, Colors::White, true);
    
    FillRect_(x, y + 7, w, h, Colors::Gray);
    float pct = (*value - minV) / (maxV - minV);
    FillRect_(x, y + 7, w * pct, h, Colors::Accent);
    FillCircle_(x + w * pct, y + 10, 8, Colors::White);
    
    wchar_t valStr[16];
    swprintf_s(valStr, L"%.1f", *value);
    RenderString(valStr, x + w + 10, y - 2, Colors::Gray, true);
    
    if (g_MouseDown && InRect((float)g_MousePos.x, (float)g_MousePos.y, x - 5, y, w + 10, 20))
    {
        float newPct = ((float)g_MousePos.x - x) / w;
        if (newPct < 0) newPct = 0;
        if (newPct > 1) newPct = 1;
        *value = minV + newPct * (maxV - minV);
        return true;
    }
    return false;
}

// ============================================================================
// RENDER ESP
// ============================================================================
void RenderESP()
{
    if (!g_Settings.espEnabled || !g_GameRunning) return;
    
    auto& players = PlayerManager::GetPlayers();
    float centerX = (float)g_MonitorWidth / 2;
    float centerY = (float)g_MonitorHeight / 2;
    
    for (auto& p : players)
    {
        if (p.origin.x == 0 && p.origin.y == 0) continue;
        if (!p.valid || !p.isAlive) continue;
        
        float angle = p.yaw * 3.14159f / 180.0f;
        float dist = p.distance;
        float screenX = centerX + cosf(angle) * (dist * 3.0f);
        float screenY = centerY + sinf(angle) * (dist * 3.0f);
        
        if (screenX < 0 || screenX > g_MonitorWidth || screenY < 0 || screenY > g_MonitorHeight)
            continue;
        
        float boxH = 80.0f - dist * 0.3f;
        if (boxH < 30) boxH = 30;
        float boxW = boxH * 0.4f;
        
        D2D1_COLOR_F color = p.isEnemy ? Colors::Red : Colors::Green;
        
        if (g_Settings.espBoxes)
            DrawRect_(screenX - boxW/2, screenY - boxH, boxW, boxH, color, 2.0f);
        
        if (g_Settings.espHealth)
        {
            float healthPct = (float)p.health / (float)p.maxHealth;
            float barH = boxH * healthPct;
            D2D1_COLOR_F hpColor = {1.0f - healthPct, healthPct, 0, 1.0f};
            FillRect_(screenX - boxW/2 - 6, screenY - barH, 4, barH, hpColor);
        }
        
        if (g_Settings.espNames)
        {
            wchar_t nameW[32];
            mbstowcs_s(nullptr, nameW, p.name, 31);
            RenderString(nameW, screenX - 30, screenY - boxH - 18, Colors::White, true);
        }
        
        if (g_Settings.espDistance)
        {
            wchar_t distStr[16];
            swprintf_s(distStr, L"%.0fm", p.distance);
            RenderString(distStr, screenX - 15, screenY + 5, Colors::Yellow, true);
        }
    }
}

// ============================================================================
// RENDER CROSSHAIR
// ============================================================================
void RenderCrosshair()
{
    if (!g_Settings.crosshair) return;
    
    float cx = (float)g_MonitorWidth / 2;
    float cy = (float)g_MonitorHeight / 2;
    
    DrawLine_(cx - 8, cy, cx + 8, cy, Colors::White, 2.0f);
    DrawLine_(cx, cy - 8, cx, cy + 8, Colors::White, 2.0f);
}

// ============================================================================
// RENDER RADAR
// ============================================================================
void RenderRadar()
{
    if (!g_Settings.radarEnabled || !g_GameRunning) return;
    
    float size = g_Settings.radarSize;
    float cx = 20 + size/2;
    float cy = 20 + size/2;
    
    FillCircle_(cx, cy, size/2, D2D1_COLOR_F{0.1f, 0.1f, 0.1f, 0.85f});
    DrawCircle_(cx, cy, size/2, Colors::Accent, 2.0f);
    
    DrawLine_(cx - size/2, cy, cx + size/2, cy, D2D1_COLOR_F{0.3f, 0.3f, 0.3f, 0.5f}, 1.0f);
    DrawLine_(cx, cy - size/2, cx, cy + size/2, D2D1_COLOR_F{0.3f, 0.3f, 0.3f, 0.5f}, 1.0f);
    
    FillCircle_(cx, cy, 4, Colors::Blue);
    
    auto& players = PlayerManager::GetPlayers();
    auto& local = PlayerManager::GetLocalPlayer();
    
    for (auto& p : players)
    {
        if (p.origin.x == 0 && p.origin.y == 0) continue;
        if (!p.valid || !p.isAlive) continue;
        
        Vec2 radarPos;
        WorldToRadar(p.origin, local.origin, local.yaw, radarPos, cx, cy, size);
        
        float dx = radarPos.x - cx;
        float dy = radarPos.y - cy;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > size/2 - 5)
        {
            float scale = (size/2 - 5) / dist;
            radarPos.x = cx + dx * scale;
            radarPos.y = cy + dy * scale;
        }
        
        D2D1_COLOR_F dotColor = p.isEnemy ? Colors::Red : Colors::Green;
        FillCircle_(radarPos.x, radarPos.y, 4, dotColor);
    }
    
    RenderString(L"RADAR", cx - 20, cy + size/2 + 5, Colors::White, true);
}

// ============================================================================
// RENDER MENU - Shows REAL hardware status
// ============================================================================
void RenderMenu()
{
    if (!g_MenuVisible) return;
    
    float menuX = (float)g_MonitorWidth / 2 - 200;
    float menuY = (float)g_MonitorHeight / 2 - 200;
    float menuW = 400;
    float menuH = 400;
    
    DrawRoundedRect_(menuX, menuY, menuW, menuH, 10, Colors::MenuBG);
    FillRect_(menuX, menuY, menuW, 40, Colors::Accent);
    RenderString(L"PROJECT ZERO | v4.1", menuX + 10, menuY + 8, Colors::White, false);
    
    // Status indicator - REAL STATUS
    bool dmaOnline = DMAEngine::IsConnected();
    D2D1_COLOR_F statusColor = dmaOnline ? Colors::Green : Colors::Red;
    FillCircle_(menuX + menuW - 25, menuY + 20, 8, statusColor);
    
    // Tabs
    float tabY = menuY + 50;
    const wchar_t* tabs[] = {L"ESP", L"RADAR", L"AIMBOT", L"STATUS"};
    for (int i = 0; i < 4; i++)
    {
        float tabX = menuX + 10 + i * 95;
        D2D1_COLOR_F tabColor = (i == g_CurrentTab) ? Colors::Accent : Colors::DarkGray;
        DrawRoundedRect_(tabX, tabY, 90, 30, 5, tabColor);
        RenderString(tabs[i], tabX + 20, tabY + 5, Colors::White, true);
        
        if (g_MouseDown && InRect((float)g_MousePos.x, (float)g_MousePos.y, tabX, tabY, 90, 30))
        {
            g_CurrentTab = i;
            g_MouseDown = false;
        }
    }
    
    float contentY = tabY + 45;
    float contentX = menuX + 20;
    
    switch (g_CurrentTab)
    {
    case 0: // ESP
        Toggle(L"Enable ESP", contentX, contentY, &g_Settings.espEnabled);
        Toggle(L"Boxes", contentX, contentY + 35, &g_Settings.espBoxes);
        Toggle(L"Health", contentX, contentY + 70, &g_Settings.espHealth);
        Toggle(L"Names", contentX, contentY + 105, &g_Settings.espNames);
        Toggle(L"Distance", contentX, contentY + 140, &g_Settings.espDistance);
        break;
        
    case 1: // RADAR
        Toggle(L"Enable Radar", contentX, contentY, &g_Settings.radarEnabled);
        Slider(L"Size", contentX, contentY + 55, &g_Settings.radarSize, 100, 300);
        Slider(L"Zoom", contentX, contentY + 110, &g_Settings.radarZoom, 0.5f, 3.0f);
        break;
        
    case 2: // AIMBOT
        Toggle(L"Enable Aimbot", contentX, contentY, &g_Settings.aimbotEnabled);
        Toggle(L"FOV Circle", contentX, contentY + 35, &g_Settings.aimbotFOVCircle);
        Slider(L"FOV", contentX, contentY + 90, &g_Settings.aimbotFOV, 5, 100);
        Slider(L"Smooth", contentX, contentY + 145, &g_Settings.aimbotSmooth, 1, 20);
        
        // REAL KMBox Status
        {
            bool kmConnected = IsKMBoxConnected();
            wchar_t kmStatus[64];
            swprintf_s(kmStatus, L"KMBox: %S", GetKMBoxStatus());
            RenderString(kmStatus, contentX, contentY + 200, kmConnected ? Colors::Green : Colors::Red, true);
            
            if (kmConnected)
            {
                wchar_t portInfo[32];
                swprintf_s(portInfo, L"Port: %S", HardwareController::GetLockedPort());
                RenderString(portInfo, contentX, contentY + 225, Colors::Gray, true);
            }
        }
        break;
        
    case 3: // STATUS - REAL HARDWARE STATUS
        {
            // DMA Status - REAL
            bool dmaOK = DMAEngine::IsConnected();
            wchar_t dmaStatus[64];
            swprintf_s(dmaStatus, L"DMA: %S", DMAEngine::GetStatus());
            RenderString(dmaStatus, contentX, contentY, dmaOK ? Colors::Green : Colors::Red, true);
            
            // KMBox Status - REAL
            bool kmOK = IsKMBoxConnected();
            wchar_t kmStatus[64];
            swprintf_s(kmStatus, L"KMBox: %S", GetKMBoxStatus());
            RenderString(kmStatus, contentX, contentY + 30, kmOK ? Colors::Green : Colors::Red, true);
            
            // Resolution
            wchar_t resInfo[64];
            swprintf_s(resInfo, L"Resolution: %dx%d", g_MonitorWidth, g_MonitorHeight);
            RenderString(resInfo, contentX, contentY + 60, Colors::White, true);
            
            // Offsets
            wchar_t offsetStatus[64];
            swprintf_s(offsetStatus, L"Offsets: %s", OffsetUpdater::s_Synced ? L"CLOUD" : L"LOCAL");
            RenderString(offsetStatus, contentX, contentY + 90, OffsetUpdater::s_Synced ? Colors::Green : Colors::Yellow, true);
            
            RenderString(L"---", contentX, contentY + 130, Colors::Gray, true);
            RenderString(L"INSERT - Toggle Menu", contentX, contentY + 155, Colors::Gray, true);
            RenderString(L"END - Exit", contentX, contentY + 180, Colors::Gray, true);
        }
        break;
    }
}

// ============================================================================
// AIMBOT FOV CIRCLE
// ============================================================================
void RenderAimbotFOV()
{
    if (!g_Settings.aimbotEnabled || !g_Settings.aimbotFOVCircle) return;
    
    float cx = (float)g_MonitorWidth / 2;
    float cy = (float)g_MonitorHeight / 2;
    DrawCircle_(cx, cy, g_Settings.aimbotFOV * 5, D2D1_COLOR_F{1.0f, 1.0f, 1.0f, 0.3f}, 1.0f);
}

// ============================================================================
// CREATE THE BLACK WINDOW (Professional Method)
// ============================================================================
bool CreateBlackWindow()
{
    // STEP 1: DETECT MONITOR RESOLUTION
    g_MonitorWidth = GetSystemMetrics(SM_CXSCREEN);
    g_MonitorHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // STEP 2: Register window class with BLACK background
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = BlackWindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);  // SOLID BLACK BACKGROUND
    wc.lpszClassName = L"ZeroEliteBlackWindow";
    RegisterClassExW(&wc);
    
    // STEP 3: Create SOLID BLACK HWND (NOT transparent from start)
    g_BlackWindow = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        wc.lpszClassName, 
        L"",
        WS_POPUP,
        0, 0, g_MonitorWidth, g_MonitorHeight,  // RESOLUTION SYNC
        nullptr, nullptr, wc.hInstance, nullptr);
    
    if (!g_BlackWindow) return false;
    
    // STEP 4: COLOR KEYING - Make ONLY black transparent
    // This is the PROFESSIONAL way - draw on a "stage"
    SetLayeredWindowAttributes(g_BlackWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // STEP 5: Extend frame for proper composition
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(g_BlackWindow, &margins);
    
    ShowWindow(g_BlackWindow, SW_SHOWDEFAULT);
    UpdateWindow(g_BlackWindow);
    
    return true;
}

// ============================================================================
// INIT GRAPHICS
// ============================================================================
bool InitGraphics()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = g_MonitorWidth;
    scd.BufferDesc.Height = g_MonitorHeight;
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = g_BlackWindow;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    
    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        nullptr, 0, D3D11_SDK_VERSION, &scd,
        &g_SwapChain, &g_Device, nullptr, &g_Context)))
        return false;
    
    ID3D11Texture2D* backBuffer = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
    backBuffer->Release();
    
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_D2DFactory);
    
    IDXGISurface* surface = nullptr;
    g_SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);
    
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    g_D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &g_D2DTarget);
    surface->Release();
    
    g_D2DTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &g_Brush);
    
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_DWriteFactory);
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &g_TextFormat);
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &g_SmallTextFormat);
    
    return true;
}

// ============================================================================
// CLEANUP
// ============================================================================
void Cleanup()
{
    if (g_SmallTextFormat) g_SmallTextFormat->Release();
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
// MAIN - STRICT EXECUTION ORDER
// ============================================================================
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // ===============================================
    // STEP 1: REAL HARDWARE CHECK (Console)
    // ===============================================
    if (!DMAEngine::Initialize())
    {
        MessageBoxW(nullptr, L"Hardware validation failed!", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // ===============================================
    // STEP 2: CREATE THE BLACK WINDOW
    // ===============================================
    if (!CreateBlackWindow())
    {
        MessageBoxW(nullptr, L"Failed to create black window!", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // ===============================================
    // STEP 3: INIT GRAPHICS
    // ===============================================
    if (!InitGraphics())
    {
        MessageBoxW(nullptr, L"Failed to initialize graphics!", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // ===============================================
    // STEP 4: INITIALIZE SYSTEMS
    // ===============================================
    PlayerManager::Initialize();
    
    // ===============================================
    // STEP 5: START BACKGROUND THREADS
    // ===============================================
    std::thread inputThread(InputThread);
    std::thread updateThread(UpdateThread);
    
    g_MenuVisible = false;
    
    // ===============================================
    // MAIN RENDER LOOP
    // ===============================================
    MSG msg = {};
    while (g_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                g_Running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        if (!g_Running) break;
        
        // Clear to BLACK (this becomes transparent via color keying)
        float clearColor[4] = {0, 0, 0, 1};  // Solid black
        g_Context->ClearRenderTargetView(g_RenderTarget, clearColor);
        
        // Begin D2D drawing
        g_D2DTarget->BeginDraw();
        g_D2DTarget->Clear(Colors::Black);  // Black background (transparent via LWA_COLORKEY)
        
        // Draw only when needed
        if (g_GameRunning || g_MenuVisible)
        {
            RenderESP();
            RenderCrosshair();
            RenderRadar();
            RenderAimbotFOV();
        }
        
        RenderMenu();
        
        g_D2DTarget->EndDraw();
        
        // Present (V-Sync off)
        g_SwapChain->Present(0, 0);
        
        Sleep(1);
    }
    
    // Cleanup
    g_Running = false;
    inputThread.join();
    updateThread.join();
    Cleanup();
    
    return 0;
}
