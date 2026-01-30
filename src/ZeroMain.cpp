// PROJECT ZERO - Professional DMA Overlay
// Features: Transparent Overlay, Mouse Control, Scatter Reads, Pattern Scan

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
#include <ctime>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

// ============================================================================
// SETTINGS STRUCTURE
// ============================================================================
struct Settings {
    // Aimbot
    bool aimbotEnabled = false;
    float aimbotFOV = 120.0f;
    float aimbotSmooth = 7.5f;
    int aimbotBone = 0;
    bool aimbotVisCheck = true;
    float aimbotMaxDist = 150.0f;
    int aimbotKey = 1;  // 0=LMB, 1=RMB, 2=Shift
    int aimbotPriority = 0;  // 0=Crosshair, 1=Distance, 2=Health
    
    // ESP
    bool espEnabled = true;
    bool espBox = true;
    bool espSkeleton = true;
    bool espHealth = true;
    bool espName = true;
    bool espDistance = true;
    bool espSnapline = false;
    int espBoxType = 1;
    
    // Misc
    bool triggerbot = false;
    bool noFlash = true;
    bool noSmoke = true;
    bool bhop = true;
    bool magicBullet = true;
    bool radarHack = true;
    
    // Radar
    bool radarEnabled = true;
    float radarSize = 220.0f;
    float radarZoom = 1.5f;
    float radarX = 20.0f;
    float radarY = 20.0f;
    bool radarShowTeam = true;
    bool radarShowEnemy = true;
};

// ============================================================================
// PLAYER DATA
// ============================================================================
struct Player {
    bool valid = false;
    bool isEnemy = true;
    bool isVisible = false;
    char name[32] = {0};
    int health = 100;
    float distance = 0.0f;
    float worldX = 0.0f, worldY = 0.0f, worldZ = 0.0f;
    float yaw = 0.0f;
};

// ============================================================================
// GLOBALS
// ============================================================================
static Settings g_Settings;
static std::vector<Player> g_Players;
static std::mutex g_PlayerMutex;
static float g_LocalYaw = 0.0f;

// Window & Graphics
static HWND g_Hwnd = nullptr;
static HWND g_GameHwnd = nullptr;
static int g_Width = 0;
static int g_Height = 0;

// D3D11
static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D11RenderTargetView* g_RenderTarget = nullptr;

// D2D
static ID2D1Factory* g_D2DFactory = nullptr;
static ID2D1RenderTarget* g_D2DTarget = nullptr;
static ID2D1SolidColorBrush* g_Brush = nullptr;
static IDWriteFactory* g_WriteFactory = nullptr;
static IDWriteTextFormat* g_Font = nullptr;
static IDWriteTextFormat* g_FontBold = nullptr;
static IDWriteTextFormat* g_FontSmall = nullptr;

// State
static std::atomic<bool> g_Running(true);
static std::atomic<bool> g_MenuVisible(true);
static std::atomic<bool> g_Clickable(true);
static int g_CurrentTab = 0;
static int g_HoveredItem = -1;
static int g_SelectedSlider = -1;
static POINT g_MousePos = {0, 0};
static bool g_MouseDown = false;
static bool g_MouseClicked = false;

// ============================================================================
// INPUT THREAD (Non-blocking)
// ============================================================================
static std::atomic<bool> g_InsertPressed(false);
static std::atomic<bool> g_InsertWasDown(false);

void InputThread()
{
    while (g_Running)
    {
        bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        
        if (insertDown && !g_InsertWasDown)
        {
            g_InsertPressed = true;
        }
        g_InsertWasDown = insertDown;
        
        // END key to exit
        if (GetAsyncKeyState(VK_END) & 0x8000)
        {
            g_Running = false;
        }
        
        Sleep(10);
    }
}

// ============================================================================
// DMA SCATTER READ SIMULATION
// ============================================================================
struct ScatterRequest {
    uintptr_t address;
    void* buffer;
    size_t size;
};

class DMAScatterRead {
public:
    std::vector<ScatterRequest> requests;
    
    void AddRead(uintptr_t addr, void* buf, size_t size) {
        requests.push_back({addr, buf, size});
    }
    
    void Execute() {
        // In real DMA, this would batch all reads together
        // For simulation, we just fill with fake data
        for (auto& req : requests) {
            memset(req.buffer, 0, req.size);
        }
        requests.clear();
    }
};

static DMAScatterRead g_Scatter;

// ============================================================================
// PATTERN SCANNER (Auto-Offset Updater)
// ============================================================================
class PatternScanner {
public:
    static uintptr_t FindPattern(const char* module, const char* pattern, const char* mask) {
        // In real implementation, this would scan game memory
        // Returns 0 for simulation
        (void)module; (void)pattern; (void)mask;
        return 0;
    }
    
    static bool UpdateOffsets() {
        // Patterns for BO6 (example - would need real patterns)
        // uintptr_t clientInfo = FindPattern("cod.exe", "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0", "xxx????xxx");
        // These would be used to dynamically find game structures
        return true;
    }
};

// ============================================================================
// PLAYER UPDATE THREAD
// ============================================================================
void UpdateThread()
{
    srand((unsigned)time(nullptr));
    
    // Initialize players
    {
        std::lock_guard<std::mutex> lock(g_PlayerMutex);
        g_Players.clear();
        for (int i = 0; i < 12; i++)
        {
            Player p = {};
            p.valid = true;
            p.isEnemy = (i < 6);
            p.health = 30 + rand() % 70;
            p.distance = 10.0f + (float)(rand() % 200);
            p.isVisible = (rand() % 2) == 0;
            sprintf_s(p.name, "%s_%02d", p.isEnemy ? "Enemy" : "Team", i + 1);
            
            float angle = (float)i * (6.28318f / 12.0f);
            p.worldX = cosf(angle) * (50.0f + rand() % 100);
            p.worldY = sinf(angle) * (50.0f + rand() % 100);
            p.yaw = (float)(rand() % 360);
            
            g_Players.push_back(p);
        }
    }
    
    float time = 0.0f;
    
    while (g_Running)
    {
        time += 0.016f;
        
        // Simulate scatter read for all players
        g_Scatter.requests.clear();
        for (size_t i = 0; i < g_Players.size(); i++)
        {
            // In real DMA, add read requests for each player's data
            g_Scatter.AddRead(0x1000 + i * 0x100, nullptr, sizeof(Player));
        }
        g_Scatter.Execute();
        
        // Update positions (simulation)
        {
            std::lock_guard<std::mutex> lock(g_PlayerMutex);
            for (size_t i = 0; i < g_Players.size(); i++)
            {
                Player& p = g_Players[i];
                float angle = time * 0.3f + (float)i * 0.5f;
                float dist = 50.0f + sinf(time + i) * 30.0f;
                p.worldX = cosf(angle) * dist;
                p.worldY = sinf(angle) * dist;
                p.distance = dist;
                p.yaw = fmodf(angle * 57.3f, 360.0f);
                p.health = 30 + (int)(sinf(time * 0.5f + i) * 35 + 35);
            }
            g_LocalYaw = fmodf(time * 20.0f, 360.0f);
        }
        
        Sleep(16);  // ~60 FPS update
    }
}

// ============================================================================
// WINDOW SETUP
// ============================================================================
void SetClickThrough(bool clickThrough)
{
    LONG_PTR style = GetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE);
    if (clickThrough)
        style |= WS_EX_TRANSPARENT;
    else
        style &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(g_Hwnd, GWL_EXSTYLE, style);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEMOVE:
        g_MousePos.x = LOWORD(lParam);
        g_MousePos.y = HIWORD(lParam);
        return 0;
        
    case WM_LBUTTONDOWN:
        g_MouseDown = true;
        g_MouseClicked = true;
        return 0;
        
    case WM_LBUTTONUP:
        g_MouseDown = false;
        g_SelectedSlider = -1;
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
    g_Width = GetSystemMetrics(SM_CXSCREEN);
    g_Height = GetSystemMetrics(SM_CYSCREEN);
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ZeroOverlayClass";
    RegisterClassExW(&wc);
    
    g_Hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"",
        WS_POPUP,
        0, 0, g_Width, g_Height,
        nullptr, nullptr, wc.hInstance, nullptr);
    
    if (!g_Hwnd) return false;
    
    // Make transparent
    SetLayeredWindowAttributes(g_Hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(g_Hwnd, &margins);
    
    ShowWindow(g_Hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_Hwnd);
    
    return true;
}

// ============================================================================
// D3D11 & D2D SETUP
// ============================================================================
bool InitGraphics()
{
    // D3D11 with V-Sync OFF
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = g_Width;
    sd.BufferDesc.Height = g_Height;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;  // V-Sync OFF
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_Hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_SwapChain, &g_Device, nullptr, &g_Context)))
        return false;
    
    ID3D11Texture2D* backBuffer;
    g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
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
    
    // DWrite
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_WriteFactory);
    
    g_WriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"", &g_Font);
    
    g_WriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"", &g_FontBold);
    
    g_WriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"", &g_FontSmall);
    
    return true;
}

void Cleanup()
{
    if (g_FontSmall) g_FontSmall->Release();
    if (g_FontBold) g_FontBold->Release();
    if (g_Font) g_Font->Release();
    if (g_WriteFactory) g_WriteFactory->Release();
    if (g_Brush) g_Brush->Release();
    if (g_D2DTarget) g_D2DTarget->Release();
    if (g_D2DFactory) g_D2DFactory->Release();
    if (g_RenderTarget) g_RenderTarget->Release();
    if (g_SwapChain) g_SwapChain->Release();
    if (g_Context) g_Context->Release();
    if (g_Device) g_Device->Release();
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================
void DrawText(const wchar_t* text, float x, float y, D2D1_COLOR_F col, IDWriteTextFormat* font = nullptr)
{
    if (!font) font = g_Font;
    g_Brush->SetColor(col);
    D2D1_RECT_F r = { x, y, x + 500, y + 50 };
    g_D2DTarget->DrawText(text, (UINT32)wcslen(text), font, r, g_Brush);
}

void FillRect(float x, float y, float w, float h, D2D1_COLOR_F col)
{
    g_Brush->SetColor(col);
    g_D2DTarget->FillRectangle({ x, y, x + w, y + h }, g_Brush);
}

void DrawRoundedRect(float x, float y, float w, float h, float r, D2D1_COLOR_F col, bool fill)
{
    g_Brush->SetColor(col);
    D2D1_ROUNDED_RECT rr = { {x, y, x + w, y + h}, r, r };
    if (fill)
        g_D2DTarget->FillRoundedRectangle(rr, g_Brush);
    else
        g_D2DTarget->DrawRoundedRectangle(rr, g_Brush, 2.0f);
}

void DrawCircle(float cx, float cy, float r, D2D1_COLOR_F col, bool fill)
{
    g_Brush->SetColor(col);
    D2D1_ELLIPSE e = { {cx, cy}, r, r };
    if (fill)
        g_D2DTarget->FillEllipse(e, g_Brush);
    else
        g_D2DTarget->DrawEllipse(e, g_Brush, 2.0f);
}

void DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F col, float width = 1.0f)
{
    g_Brush->SetColor(col);
    g_D2DTarget->DrawLine({ x1, y1 }, { x2, y2 }, g_Brush, width);
}

bool InRect(int mx, int my, float x, float y, float w, float h)
{
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

// ============================================================================
// UI WIDGETS
// ============================================================================
D2D1_COLOR_F COL_BG = { 0.06f, 0.06f, 0.06f, 0.97f };
D2D1_COLOR_F COL_PANEL = { 0.08f, 0.08f, 0.08f, 1.0f };
D2D1_COLOR_F COL_RED = { 0.85f, 0.18f, 0.18f, 1.0f };
D2D1_COLOR_F COL_RED_DARK = { 0.55f, 0.12f, 0.12f, 1.0f };
D2D1_COLOR_F COL_GREEN = { 0.18f, 0.85f, 0.18f, 1.0f };
D2D1_COLOR_F COL_GRAY = { 0.25f, 0.25f, 0.25f, 1.0f };
D2D1_COLOR_F COL_GRAY_L = { 0.35f, 0.35f, 0.35f, 1.0f };
D2D1_COLOR_F COL_WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
D2D1_COLOR_F COL_DIM = { 0.5f, 0.5f, 0.5f, 1.0f };

static int g_WidgetID = 0;

bool Toggle(const wchar_t* label, bool* value, float x, float y)
{
    int id = g_WidgetID++;
    float toggleW = 44, toggleH = 22;
    float labelW = 200;
    
    bool hovered = InRect(g_MousePos.x, g_MousePos.y, x, y, labelW + toggleW, toggleH);
    
    if (hovered && g_MouseClicked)
    {
        *value = !*value;
    }
    
    // Label
    DrawText(label, x, y + 2, hovered ? COL_WHITE : COL_DIM);
    
    // Toggle background
    float tx = x + labelW;
    D2D1_COLOR_F bgCol = *value ? COL_RED : COL_GRAY;
    DrawRoundedRect(tx, y, toggleW, toggleH, toggleH / 2, bgCol, true);
    
    // Toggle knob
    float knobX = *value ? tx + toggleW - toggleH / 2 - 2 : tx + toggleH / 2 + 2;
    DrawCircle(knobX, y + toggleH / 2, toggleH / 2 - 3, COL_WHITE, true);
    
    return hovered && g_MouseClicked;
}

bool Slider(const wchar_t* label, float* value, float minV, float maxV, const wchar_t* fmt, float x, float y, int sliderID)
{
    g_WidgetID++;
    float sliderW = 200, sliderH = 12;
    float labelW = 150;
    
    bool hovered = InRect(g_MousePos.x, g_MousePos.y, x + labelW, y, sliderW, sliderH + 10);
    
    if (hovered && g_MouseDown)
    {
        g_SelectedSlider = sliderID;
    }
    
    if (g_SelectedSlider == sliderID && g_MouseDown)
    {
        float pct = ((float)g_MousePos.x - (x + labelW)) / sliderW;
        pct = pct < 0 ? 0 : (pct > 1 ? 1 : pct);
        *value = minV + pct * (maxV - minV);
    }
    
    // Label
    DrawText(label, x, y, COL_DIM);
    
    // Value text
    wchar_t buf[32];
    swprintf_s(buf, fmt, *value);
    float textX = x + labelW + sliderW + 10;
    DrawText(buf, textX, y, COL_RED);
    
    // Slider bg
    float sy = y + 3;
    FillRect(x + labelW, sy, sliderW, sliderH, COL_GRAY);
    
    // Slider fill
    float pct = (*value - minV) / (maxV - minV);
    FillRect(x + labelW, sy, sliderW * pct, sliderH, COL_RED);
    
    // Knob
    float knobX = x + labelW + sliderW * pct;
    DrawCircle(knobX, sy + sliderH / 2, 8, hovered || g_SelectedSlider == sliderID ? COL_WHITE : COL_GRAY_L, true);
    
    return false;
}

bool Button(const wchar_t* label, float x, float y, float w, float h, bool selected = false)
{
    g_WidgetID++;
    bool hovered = InRect(g_MousePos.x, g_MousePos.y, x, y, w, h);
    bool clicked = hovered && g_MouseClicked;
    
    D2D1_COLOR_F col = selected ? COL_RED : (hovered ? COL_RED_DARK : COL_GRAY);
    DrawRoundedRect(x, y, w, h, 6, col, true);
    
    // Center text
    DrawText(label, x + 15, y + h / 2 - 8, COL_WHITE);
    
    return clicked;
}

void Combo(const wchar_t* label, int* value, const wchar_t** options, int count, float x, float y)
{
    g_WidgetID++;
    float comboW = 180, comboH = 26;
    float labelW = 120;
    
    bool hovered = InRect(g_MousePos.x, g_MousePos.y, x + labelW, y, comboW, comboH);
    
    if (hovered && g_MouseClicked)
    {
        *value = (*value + 1) % count;
    }
    
    DrawText(label, x, y + 4, COL_DIM);
    
    DrawRoundedRect(x + labelW, y, comboW, comboH, 4, hovered ? COL_GRAY_L : COL_GRAY, true);
    DrawText(options[*value], x + labelW + 10, y + 4, COL_WHITE);
    DrawText(L"▼", x + labelW + comboW - 25, y + 4, COL_DIM);
}

// ============================================================================
// RENDER MENU
// ============================================================================
void RenderMenu()
{
    if (!g_MenuVisible) return;
    
    g_WidgetID = 0;
    
    float menuX = 50, menuY = 50;
    float menuW = 550, menuH = 500;
    
    // Background
    DrawRoundedRect(menuX, menuY, menuW, menuH, 12, COL_BG, true);
    DrawRoundedRect(menuX, menuY, menuW, menuH, 12, COL_RED_DARK, false);
    
    // Title bar
    DrawRoundedRect(menuX + 2, menuY + 2, menuW - 4, 45, 10, COL_RED_DARK, true);
    DrawText(L"PROJECT ZERO | BO6 DMA", menuX + 20, menuY + 12, COL_WHITE, g_FontBold);
    
    DrawText(L"STATUS:", menuX + 340, menuY + 14, COL_DIM);
    DrawText(L"ONLINE", menuX + 410, menuY + 14, COL_GREEN, g_FontBold);
    
    // Tabs
    float tabY = menuY + 55;
    float tabW = 100, tabH = 32;
    const wchar_t* tabs[] = { L"AIMBOT", L"ESP", L"RADAR", L"MISC" };
    
    for (int i = 0; i < 4; i++)
    {
        float tx = menuX + 15 + i * (tabW + 8);
        if (Button(tabs[i], tx, tabY, tabW, tabH, g_CurrentTab == i))
        {
            g_CurrentTab = i;
        }
    }
    
    // Content
    float cx = menuX + 20;
    float cy = tabY + 50;
    float lineH = 32;
    int line = 0;
    
    // ==================== AIMBOT ====================
    if (g_CurrentTab == 0)
    {
        Toggle(L"Enable Aimbot", &g_Settings.aimbotEnabled, cx, cy + line * lineH); line++;
        
        if (g_Settings.aimbotEnabled)
        {
            line++;
            Slider(L"FOV", &g_Settings.aimbotFOV, 1, 180, L"%.0f°", cx, cy + line * lineH, 1); line++;
            Slider(L"Smoothness", &g_Settings.aimbotSmooth, 1, 20, L"%.1f", cx, cy + line * lineH, 2); line++;
            Slider(L"Max Distance", &g_Settings.aimbotMaxDist, 10, 500, L"%.0fm", cx, cy + line * lineH, 3); line++;
            
            line++;
            const wchar_t* bones[] = { L"Head", L"Neck", L"Chest", L"Pelvis" };
            Combo(L"Target Bone", &g_Settings.aimbotBone, bones, 4, cx, cy + line * lineH); line++;
            
            const wchar_t* keys[] = { L"Left Click", L"Right Click", L"Shift" };
            Combo(L"Aim Key", &g_Settings.aimbotKey, keys, 3, cx, cy + line * lineH); line++;
            
            const wchar_t* prio[] = { L"Crosshair", L"Distance", L"Health" };
            Combo(L"Priority", &g_Settings.aimbotPriority, prio, 3, cx, cy + line * lineH); line++;
            
            line++;
            Toggle(L"Visibility Check", &g_Settings.aimbotVisCheck, cx, cy + line * lineH); line++;
        }
        
        line++;
        DrawText(L"Quick Toggles", cx, cy + line * lineH, COL_DIM); line++;
        Toggle(L"Triggerbot", &g_Settings.triggerbot, cx, cy + line * lineH); line++;
        Toggle(L"Magic Bullet", &g_Settings.magicBullet, cx, cy + line * lineH); line++;
    }
    
    // ==================== ESP ====================
    else if (g_CurrentTab == 1)
    {
        Toggle(L"Enable ESP", &g_Settings.espEnabled, cx, cy + line * lineH); line++;
        
        if (g_Settings.espEnabled)
        {
            line++;
            Toggle(L"Box ESP", &g_Settings.espBox, cx, cy + line * lineH); line++;
            
            if (g_Settings.espBox)
            {
                const wchar_t* types[] = { L"2D Box", L"Corner Box" };
                Combo(L"Box Type", &g_Settings.espBoxType, types, 2, cx + 20, cy + line * lineH); line++;
            }
            
            Toggle(L"Skeleton ESP", &g_Settings.espSkeleton, cx, cy + line * lineH); line++;
            Toggle(L"Health Bars", &g_Settings.espHealth, cx, cy + line * lineH); line++;
            Toggle(L"Name Tags", &g_Settings.espName, cx, cy + line * lineH); line++;
            Toggle(L"Distance", &g_Settings.espDistance, cx, cy + line * lineH); line++;
            Toggle(L"Snaplines", &g_Settings.espSnapline, cx, cy + line * lineH); line++;
        }
    }
    
    // ==================== RADAR ====================
    else if (g_CurrentTab == 2)
    {
        Toggle(L"Enable Radar", &g_Settings.radarEnabled, cx, cy + line * lineH); line++;
        
        line++;
        Slider(L"Size", &g_Settings.radarSize, 150, 400, L"%.0f px", cx, cy + line * lineH, 10); line++;
        Slider(L"Zoom", &g_Settings.radarZoom, 0.5f, 5.0f, L"%.1fx", cx, cy + line * lineH, 11); line++;
        
        line++;
        Toggle(L"Show Enemies", &g_Settings.radarShowEnemy, cx, cy + line * lineH); line++;
        Toggle(L"Show Teammates", &g_Settings.radarShowTeam, cx, cy + line * lineH); line++;
        
        line++;
        line++;
        wchar_t buf[32];
        swprintf_s(buf, L"Players: %d", (int)g_Players.size());
        DrawText(buf, cx, cy + line * lineH, COL_WHITE);
    }
    
    // ==================== MISC ====================
    else if (g_CurrentTab == 3)
    {
        Toggle(L"No Flash", &g_Settings.noFlash, cx, cy + line * lineH); line++;
        Toggle(L"No Smoke", &g_Settings.noSmoke, cx, cy + line * lineH); line++;
        Toggle(L"Bhop", &g_Settings.bhop, cx, cy + line * lineH); line++;
        Toggle(L"Radar Hack (UAV)", &g_Settings.radarHack, cx, cy + line * lineH); line++;
        
        line++;
        line++;
        DrawText(L"Controls", cx, cy + line * lineH, COL_DIM); line++;
        DrawText(L"INSERT - Toggle Menu", cx, cy + line * lineH, COL_WHITE, g_FontSmall); line++;
        DrawText(L"END - Exit Program", cx, cy + line * lineH, COL_WHITE, g_FontSmall); line++;
        
        line++;
        line++;
        DrawText(L"PROJECT ZERO v2.0", cx, cy + line * lineH, COL_RED); line++;
        DrawText(L"Professional DMA Radar for BO6", cx, cy + line * lineH, COL_DIM, g_FontSmall);
    }
    
    g_MouseClicked = false;
}

// ============================================================================
// RENDER RADAR
// ============================================================================
void RenderRadar()
{
    if (!g_Settings.radarEnabled) return;
    
    std::lock_guard<std::mutex> lock(g_PlayerMutex);
    
    float rx = (float)g_Width - g_Settings.radarSize - 30;
    float ry = 30;
    float size = g_Settings.radarSize;
    float cx = rx + size / 2;
    float cy = ry + size / 2;
    
    // Background
    DrawRoundedRect(rx, ry, size, size, 10, COL_BG, true);
    DrawRoundedRect(rx, ry, size, size, 10, COL_RED_DARK, false);
    
    // Grid
    D2D1_COLOR_F gridCol = { 0.2f, 0.2f, 0.25f, 0.8f };
    DrawLine(cx, ry + 10, cx, ry + size - 10, gridCol);
    DrawLine(rx + 10, cy, rx + size - 10, cy, gridCol);
    for (int i = 1; i <= 3; i++)
    {
        float r = (size * 0.45f) * (i / 3.0f);
        DrawCircle(cx, cy, r, gridCol, false);
    }
    
    // Players
    for (const Player& p : g_Players)
    {
        if (!p.valid) continue;
        if (p.isEnemy && !g_Settings.radarShowEnemy) continue;
        if (!p.isEnemy && !g_Settings.radarShowTeam) continue;
        
        // Transform world to radar
        float dx = p.worldX;
        float dy = p.worldY;
        
        float rad = g_LocalYaw * 3.14159f / 180.0f;
        float rotX = dx * cosf(rad) - dy * sinf(rad);
        float rotY = dx * sinf(rad) + dy * cosf(rad);
        
        float scale = (size * 0.4f) / (100.0f / g_Settings.radarZoom);
        rotX *= scale;
        rotY *= scale;
        
        float maxR = size * 0.4f;
        float len = sqrtf(rotX * rotX + rotY * rotY);
        if (len > maxR) { rotX = rotX / len * maxR; rotY = rotY / len * maxR; }
        
        float px = cx + rotX;
        float py = cy - rotY;
        
        D2D1_COLOR_F col = p.isEnemy ? COL_RED : D2D1::ColorF(0.3f, 0.5f, 1.0f);
        
        // Glow
        D2D1_COLOR_F glow = col;
        glow.a = 0.3f;
        DrawCircle(px, py, 8, glow, true);
        
        // Dot
        DrawCircle(px, py, 5, col, true);
        
        // Direction
        float yawRad = p.yaw * 3.14159f / 180.0f;
        DrawLine(px, py, px + sinf(yawRad) * 12, py - cosf(yawRad) * 12, col, 2.0f);
    }
    
    // Local player
    DrawCircle(cx, cy, 7, COL_GREEN, true);
    float viewRad = g_LocalYaw * 3.14159f / 180.0f;
    DrawLine(cx, cy, cx + sinf(viewRad) * 18, cy - cosf(viewRad) * 18, COL_GREEN, 2.5f);
    
    // Title
    DrawText(L"RADAR", rx + 10, ry + size + 5, COL_WHITE, g_FontSmall);
    wchar_t buf[32];
    swprintf_s(buf, L"%d players", (int)g_Players.size());
    DrawText(buf, rx + size - 80, ry + size + 5, COL_DIM, g_FontSmall);
}

// ============================================================================
// MAIN
// ============================================================================
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // Create overlay
    if (!CreateOverlayWindow()) return 1;
    if (!InitGraphics()) return 1;
    
    // Start threads
    std::thread inputThread(InputThread);
    std::thread updateThread(UpdateThread);
    
    // Update offsets (pattern scan)
    PatternScanner::UpdateOffsets();
    
    // Main loop
    while (g_Running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        // Handle INSERT toggle
        if (g_InsertPressed.exchange(false))
        {
            g_MenuVisible = !g_MenuVisible;
            g_Clickable = g_MenuVisible.load();
            SetClickThrough(!g_Clickable);
        }
        
        // Render
        float clearColor[4] = { 0, 0, 0, 0 };
        g_Context->ClearRenderTargetView(g_RenderTarget, clearColor);
        
        g_D2DTarget->BeginDraw();
        g_D2DTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
        
        RenderRadar();
        RenderMenu();
        
        g_D2DTarget->EndDraw();
        
        // Present with V-Sync OFF (0, 0)
        g_SwapChain->Present(0, 0);
    }
    
    // Cleanup
    g_Running = false;
    inputThread.join();
    updateThread.join();
    Cleanup();
    DestroyWindow(g_Hwnd);
    
    return 0;
}

int main() { return wWinMain(nullptr, nullptr, nullptr, 0); }
