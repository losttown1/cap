// dear imgui: Platform Backend for Windows (standard windows API for 32-bits AND 64-bits applications)
// Zero Elite - ImGui Win32 Backend Implementation

#include "../include/imgui.h"
#include "../include/imgui_impl_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <windowsx.h>  // For GET_X_LPARAM and GET_Y_LPARAM
#include <tchar.h>
#include <dwmapi.h>

// Win32 data
struct ImGui_ImplWin32_Data
{
    HWND                        hWnd;
    HWND                        MouseHwnd;
    bool                        MouseTracked;
    INT64                       Time;
    INT64                       TicksPerSecond;
    ImGuiMouseCursor            LastMouseCursor;
    bool                        HasGamepad;
    bool                        WantUpdateHasGamepad;

    ImGui_ImplWin32_Data() { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData
static ImGui_ImplWin32_Data* ImGui_ImplWin32_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplWin32_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

// Functions
static bool ImGui_ImplWin32_InitEx(void* hwnd, bool platform_has_own_dc)
{
    (void)platform_has_own_dc;
    
    ImGuiIO& io = ImGui::GetIO();
    if (io.BackendPlatformUserData != nullptr)
        return false;

    INT64 perf_frequency, perf_counter;
    if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&perf_frequency))
        return false;
    if (!::QueryPerformanceCounter((LARGE_INTEGER*)&perf_counter))
        return false;

    // Setup backend data
    ImGui_ImplWin32_Data* bd = new ImGui_ImplWin32_Data();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_win32";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    bd->hWnd = (HWND)hwnd;
    bd->TicksPerSecond = perf_frequency;
    bd->Time = perf_counter;
    bd->LastMouseCursor = ImGuiMouseCursor_COUNT;
    bd->WantUpdateHasGamepad = true;

    // Set display size (it will be updated in NewFrame)
    RECT rect = { 0, 0, 0, 0 };
    ::GetClientRect(bd->hWnd, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    return true;
}

bool ImGui_ImplWin32_Init(void* hwnd)
{
    return ImGui_ImplWin32_InitEx(hwnd, false);
}

bool ImGui_ImplWin32_InitForOpenGL(void* hwnd)
{
    return ImGui_ImplWin32_InitEx(hwnd, true);
}

void ImGui_ImplWin32_Shutdown()
{
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    if (!bd) return;

    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasGamepad);

    delete bd;
}

static bool ImGui_ImplWin32_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        ::SetCursor(nullptr);
    }
    else
    {
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        ::SetCursor(::LoadCursor(nullptr, win32_cursor));
    }
    return true;
}

static void ImGui_ImplWin32_UpdateMouseData()
{
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    HWND focused_window = ::GetForegroundWindow();
    const bool is_app_focused = (focused_window == bd->hWnd);
    
    if (is_app_focused)
    {
        // Update mouse position
        POINT mouse_pos;
        if (io.WantSetMousePos)
        {
            POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
            if (::ClientToScreen(bd->hWnd, &pos))
                ::SetCursorPos(pos.x, pos.y);
        }
        else if (::GetCursorPos(&mouse_pos) && ::ScreenToClient(bd->hWnd, &mouse_pos))
        {
            io.AddMousePosEvent((float)mouse_pos.x, (float)mouse_pos.y);
        }
    }
}

void ImGui_ImplWin32_NewFrame()
{
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();
    if (!bd) return;

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size
    RECT rect = { 0, 0, 0, 0 };
    ::GetClientRect(bd->hWnd, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // Setup time step
    INT64 current_time = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
    io.DeltaTime = (float)(current_time - bd->Time) / bd->TicksPerSecond;
    bd->Time = current_time;

    // Update mouse input
    ImGui_ImplWin32_UpdateMouseData();

    // Update cursor
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (bd->LastMouseCursor != mouse_cursor)
    {
        bd->LastMouseCursor = mouse_cursor;
        ImGui_ImplWin32_UpdateMouseCursor();
    }
}

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return 0;

    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Data* bd = ImGui_ImplWin32_GetBackendData();

    switch (msg)
    {
    case WM_MOUSEMOVE:
        bd->MouseHwnd = hwnd;
        if (!bd->MouseTracked)
        {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            ::TrackMouseEvent(&tme);
            bd->MouseTracked = true;
        }
        io.AddMousePosEvent((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
        break;
    case WM_MOUSELEAVE:
        if (bd->MouseHwnd == hwnd)
            bd->MouseHwnd = nullptr;
        bd->MouseTracked = false;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        break;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            int button = 0;
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
            if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
            if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
            if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == nullptr)
                ::SetCapture(hwnd);
            io.AddMouseButtonEvent(button, true);
        }
        return 0;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
        {
            int button = 0;
            if (msg == WM_LBUTTONUP) { button = 0; }
            if (msg == WM_RBUTTONUP) { button = 1; }
            if (msg == WM_MBUTTONUP) { button = 2; }
            if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            io.AddMouseButtonEvent(button, false);
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
                ::ReleaseCapture();
        }
        return 0;
    case WM_MOUSEWHEEL:
        io.AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
        return 0;
    case WM_MOUSEHWHEEL:
        io.AddMouseWheelEvent((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        {
            bool down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            int vk = (int)wParam;
            if (vk == VK_CONTROL) io.KeyCtrl = down;
            if (vk == VK_SHIFT) io.KeyShift = down;
            if (vk == VK_MENU) io.KeyAlt = down;
            if (vk == VK_LWIN || vk == VK_RWIN) io.KeySuper = down;
        }
        return 0;
    case WM_CHAR:
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacter((unsigned int)wParam);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor())
            return 1;
        return 0;
    }
    return 0;
}

// DPI-related helpers
#ifndef DPI_ENUMS_DECLARED
typedef enum { PROCESS_DPI_UNAWARE = 0, PROCESS_SYSTEM_DPI_AWARE = 1, PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
typedef enum { MDT_EFFECTIVE_DPI = 0, MDT_ANGULAR_DPI = 1, MDT_RAW_DPI = 2, MDT_DEFAULT = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;
#endif
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    (DPI_AWARENESS_CONTEXT)-3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif

typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

void ImGui_ImplWin32_EnableDpiAwareness()
{
    // if (IsWindows10OrGreater())
    {
        HINSTANCE user32 = ::LoadLibraryA("user32.dll");
        if (user32)
        {
            PFN_SetProcessDpiAwarenessContext SetProcessDpiAwarenessContextFn = (PFN_SetProcessDpiAwarenessContext)::GetProcAddress(user32, "SetProcessDpiAwarenessContext");
            if (SetProcessDpiAwarenessContextFn)
            {
                SetProcessDpiAwarenessContextFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
                ::FreeLibrary(user32);
                return;
            }
            ::FreeLibrary(user32);
        }
    }
    // if (IsWindows8Point1OrGreater())
    {
        HINSTANCE shcore = ::LoadLibraryA("shcore.dll");
        if (shcore)
        {
            PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFn = (PFN_SetProcessDpiAwareness)::GetProcAddress(shcore, "SetProcessDpiAwareness");
            if (SetProcessDpiAwarenessFn)
                SetProcessDpiAwarenessFn(PROCESS_PER_MONITOR_DPI_AWARE);
            ::FreeLibrary(shcore);
        }
    }
    // else
    //     SetProcessDPIAware();
}

float ImGui_ImplWin32_GetDpiScaleForHwnd(void* hwnd)
{
    HINSTANCE shcore = ::LoadLibraryA("shcore.dll");
    if (shcore)
    {
        PFN_GetDpiForMonitor GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore, "GetDpiForMonitor");
        if (GetDpiForMonitorFn)
        {
            HMONITOR monitor = ::MonitorFromWindow((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
            UINT xdpi = 96, ydpi = 96;
            if (GetDpiForMonitorFn(monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) == S_OK)
            {
                ::FreeLibrary(shcore);
                return xdpi / 96.0f;
            }
        }
        ::FreeLibrary(shcore);
    }
    return 1.0f;
}

float ImGui_ImplWin32_GetDpiScaleForMonitor(void* monitor)
{
    HINSTANCE shcore = ::LoadLibraryA("shcore.dll");
    if (shcore)
    {
        PFN_GetDpiForMonitor GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore, "GetDpiForMonitor");
        if (GetDpiForMonitorFn)
        {
            UINT xdpi = 96, ydpi = 96;
            if (GetDpiForMonitorFn((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) == S_OK)
            {
                ::FreeLibrary(shcore);
                return xdpi / 96.0f;
            }
        }
        ::FreeLibrary(shcore);
    }
    return 1.0f;
}

// GET_X_LPARAM / GET_Y_LPARAM fallback (already defined in windowsx.h)
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
