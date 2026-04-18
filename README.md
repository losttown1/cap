# Zero Elite

**Zero Elite** is a DirectX 11 + ImGui game overlay scaffold (x64-only) with a transparent click-through window and a professional menu.

## Open & Build (Windows / Visual Studio)

- Open `ZeroElite.sln` in Visual Studio 2022+.
- Ensure you build **x64** (solution is configured for x64 only).
- Build `Debug|x64` or `Release|x64`.

## Controls

- **INSERT** toggles the menu.
  - When the menu is closed, the overlay is **click-through** (`WS_EX_TRANSPARENT`).
  - When the menu is open, click-through is disabled so you can interact with ImGui.

## Project Layout

- `src/`
  - `ZeroMain.cpp` (WinMain + DX11 overlay loop)
  - `ZeroUI.cpp` (ImGui menu: ESP, Aimbot, Refresh Rate)
  - `DMA_Engine.cpp` (DMA scaffold + required vmmdll link pragma)
- `include/`
  - `imgui/` (ImGui headers)
- `libs/`
  - `vmmdll.lib` (buildable stub library shipped with this scaffold)

## DMA Settings (Scaffold Constants)

- **Target PID**: `35028`
- **Handle**: `0x021EE040`

The DMA engine file currently initializes as a stub (no real vmmdll calls). If you have the real `vmmdll.lib`, replace `libs/vmmdll.lib` with your actual library and implement VMMDLL calls in `src/DMA_Engine.cpp`.

