// ZeroUI.hpp - Zero Elite User Interface Header
// PROJECT ZERO | Modern Menu Interface

#pragma once

#include "../include/imgui.h"

// Global settings for Zero system
extern bool esp_enabled;
extern bool aimbot_enabled;
extern float refresh_rate;
extern float box_color[4];

// Render the Zero menu
void RenderZeroMenu();

// Menu visibility control
void ToggleZeroMenu();
bool IsZeroMenuVisible();
void SetZeroMenuVisible(bool visible);

// Initialize and shutdown
void InitializeZeroUI();
void ShutdownZeroUI();
