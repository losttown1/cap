#ifndef ZERO_UI_HPP
#define ZERO_UI_HPP

#include <imgui.h>

// Global settings accessible from other files
extern bool esp_enabled;
extern bool aimbot_enabled;
extern float refresh_rate;
extern float box_color[4];

// UI functions
void InitializeZeroUI();
void ShutdownZeroUI();
void RenderZeroMenu();
void ToggleZeroMenu();
bool IsZeroMenuVisible();
void SetZeroMenuVisible(bool visible);

#endif // ZERO_UI_HPP
