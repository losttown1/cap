// VMMDLL - Virtual Memory Manager DLL Interface
// Zero Elite - DMA Memory Interface Header

#ifndef VMMDLL_H
#define VMMDLL_H

#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// VMM Handle Types
typedef HANDLE VMM_HANDLE;
typedef DWORD VMMDLL_PID;

// Initialization Flags
#define VMMDLL_FLAG_NO_REFRESH          0x0001
#define VMMDLL_FLAG_NOPARSEPHYSICAL     0x0002
#define VMMDLL_FLAG_NOPARSEALL          0x0004

// Memory Read/Write Flags
#define VMMDLL_FLAG_NOCACHE             0x0001
#define VMMDLL_FLAG_ZEROPAD_ON_FAIL     0x0002
#define VMMDLL_FLAG_FORCECACHE_READ     0x0008
#define VMMDLL_FLAG_NOPAGING            0x0010
#define VMMDLL_FLAG_NOPAGING_IO         0x0020

// Process Information
typedef struct tdVMMDLL_PROCESS_INFORMATION {
    ULONG64 magic;
    WORD wVersion;
    WORD wSize;
    DWORD tpMemoryModel;
    DWORD tpSystem;
    BOOL fUserOnly;
    DWORD dwPID;
    DWORD dwPPID;
    DWORD dwState;
    CHAR szName[16];
    CHAR szNameLong[64];
    ULONG64 paDTB;
    ULONG64 paDTB_UserOpt;
    union { ULONG64 vaEPROCESS; ULONG64 vaPEB32_64; };
    ULONG64 vaPEB;
    BOOL fWow64;
    DWORD vaPEB32;
    DWORD dwSessionId;
    ULONG64 qwLUID;
    CHAR szSID[96];
} VMMDLL_PROCESS_INFORMATION, *PVMMDLL_PROCESS_INFORMATION;

// Module Information
typedef struct tdVMMDLL_MAP_MODULEENTRY {
    ULONG64 vaBase;
    ULONG64 vaEntry;
    DWORD cbImageSize;
    BOOL fWoW64;
    CHAR uszText[32];
    CHAR uszFullName[256];
    DWORD tp;
    DWORD cbFileSizeRaw;
} VMMDLL_MAP_MODULEENTRY, *PVMMDLL_MAP_MODULEENTRY;

// Memory Region Information
typedef struct tdVMMDLL_MAP_VADENTRY {
    ULONG64 vaStart;
    ULONG64 vaEnd;
    ULONG64 vaVad;
    DWORD u0;
    DWORD u1;
    DWORD u2;
    DWORD cbPrototypePte;
    DWORD CommitCharge;
    BOOL fMemCommit;
    BOOL fFile;
    BOOL fPageFile;
    BOOL fImage;
    CHAR uszText[260];
} VMMDLL_MAP_VADENTRY, *PVMMDLL_MAP_VADENTRY;

// Scatter Memory Read/Write
typedef struct tdVMMDLL_SCATTER_HANDLE {
    DWORD dwVersion;
    DWORD cMEMs;
    PVOID pMEMs;
} VMMDLL_SCATTER_HANDLE, *PVMMDLL_SCATTER_HANDLE;

//-----------------------------------------------------------------------------
// VMMDLL CORE FUNCTIONALITY
//-----------------------------------------------------------------------------

// Initialize VMMDLL
// Returns: VMM_HANDLE on success, NULL on failure
__declspec(dllimport) VMM_HANDLE VMMDLL_Initialize(
    _In_ DWORD argc,
    _In_ LPSTR argv[]
);

// Close VMMDLL
__declspec(dllimport) VOID VMMDLL_Close(
    _In_ VMM_HANDLE hVMM
);

// Close All VMMDLL handles
__declspec(dllimport) VOID VMMDLL_CloseAll();

//-----------------------------------------------------------------------------
// VMMDLL PROCESS FUNCTIONALITY
//-----------------------------------------------------------------------------

// Get PID from process name
__declspec(dllimport) BOOL VMMDLL_PidGetFromName(
    _In_ VMM_HANDLE hVMM,
    _In_ LPSTR szProcName,
    _Out_ PDWORD pdwPID
);

// Get list of all PIDs
__declspec(dllimport) BOOL VMMDLL_PidList(
    _In_ VMM_HANDLE hVMM,
    _Out_opt_ PDWORD pPIDs,
    _Inout_ PULONG64 pcPIDs
);

// Get process information
__declspec(dllimport) BOOL VMMDLL_ProcessGetInformation(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _Inout_opt_ PVMMDLL_PROCESS_INFORMATION pInfo,
    _Inout_ PDWORD pcbInfo
);

// Get module base address
__declspec(dllimport) ULONG64 VMMDLL_ProcessGetModuleBase(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _In_ LPSTR szModuleName
);

// Get procedure address
__declspec(dllimport) ULONG64 VMMDLL_ProcessGetProcAddress(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _In_ LPSTR szModuleName,
    _In_ LPSTR szFunctionName
);

//-----------------------------------------------------------------------------
// VMMDLL MEMORY READ/WRITE FUNCTIONALITY
//-----------------------------------------------------------------------------

// Read memory from process
__declspec(dllimport) BOOL VMMDLL_MemRead(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _In_ ULONG64 qwA,
    _Out_ PBYTE pb,
    _In_ DWORD cb
);

// Read memory from process with flags
__declspec(dllimport) BOOL VMMDLL_MemReadEx(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _In_ ULONG64 qwA,
    _Out_ PBYTE pb,
    _In_ DWORD cb,
    _Out_opt_ PDWORD pcbReadOpt,
    _In_ ULONG64 flags
);

// Write memory to process
__declspec(dllimport) BOOL VMMDLL_MemWrite(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _In_ ULONG64 qwA,
    _In_ PBYTE pb,
    _In_ DWORD cb
);

// Read physical memory
__declspec(dllimport) BOOL VMMDLL_MemReadPhysical(
    _In_ VMM_HANDLE hVMM,
    _In_ ULONG64 qwPA,
    _Out_ PBYTE pb,
    _In_ DWORD cb
);

// Write physical memory
__declspec(dllimport) BOOL VMMDLL_MemWritePhysical(
    _In_ VMM_HANDLE hVMM,
    _In_ ULONG64 qwPA,
    _In_ PBYTE pb,
    _In_ DWORD cb
);

//-----------------------------------------------------------------------------
// VMMDLL SCATTER READ/WRITE FUNCTIONALITY
//-----------------------------------------------------------------------------

// Initialize scatter handle
__declspec(dllimport) PVMMDLL_SCATTER_HANDLE VMMDLL_Scatter_Initialize(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _In_ DWORD dwFlags
);

// Prepare scatter read
__declspec(dllimport) BOOL VMMDLL_Scatter_Prepare(
    _In_ PVMMDLL_SCATTER_HANDLE hS,
    _In_ ULONG64 va,
    _In_ DWORD cb
);

// Prepare scatter read with buffer
__declspec(dllimport) BOOL VMMDLL_Scatter_PrepareEx(
    _In_ PVMMDLL_SCATTER_HANDLE hS,
    _In_ ULONG64 va,
    _In_ DWORD cb,
    _Out_ PBYTE pb,
    _Out_opt_ PDWORD pcbRead
);

// Execute scatter read
__declspec(dllimport) BOOL VMMDLL_Scatter_Execute(
    _In_ PVMMDLL_SCATTER_HANDLE hS
);

// Read scatter result
__declspec(dllimport) BOOL VMMDLL_Scatter_Read(
    _In_ PVMMDLL_SCATTER_HANDLE hS,
    _In_ ULONG64 va,
    _In_ DWORD cb,
    _Out_ PBYTE pb,
    _Out_opt_ PDWORD pcbRead
);

// Clear scatter handle
__declspec(dllimport) BOOL VMMDLL_Scatter_Clear(
    _In_ PVMMDLL_SCATTER_HANDLE hS,
    _In_ DWORD dwPID,
    _In_ DWORD dwFlags
);

// Close scatter handle
__declspec(dllimport) VOID VMMDLL_Scatter_CloseHandle(
    _In_ PVMMDLL_SCATTER_HANDLE hS
);

//-----------------------------------------------------------------------------
// VMMDLL UTILITY FUNCTIONS
//-----------------------------------------------------------------------------

// Virtual to Physical address translation
__declspec(dllimport) BOOL VMMDLL_MemVirt2Phys(
    _In_ VMM_HANDLE hVMM,
    _In_ DWORD dwPID,
    _In_ ULONG64 qwVA,
    _Out_ PULONG64 pqwPA
);

// Get physical memory map
__declspec(dllimport) BOOL VMMDLL_Map_GetPhysMem(
    _In_ VMM_HANDLE hVMM,
    _Out_ PVOID* ppPhysMemMap,
    _Out_ PDWORD pcPhysMemMap
);

#ifdef __cplusplus
}
#endif

#endif // VMMDLL_H
