# Zero Elite - Game Overlay

A professional DirectX 11 game overlay built with ImGui, featuring DMA (Direct Memory Access) integration for advanced memory operations.

## Features

- **Transparent Overlay**: Fully transparent window using `WS_EX_LAYERED` and `WS_EX_TRANSPARENT`
- **DirectX 11 Rendering**: High-performance graphics with D3D11
- **ImGui Interface**: Professional, modern UI with dark theme
- **DMA Integration**: VMMDLL-based memory access
- **Hotkey Support**: INSERT key to toggle menu

### UI Features

- **ESP Toggle**: Enable/disable player ESP visualization
- **Aimbot Toggle**: Enable/disable aim assistance
- **Refresh Rate Slider**: Adjust overlay refresh rate (30-240 Hz)
- **Theme Customization**: Multiple color themes
- **DMA Status Panel**: Real-time DMA connection status

## Project Structure

```
ZeroElite/
├── include/                 # Header files
│   ├── imgui.h              # ImGui core header
│   ├── imgui_internal.h     # ImGui internal structures
│   ├── imgui_impl_dx11.h    # DirectX 11 backend
│   ├── imgui_impl_win32.h   # Win32 backend
│   ├── imconfig.h           # ImGui configuration
│   └── vmmdll.h             # VMMDLL interface header
├── src/                     # Source files
│   ├── ZeroMain.cpp         # Application entry point
│   ├── ZeroUI.cpp           # User interface implementation
│   ├── ZeroUI.h             # User interface header
│   ├── DMA_Engine.cpp       # DMA memory operations
│   ├── DMA_Engine.h         # DMA engine header
│   ├── imgui.cpp            # ImGui core implementation
│   ├── imgui_draw.cpp       # ImGui drawing routines
│   ├── imgui_tables.cpp     # ImGui tables
│   ├── imgui_widgets.cpp    # ImGui widgets
│   ├── imgui_impl_dx11.cpp  # DirectX 11 backend
│   └── imgui_impl_win32.cpp # Win32 backend
├── libs/                    # Libraries
│   ├── vmmdll.lib           # VMMDLL import library (placeholder)
│   └── README.txt           # Library setup instructions
├── ZeroElite.sln            # Visual Studio solution
├── ZeroElite.vcxproj        # Visual Studio project
├── ZeroElite.vcxproj.filters# Project filters
└── README.md                # This file
```

## Requirements

- **Visual Studio 2019/2022** with C++ desktop development workload
- **Windows SDK** (10.0 or later)
- **DirectX 11 SDK** (included with Windows SDK)
- **VMMDLL Library** (from MemProcFS project)

## Build Instructions

### Step 1: Install Prerequisites

1. Install Visual Studio 2019 or 2022 with:
   - Desktop development with C++
   - Windows 10/11 SDK

### Step 2: Setup VMMDLL Library

1. Download MemProcFS from: https://github.com/ufrisk/MemProcFS
2. Extract the package
3. Copy `vmmdll.lib` to the `libs/` folder (replace the placeholder)

### Step 3: Build the Project

1. Open `ZeroElite.sln` in Visual Studio
2. Select **Release | x64** configuration
3. Build the solution (F7 or Build > Build Solution)

### Step 4: Run

1. Copy required DLL files to the output folder:
   - `vmm.dll`
   - `leechcore.dll`
   - `FTD3XX.dll` (for FPGA devices)
2. Run `ZeroElite.exe`

## Configuration

### DMA Settings

Located in `src/DMA_Engine.h`:

```cpp
constexpr DWORD TARGET_PID = 35028;
constexpr ULONG64 TARGET_HANDLE = 0x021EE040;
```

Modify these values to match your target process.

### Hotkeys

| Key      | Action           |
|----------|------------------|
| INSERT   | Toggle Menu      |
| ESC      | Close Menu       |
| END      | Exit Application |
| F1       | Toggle ESP       |
| F2       | Toggle Aimbot    |

## Technical Details

### Overlay Window

The overlay uses the following Windows styles for transparency:
- `WS_EX_LAYERED`: Enables layered window
- `WS_EX_TRANSPARENT`: Click-through when menu is hidden
- `WS_EX_TOPMOST`: Always on top
- `WS_EX_TOOLWINDOW`: Hidden from taskbar

### ImGui Integration

The project includes a full ImGui implementation with:
- DirectX 11 renderer backend
- Win32 platform backend
- Custom dark theme with accent colors

### DMA Memory Access

Uses VMMDLL for direct memory access:
- Process memory reading/writing
- Physical memory access
- Scatter read operations for efficiency

## Architecture

This project is configured for **x64 architecture only**. Win32 builds are not supported due to DMA hardware requirements.

## License

This project is provided as-is for educational purposes. Use responsibly and in accordance with all applicable laws and terms of service.

## Disclaimer

This software is intended for educational and research purposes only. The authors are not responsible for any misuse of this software. Always ensure you have permission before accessing any computer system or process memory.

---

**Zero Elite** - Professional Game Overlay Framework
