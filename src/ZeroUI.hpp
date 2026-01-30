// ZeroUI.hpp - PROJECT ZERO User Interface Header
// Modern ImGui Menu Interface

#pragma once

// ImGui Headers
#include "../include/imgui.h"

// إعدادات النظام العامة
extern bool esp_enabled;
extern bool aimbot_enabled;
extern float refresh_rate;
extern float box_color[4];

// دوال التهيئة والإغلاق
void InitializeZeroUI();
void ShutdownZeroUI();

// دالة رسم القائمة
void RenderZeroMenu();

// دوال التحكم بظهور القائمة
void ToggleZeroMenu();
bool IsZeroMenuVisible();
void SetZeroMenuVisible(bool visible);
