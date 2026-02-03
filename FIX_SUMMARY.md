# Project Zero: Technical Resolution Report

This report outlines the critical technical improvements and bug fixes implemented in the Project Zero DMA codebase. The primary focus was on ensuring hardware integrity, resolving build failures, and eliminating non-data-driven rendering.

### Hardware Verification and Connectivity

The initialization sequence has been overhauled to ensure that the software only operates when genuine hardware is detected. This was achieved through two primary implementations:

| Feature | Implementation Detail | Result |
| :--- | :--- | :--- |
| **Real DMA Check** | Integrated `VMMDLL_ConfigGet` with `VMMDLL_OPT_FPGA_ID` during the connection phase. | Prevents "fake" online status; system halts if FPGA ID is 0. |
| **KMBox Auto-Pairing** | Implemented a COM port scanner (COM1-COM20) using a ping signal (`?`). | Automatically locks the correct port and logs `KMBox: AUTO-LOCKED`. |

### Rendering and Overlay Architecture

To enhance security and accuracy, the rendering pipeline was transitioned from a simulated GDI approach to a data-driven "Black Stage" architecture.

> **The Black Stage Implementation**: A solid Win32 window (`HWND`) is now created covering the full monitor resolution. By applying `SetLayeredWindowAttributes` with `LWA_COLORKEY` after window creation, the black background becomes transparent to the game, providing a stable and stealthy canvas for the radar and ESP.

The radar system is now strictly data-driven. All "random dots" and simulated movements have been removed. The `PlayerManager` now clears all entity data if the DMA scatter read fails or if the device is disconnected, ensuring the radar remains empty unless real game data is being processed.

### Build System and Automation

The recurring build error **C2664** in `DMA_Engine.cpp` was resolved by implementing proper pointer casting for the `VMMDLL_SCATTER_HANDLE`. This allows for a seamless compilation process using the standard F7 build command. Furthermore, the entire startup sequence—including hardware verification and overlay creation—is now fully automated within the `wWinMain` entry point, enabling an "Auto-Pilot" experience for the end-user.
