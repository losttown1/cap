#pragma once

#include <cstdint>

struct ZeroSettings
{
    bool espEnabled = false;
    bool aimbotEnabled = false;
    int refreshRateHz = 144; // slider-controlled
};

class ZeroUI
{
public:
    void ApplyStyle();
    void Render(bool* menuOpen, ZeroSettings& settings);
};

