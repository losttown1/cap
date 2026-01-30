#include "../include/imgui_impl_dx11.h"
#include "../include/imgui_impl_win32.h"

// DX11 Implementation Stubs
bool ImGui_ImplDX11_Init(ID3D11Device* device, ID3D11DeviceContext* device_context) { return true; }
void ImGui_ImplDX11_Shutdown() {}
void ImGui_ImplDX11_NewFrame() {}
void ImGui_ImplDX11_RenderDrawData(void* draw_data) {}

// Win32 Implementation Stubs
bool ImGui_ImplWin32_Init(void* hwnd) { return true; }
void ImGui_ImplWin32_Shutdown() {}
void ImGui_ImplWin32_NewFrame() {}

// Win32 Message Handler Stub
extern "C" long long ImGui_ImplWin32_WndProcHandler(void* hWnd, unsigned int msg, unsigned long long wParam, long long lParam) {
    return 0;
}
