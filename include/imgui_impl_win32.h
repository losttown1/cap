// dear imgui: Platform Backend for Windows (standard windows API for 32-bits AND 64-bits applications)
// Zero Elite - ImGui Win32 Backend Header

#pragma once
#include "imgui.h"

IMGUI_IMPL_API bool     ImGui_ImplWin32_Init(void* hwnd);
IMGUI_IMPL_API bool     ImGui_ImplWin32_InitForOpenGL(void* hwnd);
IMGUI_IMPL_API void     ImGui_ImplWin32_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplWin32_NewFrame();

// Win32 message handler
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
IMGUI_IMPL_API void     ImGui_ImplWin32_EnableDpiAwareness();
IMGUI_IMPL_API float    ImGui_ImplWin32_GetDpiScaleForHwnd(void* hwnd);
IMGUI_IMPL_API float    ImGui_ImplWin32_GetDpiScaleForMonitor(void* monitor);
#endif

// Win32 message handler (for any window)
extern IMGUI_IMPL_API long long ImGui_ImplWin32_WndProcHandler(void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam);
