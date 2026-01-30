============================================
PROJECT ZERO - Required Libraries
============================================

Place the following library files in this folder:

1. vmmdll.lib
   - Required for DMA memory operations
   - Get from: https://github.com/ufrisk/MemProcFS/releases
   - Download the latest release and extract vmmdll.lib

2. FTD3XX.lib (Optional - for FTD3XX driver mode)
   - Required for maximum DMA speed
   - Get from: FTDI website or your DMA device vendor

3. leechcore.lib (Optional - for LeechCore support)
   - Enhanced DMA functionality
   - Get from: https://github.com/ufrisk/LeechCore/releases

============================================
Installation Steps:
============================================

1. Download MemProcFS from GitHub releases
2. Extract the archive
3. Copy vmmdll.lib to this folder
4. Also copy vmmdll.dll to your output/bin folder

============================================
Also needed in bin folder:
============================================

- vmmdll.dll
- leechcore.dll
- FTD3XX.dll (if using FTD3XX mode)

============================================
