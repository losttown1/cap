# PROJECT ZERO | BO6 DMA Radar

Professional DMA Overlay for Black Ops 6 with Radar, ESP, and Aimbot features.

## Features

### Core Features
- **Transparent Overlay** - Renders on top of game without interfering with input
- **Mouse-Controlled Menu** - Full mouse support with toggles, sliders, and combo boxes
- **Non-Blocking Input** - INSERT key handled in separate thread (no freezing!)
- **V-Sync OFF** - Minimal input lag for radar updates
- **DMA Scatter Reads** - Batch multiple memory reads for smooth performance
- **Pattern Scanner** - Auto-update offsets when game patches (stub implementation)

### Radar
- Real-time 2D radar with enemy/team positions
- Configurable size and zoom
- Player direction indicators
- Color-coded dots (Red = Enemy, Blue = Team, Green = You)

### Aimbot (UI Only)
- FOV slider
- Smoothness control
- Target bone selection
- Visibility check option
- Configurable aim key

### ESP (UI Only)
- Box ESP (2D and Corner)
- Skeleton ESP
- Health bars
- Name tags
- Distance display
- Snaplines

### Misc
- No Flash
- No Smoke
- Bhop
- Radar Hack (UAV)
- Magic Bullet

## Controls

| Key | Action |
|-----|--------|
| INSERT | Toggle Menu (Show/Hide) |
| END | Exit Program |
| Mouse | Interact with Menu |

## Building

### Requirements
- Visual Studio 2022
- Windows 10 SDK
- DirectX 11 (included with Windows)

### Steps
1. Open `ZeroElite.sln` in Visual Studio
2. Select **Release | x64** configuration
3. Build solution (Ctrl+Shift+B)
4. Run `bin/Release/ZeroElite.exe`

### For Real DMA Support
1. Place `vmmdll.lib` in the `libs/` folder
2. Place `vmmdll.dll` and related DLLs next to the .exe
3. In `src/DMA_Engine.hpp`, change `#define DMA_ENABLED 0` to `#define DMA_ENABLED 1`
4. Rebuild the project

## Project Structure

```
ZeroElite/
├── src/
│   ├── ZeroMain.cpp       # Main application, window, rendering
│   ├── ZeroUI.cpp         # UI stubs (rendering in ZeroMain)
│   ├── ZeroUI.hpp         # UI declarations
│   ├── DMA_Engine.cpp     # DMA implementation & player management
│   └── DMA_Engine.hpp     # DMA structures & declarations
├── include/
│   └── vmmdll.h           # VMMDLL header (for real DMA)
├── libs/
│   └── vmmdll.lib         # VMMDLL library (optional)
├── ZeroElite.sln
├── ZeroElite.vcxproj
└── README.md
```

## Technical Details

### Rendering Pipeline
- **D3D11** - Hardware-accelerated graphics
- **D2D1** - 2D drawing (shapes, text)
- **DWrite** - Text rendering with custom fonts

### Overlay Window
- `WS_EX_TOPMOST` - Always on top
- `WS_EX_LAYERED` - Transparency support
- `WS_EX_TRANSPARENT` - Click-through when menu hidden
- `DwmExtendFrameIntoClientArea` - Glass effect for true transparency

### Threading
- **Main Thread** - Window message pump, rendering
- **Input Thread** - Keyboard monitoring (GetAsyncKeyState)
- **Update Thread** - Player data simulation/reading

### DMA Features
- **Scatter Reads** - Batch multiple memory reads for efficiency
- **Pattern Scanning** - Automatic offset updates (stub)
- **Simulation Mode** - Works without FPGA for testing

## Troubleshooting

### Menu Not Showing
- Press INSERT to toggle menu visibility
- Make sure the program is running (check taskbar)

### Can't Click Menu
- When menu is visible, overlay captures mouse input
- Press INSERT to hide menu and return to game

### Radar Not Moving
- In simulation mode, players animate automatically
- For real gameplay, ensure DMA_ENABLED = 1 and FPGA connected

## Disclaimer

This software is provided for educational purposes only. Use at your own risk.
The developer is not responsible for any bans or damages resulting from use of this software.

## Version

**PROJECT ZERO v2.0** - Professional DMA Radar for BO6
