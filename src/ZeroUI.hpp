// ZeroUI.hpp - UI Declarations
#pragma once

// UI State
extern bool g_MenuVisible;
extern int g_CurrentTab;

// UI Functions
void InitializeZeroUI();
void RenderZeroMenu();
void ToggleZeroMenu();
bool IsZeroMenuVisible();
void SetZeroMenuVisible(bool visible);
