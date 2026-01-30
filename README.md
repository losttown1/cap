# PROJECT ZERO | BO6 DMA v2.2

Professional DMA Overlay for Black Ops 6 with Scatter Registry, Map Textures, and Config-driven Initialization.

## Professional Features

### 1. Scatter Read Registry (Maximum Performance)

Instead of reading player data randomly, we batch ALL reads into a single DMA transaction:

```cpp
// Every frame, this is ONE transaction instead of hundreds
g_ScatterRegistry.Clear();
for (int i = 0; i < playerCount; i++) {
    g_ScatterRegistry.RegisterPlayerData(i, entityAddr);
}
g_ScatterRegistry.RegisterLocalPlayer(localAddr);
g_ScatterRegistry.RegisterViewMatrix(viewMatrixAddr);
g_ScatterRegistry.ExecuteAll();  // ONE DMA call!
```

**Benefits:**
- Up to 10x faster than individual reads
- Reduced DMA latency
- Smoother radar updates

### 2. Map Texture Support

Load real game map images as radar backgrounds:

```ini
[Map]
ImagePath=maps/nuketown.png
ScaleX=1.0
ScaleY=1.0
OffsetX=0.0
OffsetY=0.0
Rotation=0.0
```

**Features:**
- PNG/JPG support
- Coordinate calibration (scale, offset, rotation)
- Pre-configured BO6 map database
- Toggle on/off in Radar tab

### 3. Hardware-ID Masking (Config-driven Init)

Load device settings from `zero.ini` instead of hardcoding:

```ini
[Device]
Type=fpga
Arg=
Algorithm=0
UseCustomPCIe=0
CustomPCIeID=
```

**Security Benefits:**
- No unique signatures in code
- Change device ID without recompiling
- Switch between DMA devices easily

## Configuration File (zero.ini)

The program automatically creates this file on first run:

```ini
; PROJECT ZERO - Configuration File
; Hardware-ID Masking: Change device settings to avoid detection

[Device]
Type=fpga
Arg=
Algorithm=0
UseCustomPCIe=0
CustomPCIeID=

[Target]
ProcessName=cod.exe

[Performance]
ScatterBatchSize=128
UpdateRateHz=120
UseScatterRegistry=1

[Map]
ImagePath=
ScaleX=1.0
ScaleY=1.0
OffsetX=0.0
OffsetY=0.0
Rotation=0.0

[Debug]
DebugMode=0
LogReads=0
```

## UI Tabs

### AIMBOT
- Enable/Disable
- FOV (10-300)
- Smoothness (1-20)
- Max Distance (10-500m)
- Target Bone (Head/Neck/Chest)
- Visibility Check
- Show FOV Circle

### VISUALS (ESP)
- Box ESP (2D/Corner)
- Skeleton ESP
- Health Bars
- Name Tags
- Distance
- Snaplines (Top/Center/Bottom)
- Show Teammates

### RADAR
- Enable/Disable
- Size (150-350px)
- Zoom (0.5x-4x)
- Show Enemies
- Show Teammates
- Use Map Texture

### MISC
- No Recoil
- Rapid Fire
- Crosshair (Cross/Dot/Circle)
- Stream Proof
- Show Performance Stats

### CONFIG
- Device info
- Process name
- Scatter batch size
- Update rate
- Pattern scan status
- Map texture status

## Performance Stats (Bottom Left)

When enabled, shows:
- FPS
- Scatter reads per frame
- Total bytes per frame
- Device info
- Pattern scan count

## Controls

| Key | Action |
|-----|--------|
| INSERT | Toggle Menu |
| END | Exit Program |
| Mouse | Interact with Menu |

## Status Indicator

- **ONLINE** (Green) - FPGA connected, process found
- **SIMULATION** (Yellow) - Demo mode
- **FPGA ERROR** (Red) - Hardware not detected
- **PROCESS NOT FOUND** (Red) - cod.exe not running

## Building

### Requirements
- Visual Studio 2022
- Windows 10/11 SDK
- DirectX 11

### Steps
1. Open `ZeroElite.sln`
2. Select **Release | x64**
3. Build (Ctrl+Shift+B)
4. Place `vmmdll.dll` + dependencies next to exe
5. Run `ZeroElite.exe`

## Adding Custom Maps

1. Get map image (PNG/JPG)
2. Find game coordinate bounds
3. Add to `zero.ini`:

```ini
[Map]
ImagePath=maps/your_map.png
ScaleX=1.0
ScaleY=1.0
OffsetX=0.0
OffsetY=0.0
Rotation=0.0
```

4. Or add to code:

```cpp
MapTextureManager::SetMapBounds("mp_custom", -5000, 5000, -5000, 5000);
```

## Pattern Scanner

Automatically finds game addresses:

| Pattern | Purpose |
|---------|---------|
| `48 8B 05 ?? ?? ?? ?? 48 8B 88` | Player Base |
| `48 8B 1D ?? ?? ?? ?? 48 85 DB 74` | Client Info |
| `48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 74` | Entity List |
| `48 8B 15 ?? ?? ?? ?? 48 8D 4C 24 ?? E8` | View Matrix |

## File Structure

```
ZeroElite/
├── src/
│   ├── ZeroMain.cpp      # Main, UI, Rendering
│   ├── DMA_Engine.cpp    # DMA, Scatter, Patterns
│   ├── DMA_Engine.hpp    # Definitions
│   ├── ZeroUI.cpp        # Stubs
│   └── ZeroUI.hpp        # Stubs
├── include/
│   └── vmmdll.h
├── libs/
│   └── vmmdll.lib
├── zero.ini              # Config (auto-created)
├── ZeroElite.sln
└── README.md
```

## Version History

- **v2.2** - Scatter Registry, Map Textures, Config File
- **v2.1** - Pattern Scanner, Full Feature Set
- **v2.0** - Direct2D UI, Auto-Launch Menu
- **v1.0** - Initial ImGui version

## Disclaimer

Educational purposes only. Use at your own risk.

---

**PROJECT ZERO v2.2** - Professional DMA Radar
