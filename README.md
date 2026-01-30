# PROJECT ZERO | BO6 DMA

Professional DMA Overlay for Black Ops 6 with Radar, ESP, and Aimbot.

## Screenshots

The application displays:
- **Menu** (Left side) - Dark/Red themed with tabs: AIMBOT, VISUALS, RADAR, MISC
- **Radar** (Top right corner) - Standalone window showing player positions
- **ESP** - Boxes, health bars, names on players
- **Status Indicator** - Shows ONLINE (green) when DMA is connected

## Features

### Auto-Launch
- Menu appears **immediately** when you start the program
- No need to press INSERT to show menu initially
- INSERT key toggles menu on/off after startup

### DMA Engine
- **DMA_ENABLED = 1** by default (ready for FPGA hardware)
- **Scatter Reads** - Batches all memory reads for maximum performance
- **Pattern Scanner (AOB Scan)** - Automatically finds game offsets
- Falls back to simulation mode if FPGA not connected

### Pattern Scanner
Automatically scans for game addresses using these patterns:
- `PlayerBase`: `48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 8B 01`
- `ClientInfo`: `48 8B 1D ? ? ? ? 48 85 DB 74 ? 48 8B 03`
- `EntityList`: `48 8D 0D ? ? ? ? E8 ? ? ? ? 48 85 C0 74`
- `ViewMatrix`: `48 8B 15 ? ? ? ? 48 8D 4C 24 ? E8`

### Aimbot Tab
- Enable/Disable toggle
- FOV slider (10-300)
- Smoothness control (1-20)
- Max Distance (10-500m)
- Target Bone selection (Head/Neck/Chest)
- Visibility Check option
- Show FOV Circle toggle

### Visuals Tab (ESP)
- Box ESP (2D or Corner style)
- Skeleton ESP
- Health Bars (color changes based on HP)
- Name Tags
- Distance display
- Snaplines (from Top/Center/Bottom)
- Show Teammates option

### Radar Tab
- 2D Radar with player dots
- Size adjustment (150-350px)
- Zoom control (0.5x-4x)
- Show/Hide Enemies
- Show/Hide Teammates
- Player count display

### Misc Tab
- No Recoil (placeholder)
- Rapid Fire (placeholder)
- Crosshair overlay (Cross/Dot/Circle styles)
- Crosshair size adjustment
- Stream Proof mode (placeholder)

## Controls

| Key | Action |
|-----|--------|
| **INSERT** | Toggle Menu Visibility |
| **END** | Exit Program |
| **Mouse** | Click & Drag controls |

## Status Indicator

The status shows DMA connection state:
- **ONLINE** (Green) - FPGA connected and cod.exe found
- **SIMULATION** (Yellow) - Running in demo mode
- **FPGA ERROR** (Red) - Hardware not detected
- **PROCESS NOT FOUND** (Red) - cod.exe not running

## Building

### Requirements
- Visual Studio 2022
- Windows 10/11 SDK
- DirectX 11 (included with Windows)

### For DMA Hardware
1. Ensure `vmmdll.lib` is in `libs/` folder
2. Place `vmmdll.dll`, `leechcore.dll`, `FTD3XX.dll` next to exe
3. `DMA_ENABLED` is already set to `1`
4. Build in **Release | x64**

### Without DMA Hardware
The program will run in **Simulation Mode** with animated test players.
This is useful for testing UI and features.

## Project Structure

```
ZeroElite/
├── src/
│   ├── ZeroMain.cpp      # Main app, window, rendering, UI
│   ├── ZeroUI.cpp        # UI stubs (for compatibility)
│   ├── ZeroUI.hpp        # UI declarations
│   ├── DMA_Engine.cpp    # DMA, Scatter, Pattern Scanner
│   └── DMA_Engine.hpp    # DMA structures
├── include/
│   └── vmmdll.h          # VMMDLL header
├── libs/
│   └── vmmdll.lib        # VMMDLL library
├── ZeroElite.sln
├── ZeroElite.vcxproj
└── README.md
```

## Technical Details

### Performance Optimizations
- **V-Sync OFF** - Minimal input lag
- **Scatter Reads** - Single DMA request for all players
- **120Hz Update Thread** - Smooth radar updates
- **Direct2D Rendering** - Hardware accelerated 2D

### Overlay Technology
- Transparent window with `WS_EX_LAYERED`
- Click-through when menu hidden (`WS_EX_TRANSPARENT`)
- Always-on-top (`WS_EX_TOPMOST`)
- DWM glass effect for true transparency

### Threading Model
- **Main Thread** - Window messages + Rendering
- **Input Thread** - Keyboard monitoring (non-blocking)
- **Update Thread** - Player data reading/simulation

## Adding New Patterns

To add a new pattern for auto-offset:

```cpp
// In DMA_Engine.hpp - Add to Patterns namespace
constexpr const char* NewPattern = "\x48\x89\x5C\x24\x00\x57";
constexpr const char* NewPatternMask = "xxxx?x";

// In DMA_Engine.cpp - UpdateAllOffsets()
addr = FindPattern(baseAddr, moduleSize, Patterns::NewPattern, Patterns::NewPatternMask);
if (addr) {
    g_Offsets.NewOffset = ResolveRelative(addr, 3, 7);
    if (g_Offsets.NewOffset) s_FoundCount++;
}
```

## Process Name

Target: `cod.exe` (Call of Duty unified engine)

## Version

**PROJECT ZERO v2.1** - Professional DMA Radar

## Disclaimer

This software is for educational purposes only.
Use at your own risk. Developer is not responsible for any consequences.
