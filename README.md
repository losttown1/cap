# Zero Elite Overlay

A C++ Game Overlay project using DirectX 11 and ImGui.

## Project Structure

- `src/`: Source code files (ZeroMain, ZeroUI, DMA_Engine).
- `libs/`: External libraries (vmmdll.lib placeholder).
- `include/`: Header files for ImGui and vmmdll.
- `ZeroElite.sln`: Visual Studio 2022 Solution.

## Features

- **Menu**: Professional menu toggleable with `INSERT` key.
- **Visuals**: Fully transparent overlay using `WS_EX_LAYERED` and `WS_EX_TRANSPARENT`.
- **DMA**: Configured for PID `35028` and Handle `0x021EE040`.
- **Modules**: ESP Toggle, Aimbot Toggle, Refresh Rate Slider.

## Build Instructions

1. Open `ZeroElite.sln` in Visual Studio 2022.
2. Select `x64` configuration.
3. Build the solution.

## Note

This project contains placeholder files for `vmmdll.lib` and minimal stubs for ImGui to ensure the project structure is valid and compilable. For a functional overlay, replace the contents of `libs/` and `include/` with the actual libraries and source files from the respective vendors (ImGui, Vmmdll).
