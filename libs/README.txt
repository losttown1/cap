================================================================================
                         REQUIRED LIBRARY FILES
================================================================================

Place the following files in this folder (libs/) for the project to build:

REQUIRED:
---------
1. vmmdll.lib       - MemProcFS/vmm library for DMA memory access
2. leechcore.lib    - LeechCore library for device communication  
3. FTD3XX.lib       - FTDI driver for USB3 FPGA communication

OPTIONAL:
---------
4. KMBoxNet.lib     - For network KMBox support (if using KMBox Net)

================================================================================
                         WHERE TO GET THESE FILES
================================================================================

1. vmmdll.lib & leechcore.lib:
   - Download from: https://github.com/ufrisk/MemProcFS/releases
   - Extract and find the .lib files in the x64 folder

2. FTD3XX.lib:
   - Download from: https://ftdichip.com/drivers/d3xx-drivers/
   - Find the .lib in the x64 folder after extraction

3. KMBoxNet.lib:
   - Comes with KMBox Net SDK (if you purchased KMBox Net device)

================================================================================
                         DLL FILES (Runtime)
================================================================================

Also place these DLL files in your output folder (bin/Debug or bin/Release):

- vmmdll.dll
- leechcore.dll  
- FTD3XX.dll

Without these DLLs, the program will not run even if it compiles successfully.

================================================================================
                         LINKER SETTINGS
================================================================================

In Visual Studio:
1. Right-click project -> Properties
2. Linker -> General -> Additional Library Directories
   Add: $(ProjectDir)libs

3. Linker -> Input -> Additional Dependencies
   Add: vmmdll.lib;leechcore.lib;FTD3XX.lib

================================================================================
