#include <windows.h>
#include <iostream>
#include "DMA_Engine.h"
#include "ZeroUI.h"

// Link vmmdll.lib using relative path as requested
#pragma comment(lib, "..\\libs\\vmmdll.lib")

// Main Entry Point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 1. Initialize DMA Engine
    DMA_Engine dma;
    if (!dma.Init()) {
        MessageBox(NULL, "Failed to initialize DMA Engine!", "Zero Elite Error", MB_ICONERROR);
        return 1;
    }

    if (!dma.AttachToProcess()) {
        MessageBox(NULL, "Failed to attach to process!", "Zero Elite Error", MB_ICONERROR);
        return 1;
    }

    // 2. Initialize UI
    ZeroUI ui;
    if (!ui.Init(hInstance)) {
        MessageBox(NULL, "Failed to initialize Zero UI!", "Zero Elite Error", MB_ICONERROR);
        return 1;
    }

    // 3. Main Loop
    while (ui.Tick()) {
        // Here you would typically read data using DMA and update the UI state
        // For example:
        // if (ui.bESPEnabled) {
        //    auto players = dma.Read<PlayerList>(address);
        //    ui.UpdateESP(players);
        // }
        
        // Control refresh rate loop if needed, or rely on VSync in Render
        if (ui.fRefreshRate < 144.0f) {
             Sleep((DWORD)(1000.0f / ui.fRefreshRate));
        }
    }

    return 0;
}
