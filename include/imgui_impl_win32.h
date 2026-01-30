#pragma once
#include <windows.h>

bool ImGui_ImplWin32_Init(void* hwnd);
void ImGui_ImplWin32_Shutdown();
void ImGui_ImplWin32_NewFrame();
