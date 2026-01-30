// PROJECT ZERO - Direct2D/DirectX Based Menu
// No ImGui dependency - draws directly with D2D

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// ============================================================================
// SETTINGS
// ============================================================================
struct Settings {
    // Aimbot
    bool aimbotEnabled = false;
    float aimbotFOV = 120.0f;
    float aimbotSmooth = 7.5f;
    int aimbotBone = 0;  // 0=Head, 1=Neck, 2=Chest
    bool aimbotVisCheck = true;
    
    // ESP
    bool espEnabled = true;
    bool espBox = true;
    bool espSkeleton = true;
    bool espHealth = true;
    bool espName = true;
    bool espDistance = true;
    int espBoxType = 1;  // 0=2D, 1=Corner
    
    // Misc
    bool triggerbot = false;
    bool noFlash = true;
    bool noSmoke = true;
    bool bhop = true;
    bool magicBullet = true;
    
    // Radar
    float radarSize = 200.0f;
    float radarZoom = 1.5f;
};

static Settings g_Settings;

// ============================================================================
// PLAYER DATA
// ============================================================================
struct Player {
    bool valid;
    bool isEnemy;
    char name[32];
    int health;
    float distance;
    float x, y;  // Radar position
    float yaw;
};

static std::vector<Player> g_Players;
static float g_LocalYaw = 0.0f;

// ============================================================================
// DIRECTX GLOBALS
// ============================================================================
static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D11RenderTargetView* g_RenderTarget = nullptr;

// Direct2D
static ID2D1Factory* g_D2DFactory = nullptr;
static ID2D1RenderTarget* g_D2DRenderTarget = nullptr;
static ID2D1SolidColorBrush* g_Brush = nullptr;
static IDWriteFactory* g_DWriteFactory = nullptr;
static IDWriteTextFormat* g_TextFormat = nullptr;
static IDWriteTextFormat* g_TitleFormat = nullptr;

// Window
static HWND g_Hwnd = nullptr;
static bool g_Running = true;
static int g_Width = 1280;
static int g_Height = 720;

// Menu
static int g_CurrentTab = 0;
static int g_SelectedItem = 0;
static bool g_MenuOpen = true;

// ============================================================================
// INITIALIZATION
// ============================================================================
bool InitD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = g_Width;
    sd.BufferDesc.Height = g_Height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_SwapChain, &g_Device, nullptr, &g_Context)))
        return false;

    ID3D11Texture2D* backBuffer;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
    backBuffer->Release();

    return true;
}

bool InitD2D()
{
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_D2DFactory)))
        return false;

    IDXGISurface* surface;
    g_SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

    g_D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &g_D2DRenderTarget);
    surface->Release();

    g_D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &g_Brush);

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_DWriteFactory);
    
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &g_TextFormat);
    
    g_DWriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"en-us", &g_TitleFormat);

    return true;
}

void InitPlayers()
{
    g_Players.clear();
    for (int i = 0; i < 12; i++)
    {
        Player p = {};
        p.valid = true;
        p.isEnemy = (i < 6);
        p.health = 30 + rand() % 70;
        p.distance = 10.0f + (float)(rand() % 200);
        sprintf_s(p.name, "%s_%02d", p.isEnemy ? "Enemy" : "Team", i + 1);
        
        float angle = (float)i * (6.28318f / 12.0f);
        p.x = cosf(angle) * (50.0f + rand() % 50);
        p.y = sinf(angle) * (50.0f + rand() % 50);
        p.yaw = (float)(rand() % 360);
        
        g_Players.push_back(p);
    }
}

void Cleanup()
{
    if (g_TextFormat) g_TextFormat->Release();
    if (g_TitleFormat) g_TitleFormat->Release();
    if (g_DWriteFactory) g_DWriteFactory->Release();
    if (g_Brush) g_Brush->Release();
    if (g_D2DRenderTarget) g_D2DRenderTarget->Release();
    if (g_D2DFactory) g_D2DFactory->Release();
    if (g_RenderTarget) g_RenderTarget->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_Context) g_Context->Release();
    if (g_Device) g_Device->Release();
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================
void DrawText(const wchar_t* text, float x, float y, D2D1_COLOR_F color, bool title = false)
{
    g_Brush->SetColor(color);
    D2D1_RECT_F rect = { x, y, x + 500, y + 30 };
    g_D2DRenderTarget->DrawText(text, (UINT32)wcslen(text), title ? g_TitleFormat : g_TextFormat, rect, g_Brush);
}

void DrawRect(float x, float y, float w, float h, D2D1_COLOR_F color, bool filled = false)
{
    g_Brush->SetColor(color);
    D2D1_RECT_F rect = { x, y, x + w, y + h };
    if (filled)
        g_D2DRenderTarget->FillRectangle(rect, g_Brush);
    else
        g_D2DRenderTarget->DrawRectangle(rect, g_Brush, 2.0f);
}

void DrawRoundedRect(float x, float y, float w, float h, D2D1_COLOR_F color, float radius, bool filled = false)
{
    g_Brush->SetColor(color);
    D2D1_ROUNDED_RECT rr = { {x, y, x + w, y + h}, radius, radius };
    if (filled)
        g_D2DRenderTarget->FillRoundedRectangle(rr, g_Brush);
    else
        g_D2DRenderTarget->DrawRoundedRectangle(rr, g_Brush, 2.0f);
}

void DrawCircle(float cx, float cy, float r, D2D1_COLOR_F color, bool filled = false)
{
    g_Brush->SetColor(color);
    D2D1_ELLIPSE ellipse = { {cx, cy}, r, r };
    if (filled)
        g_D2DRenderTarget->FillEllipse(ellipse, g_Brush);
    else
        g_D2DRenderTarget->DrawEllipse(ellipse, g_Brush, 2.0f);
}

void DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F color, float width = 1.0f)
{
    g_Brush->SetColor(color);
    g_D2DRenderTarget->DrawLine({ x1, y1 }, { x2, y2 }, g_Brush, width);
}

// ============================================================================
// MENU RENDERING
// ============================================================================
void RenderMenu()
{
    if (!g_MenuOpen) return;
    
    const float menuX = 50.0f;
    const float menuY = 50.0f;
    const float menuW = 500.0f;
    const float menuH = 550.0f;
    
    D2D1_COLOR_F bgColor = { 0.08f, 0.08f, 0.08f, 0.95f };
    D2D1_COLOR_F borderColor = { 0.8f, 0.2f, 0.2f, 1.0f };
    D2D1_COLOR_F titleBgColor = { 0.6f, 0.1f, 0.1f, 1.0f };
    D2D1_COLOR_F textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    D2D1_COLOR_F redColor = { 0.9f, 0.2f, 0.2f, 1.0f };
    D2D1_COLOR_F greenColor = { 0.2f, 0.9f, 0.2f, 1.0f };
    D2D1_COLOR_F grayColor = { 0.5f, 0.5f, 0.5f, 1.0f };
    D2D1_COLOR_F darkColor = { 0.15f, 0.15f, 0.15f, 1.0f };
    
    // Background
    DrawRoundedRect(menuX, menuY, menuW, menuH, bgColor, 10.0f, true);
    DrawRoundedRect(menuX, menuY, menuW, menuH, borderColor, 10.0f, false);
    
    // Title bar
    DrawRoundedRect(menuX + 2, menuY + 2, menuW - 4, 40, titleBgColor, 8.0f, true);
    DrawText(L"PROJECT ZERO | BO6 DMA", menuX + 15, menuY + 10, textColor, true);
    
    // Status
    DrawText(L"STATUS:", menuX + 350, menuY + 12, textColor, false);
    DrawText(L"ONLINE", menuX + 410, menuY + 12, greenColor, false);
    
    // Tabs
    const wchar_t* tabs[] = { L"AIMBOT", L"ESP", L"RADAR", L"MISC" };
    float tabX = menuX + 10;
    float tabY = menuY + 50;
    float tabW = 115;
    float tabH = 30;
    
    for (int i = 0; i < 4; i++)
    {
        D2D1_COLOR_F tabColor = (g_CurrentTab == i) ? redColor : darkColor;
        DrawRoundedRect(tabX + i * (tabW + 5), tabY, tabW, tabH, tabColor, 5.0f, true);
        DrawText(tabs[i], tabX + i * (tabW + 5) + 25, tabY + 6, textColor, false);
    }
    
    // Content area
    float contentY = tabY + 45;
    float lineHeight = 28.0f;
    int line = 0;
    
    // ==================== AIMBOT TAB ====================
    if (g_CurrentTab == 0)
    {
        DrawText(L"[  ] Enable Aimbot", menuX + 20, contentY + line * lineHeight, textColor); line++;
        if (g_Settings.aimbotEnabled)
            DrawText(L"X", menuX + 22, contentY, greenColor);
        
        wchar_t buf[64];
        swprintf_s(buf, L"FOV: %.0f", g_Settings.aimbotFOV);
        DrawText(buf, menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, 200, 12, darkColor, true);
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, g_Settings.aimbotFOV / 180.0f * 200, 12, redColor, true);
        
        swprintf_s(buf, L"Smoothness: %.1f", g_Settings.aimbotSmooth);
        DrawText(buf, menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, 200, 12, darkColor, true);
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, g_Settings.aimbotSmooth / 20.0f * 200, 12, redColor, true);
        
        const wchar_t* bones[] = { L"Head", L"Neck", L"Chest" };
        swprintf_s(buf, L"Target Bone: %s", bones[g_Settings.aimbotBone]);
        DrawText(buf, menuX + 20, contentY + line * lineHeight, textColor); line++;
        
        line++;
        DrawText(L"[  ] Visibility Check", menuX + 20, contentY + line * lineHeight, textColor); line++;
        if (g_Settings.aimbotVisCheck)
            DrawText(L"X", menuX + 22, contentY + (line-1) * lineHeight, greenColor);
        
        line++;
        DrawText(L"--- Quick Toggles ---", menuX + 20, contentY + line * lineHeight, grayColor); line++;
        
        DrawText(L"[  ] Triggerbot", menuX + 20, contentY + line * lineHeight, textColor); 
        if (g_Settings.triggerbot) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        DrawText(L"[  ] Magic Bullet", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.magicBullet) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        DrawText(L"[  ] Bhop", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.bhop) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
    }
    
    // ==================== ESP TAB ====================
    else if (g_CurrentTab == 1)
    {
        DrawText(L"[  ] Enable ESP", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.espEnabled) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        line++;
        DrawText(L"[  ] Box ESP", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.espBox) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        DrawText(L"[  ] Skeleton ESP", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.espSkeleton) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        DrawText(L"[  ] Health Bars", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.espHealth) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        DrawText(L"[  ] Name Tags", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.espName) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        DrawText(L"[  ] Distance", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.espDistance) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        line++;
        const wchar_t* boxTypes[] = { L"2D Box", L"Corner Box" };
        wchar_t buf[64];
        swprintf_s(buf, L"Box Type: %s", boxTypes[g_Settings.espBoxType]);
        DrawText(buf, menuX + 20, contentY + line * lineHeight, textColor);
        line++;
    }
    
    // ==================== RADAR TAB ====================
    else if (g_CurrentTab == 2)
    {
        wchar_t buf[64];
        swprintf_s(buf, L"Radar Size: %.0f", g_Settings.radarSize);
        DrawText(buf, menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, 200, 12, darkColor, true);
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, (g_Settings.radarSize - 100) / 300.0f * 200, 12, redColor, true);
        
        swprintf_s(buf, L"Radar Zoom: %.1fx", g_Settings.radarZoom);
        DrawText(buf, menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, 200, 12, darkColor, true);
        DrawRect(menuX + 120, contentY + (line-1) * lineHeight + 5, g_Settings.radarZoom / 5.0f * 200, 12, redColor, true);
        
        line++;
        swprintf_s(buf, L"Players Detected: %d", (int)g_Players.size());
        DrawText(buf, menuX + 20, contentY + line * lineHeight, textColor); line++;
        
        line++;
        DrawText(L"--- Player List ---", menuX + 20, contentY + line * lineHeight, grayColor); line++;
        
        for (size_t i = 0; i < g_Players.size() && i < 8; i++)
        {
            const Player& p = g_Players[i];
            D2D1_COLOR_F col = p.isEnemy ? redColor : D2D1::ColorF(0.3f, 0.6f, 1.0f);
            wchar_t pname[64];
            swprintf_s(pname, L"%S - HP:%d - %.0fm", p.name, p.health, p.distance);
            DrawText(pname, menuX + 20, contentY + line * lineHeight, col); line++;
        }
    }
    
    // ==================== MISC TAB ====================
    else if (g_CurrentTab == 3)
    {
        DrawText(L"[  ] No Flash", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.noFlash) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        DrawText(L"[  ] No Smoke", menuX + 20, contentY + line * lineHeight, textColor);
        if (g_Settings.noSmoke) DrawText(L"X", menuX + 22, contentY + line * lineHeight, greenColor);
        line++;
        
        line++;
        DrawText(L"--- Controls ---", menuX + 20, contentY + line * lineHeight, grayColor); line++;
        DrawText(L"TAB / Arrow Keys: Navigate tabs", menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawText(L"UP/DOWN: Select option", menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawText(L"ENTER/SPACE: Toggle option", menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawText(L"INSERT: Toggle menu", menuX + 20, contentY + line * lineHeight, textColor); line++;
        DrawText(L"END: Exit", menuX + 20, contentY + line * lineHeight, textColor); line++;
        
        line++;
        DrawText(L"PROJECT ZERO v2.0", menuX + 20, contentY + line * lineHeight, redColor); line++;
        DrawText(L"DMA Radar for Black Ops 6", menuX + 20, contentY + line * lineHeight, grayColor); line++;
    }
}

// ============================================================================
// RADAR RENDERING
// ============================================================================
void RenderRadar()
{
    static float animTime = 0.0f;
    animTime += 0.016f;
    
    // Update player positions (simulation)
    for (size_t i = 0; i < g_Players.size(); i++)
    {
        Player& p = g_Players[i];
        float angle = animTime * 0.3f + (float)i * 0.5f;
        float dist = 50.0f + sinf(animTime + i) * 20.0f;
        p.x = cosf(angle) * dist;
        p.y = sinf(angle) * dist;
    }
    g_LocalYaw = fmodf(animTime * 20.0f, 360.0f);
    
    float radarX = (float)g_Width - g_Settings.radarSize - 30;
    float radarY = 30.0f;
    float size = g_Settings.radarSize;
    float cx = radarX + size / 2;
    float cy = radarY + size / 2;
    
    D2D1_COLOR_F bgColor = { 0.05f, 0.05f, 0.08f, 0.95f };
    D2D1_COLOR_F borderColor = { 0.8f, 0.2f, 0.2f, 1.0f };
    D2D1_COLOR_F gridColor = { 0.2f, 0.2f, 0.25f, 1.0f };
    D2D1_COLOR_F greenColor = { 0.2f, 1.0f, 0.2f, 1.0f };
    D2D1_COLOR_F redColor = { 1.0f, 0.2f, 0.2f, 1.0f };
    D2D1_COLOR_F blueColor = { 0.2f, 0.5f, 1.0f, 1.0f };
    D2D1_COLOR_F textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    // Background
    DrawRoundedRect(radarX, radarY, size, size, bgColor, 8.0f, true);
    DrawRoundedRect(radarX, radarY, size, size, borderColor, 8.0f, false);
    
    // Grid
    DrawLine(cx, radarY + 5, cx, radarY + size - 5, gridColor);
    DrawLine(radarX + 5, cy, radarX + size - 5, cy, gridColor);
    for (int i = 1; i <= 3; i++)
    {
        float r = (size * 0.45f) * (i / 3.0f);
        DrawCircle(cx, cy, r, gridColor);
    }
    
    // Players
    float zoom = g_Settings.radarZoom;
    for (const Player& p : g_Players)
    {
        if (!p.valid) continue;
        
        float dx = p.x;
        float dy = p.y;
        
        // Rotate by local yaw
        float rad = g_LocalYaw * 3.14159f / 180.0f;
        float rx = dx * cosf(rad) - dy * sinf(rad);
        float ry = dx * sinf(rad) + dy * cosf(rad);
        
        // Scale
        float scale = (size * 0.4f) / (100.0f / zoom);
        rx *= scale;
        ry *= scale;
        
        // Clamp
        float maxR = size * 0.4f;
        float len = sqrtf(rx * rx + ry * ry);
        if (len > maxR) { rx = rx / len * maxR; ry = ry / len * maxR; }
        
        float px = cx + rx;
        float py = cy - ry;
        
        D2D1_COLOR_F col = p.isEnemy ? redColor : blueColor;
        DrawCircle(px, py, 5.0f, col, true);
        
        // Direction
        float yawRad = p.yaw * 3.14159f / 180.0f;
        DrawLine(px, py, px + sinf(yawRad) * 10, py - cosf(yawRad) * 10, col, 2.0f);
    }
    
    // Local player
    DrawCircle(cx, cy, 6.0f, greenColor, true);
    float viewRad = g_LocalYaw * 3.14159f / 180.0f;
    DrawLine(cx, cy, cx + sinf(viewRad) * 15, cy - cosf(viewRad) * 15, greenColor, 2.0f);
    
    // Title
    DrawText(L"RADAR", radarX + 10, radarY + size + 5, textColor);
    
    wchar_t buf[32];
    swprintf_s(buf, L"%d players", (int)g_Players.size());
    DrawText(buf, radarX + size - 80, radarY + size + 5, gridColor);
}

// ============================================================================
// WINDOW PROCEDURE
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_INSERT) g_MenuOpen = !g_MenuOpen;
        else if (wParam == VK_END) g_Running = false;
        else if (wParam == VK_TAB || wParam == VK_RIGHT) g_CurrentTab = (g_CurrentTab + 1) % 4;
        else if (wParam == VK_LEFT) g_CurrentTab = (g_CurrentTab + 3) % 4;
        else if (wParam == VK_RETURN || wParam == VK_SPACE)
        {
            // Toggle current option based on tab and selection
            if (g_CurrentTab == 0) g_Settings.aimbotEnabled = !g_Settings.aimbotEnabled;
            else if (g_CurrentTab == 1) g_Settings.espEnabled = !g_Settings.espEnabled;
        }
        else if (wParam == VK_UP)
        {
            if (g_CurrentTab == 0 && g_Settings.aimbotFOV < 180) g_Settings.aimbotFOV += 5;
            if (g_CurrentTab == 2 && g_Settings.radarSize < 400) g_Settings.radarSize += 10;
        }
        else if (wParam == VK_DOWN)
        {
            if (g_CurrentTab == 0 && g_Settings.aimbotFOV > 5) g_Settings.aimbotFOV -= 5;
            if (g_CurrentTab == 2 && g_Settings.radarSize > 100) g_Settings.radarSize -= 10;
        }
        return 0;
        
    case WM_DESTROY:
        g_Running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================================
// MAIN
// ============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    // Register window class
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0, 0, hInstance, 
                       nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, 
                       L"ProjectZeroClass", nullptr };
    RegisterClassExW(&wc);

    // Create window
    g_Hwnd = CreateWindowW(wc.lpszClassName, L"PROJECT ZERO | BO6",
        WS_OVERLAPPEDWINDOW, 100, 100, g_Width, g_Height,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_Hwnd) return 1;

    // Initialize
    if (!InitD3D(g_Hwnd)) return 1;
    if (!InitD2D()) return 1;
    InitPlayers();

    ShowWindow(g_Hwnd, SW_SHOW);
    UpdateWindow(g_Hwnd);

    // Main loop
    MSG msg = {};
    while (g_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) g_Running = false;
        }
        if (!g_Running) break;

        // Render
        float clearColor[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
        g_Context->ClearRenderTargetView(g_RenderTarget, clearColor);

        g_D2DRenderTarget->BeginDraw();
        
        RenderRadar();
        RenderMenu();
        
        g_D2DRenderTarget->EndDraw();
        
        g_SwapChain->Present(1, 0);
    }

    Cleanup();
    DestroyWindow(g_Hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);
    
    return 0;
}

int main() { return wWinMain(GetModuleHandle(nullptr), nullptr, nullptr, 0); }
