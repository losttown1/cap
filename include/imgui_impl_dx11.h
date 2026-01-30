#pragma once

#include <d3d11.h>
#include <dxgi.h>

#include "imgui.h"

bool ImGui_ImplDX11_Init(ID3D11Device* device, ID3D11DeviceContext* device_context);
void ImGui_ImplDX11_Shutdown();
void ImGui_ImplDX11_NewFrame();
void ImGui_ImplDX11_RenderDrawData(ImGui::ImDrawData* draw_data);
void ImGui_ImplDX11_InvalidateDeviceObjects();
void ImGui_ImplDX11_SetSwapChain(IDXGISwapChain* swap_chain);
