#ifndef ZERO_UI_HPP
#define ZERO_UI_HPP

#include <imgui.h>

// تعريف المتغيرات العامة للتحكم من ملفات أخرى
extern bool esp_enabled;
extern bool aimbot_enabled;
extern float refresh_rate;
extern float box_color[4];

// تعريف دالة رسم القائمة
void RenderZeroMenu();

// دوال التهيئة والإغلاق
void InitializeZeroUI();
void ShutdownZeroUI();

// دوال التحكم بظهور القائمة
void ToggleZeroMenu();
bool IsZeroMenuVisible();
void SetZeroMenuVisible(bool visible);

#endif // ZERO_UI_HPP
