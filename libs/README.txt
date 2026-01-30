Zero Elite - Libraries Folder
==============================

This folder should contain the VMMDLL library file.

REQUIRED FILE:
--------------
vmmdll.lib - The Virtual Memory Manager DLL import library

HOW TO OBTAIN:
--------------
1. Download MemProcFS from: https://github.com/ufrisk/MemProcFS
2. Extract the package
3. Copy 'vmmdll.lib' from the lib folder to this directory

The vmmdll.lib is linked in the source code using:
#pragma comment(lib, "..\\libs\\vmmdll.lib")

ADDITIONAL FILES (Optional):
----------------------------
You may also want to copy the following files to your output directory:
- vmm.dll
- leechcore.dll
- FTD3XX.dll (for FPGA DMA devices)
- symsrv.dll (for symbol support)

NOTE:
-----
The placeholder vmmdll.lib file included here is empty and must be replaced
with the actual library file before building the project.

For more information, visit:
https://github.com/ufrisk/MemProcFS
