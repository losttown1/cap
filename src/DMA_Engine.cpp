// DMA_Engine.cpp - Professional DMA Implementation
// Features: Scatter Reads, Pattern Scanner, Auto-Offset Updates

#include "DMA_Engine.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>

#if DMA_ENABLED
#include <Windows.h>
#include "vmmdll.h"
#pragma comment(lib, "vmmdll.lib")
static VMM_HANDLE g_VMMDLL = nullptr;
static DWORD g_DMA_PID = 0;
#endif

// ============================================================================
// GLOBAL OFFSETS
// ============================================================================
GameOffsets g_Offsets;

// ============================================================================
// STATIC MEMBERS
// ============================================================================
bool DMAEngine::s_Connected = false;
bool DMAEngine::s_SimulationMode = false;
uintptr_t DMAEngine::s_BaseAddress = 0;
size_t DMAEngine::s_ModuleSize = 0;
char DMAEngine::s_StatusText[64] = "OFFLINE";

bool PatternScanner::s_Scanned = false;
int PatternScanner::s_FoundCount = 0;

std::vector<PlayerData> PlayerManager::s_Players;
PlayerData PlayerManager::s_LocalPlayer;
std::mutex PlayerManager::s_Mutex;
bool PlayerManager::s_Initialized = false;
float PlayerManager::s_SimTime = 0;

// ============================================================================
// SCATTER READER IMPLEMENTATION
// ============================================================================
void ScatterReader::Add(uintptr_t addr, void* buf, size_t size)
{
    if (addr && buf && size > 0)
        m_Entries.push_back({addr, buf, size});
}

void ScatterReader::Execute()
{
#if DMA_ENABLED
    if (DMAEngine::IsConnected() && g_VMMDLL && g_DMA_PID)
    {
        // Use VMMDLL scatter for batched reads
        VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
        if (hScatter)
        {
            // Prepare all reads
            for (auto& entry : m_Entries)
            {
                VMMDLL_Scatter_Prepare(hScatter, entry.address, (DWORD)entry.size);
            }
            
            // Execute all at once
            VMMDLL_Scatter_Execute(hScatter);
            
            // Read results
            for (auto& entry : m_Entries)
            {
                VMMDLL_Scatter_Read(hScatter, entry.address, (DWORD)entry.size, (PBYTE)entry.buffer, nullptr);
            }
            
            VMMDLL_Scatter_CloseHandle(hScatter);
            m_Entries.clear();
            return;
        }
        
        // Fallback to individual reads
        for (auto& entry : m_Entries)
        {
            DMAEngine::ReadBuffer(entry.address, entry.buffer, entry.size);
        }
    }
    else
#endif
    {
        // Simulation - zero fill
        for (auto& entry : m_Entries)
        {
            memset(entry.buffer, 0, entry.size);
        }
    }
    m_Entries.clear();
}

void ScatterReader::Clear()
{
    m_Entries.clear();
}

// ============================================================================
// PATTERN SCANNER IMPLEMENTATION
// ============================================================================
uintptr_t PatternScanner::FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask)
{
    size_t patternLen = strlen(mask);
    if (patternLen == 0 || size < patternLen)
        return 0;
    
#if DMA_ENABLED
    if (DMAEngine::IsConnected())
    {
        // Read memory in chunks for scanning
        const size_t chunkSize = 0x10000;  // 64KB chunks
        std::vector<uint8_t> buffer(chunkSize + patternLen);
        
        for (size_t offset = 0; offset < size; offset += chunkSize)
        {
            size_t readSize = min(chunkSize + patternLen, size - offset);
            if (!DMAEngine::ReadBuffer(start + offset, buffer.data(), readSize))
                continue;
            
            // Scan chunk
            for (size_t i = 0; i < readSize - patternLen; i++)
            {
                bool found = true;
                for (size_t j = 0; j < patternLen && found; j++)
                {
                    if (mask[j] == 'x' && buffer[i + j] != (uint8_t)pattern[j])
                        found = false;
                }
                
                if (found)
                    return start + offset + i;
            }
        }
    }
#endif
    (void)start; (void)size; (void)pattern; (void)mask;
    return 0;
}

uintptr_t PatternScanner::ScanModule(const char* moduleName, const char* pattern, const char* mask)
{
    (void)moduleName;
    return FindPattern(DMAEngine::GetBaseAddress(), DMAEngine::s_ModuleSize, pattern, mask);
}

uintptr_t PatternScanner::ResolveRelative(uintptr_t instructionAddr, int offset, int instructionSize)
{
#if DMA_ENABLED
    if (DMAEngine::IsConnected())
    {
        int32_t relOffset = DMAEngine::Read<int32_t>(instructionAddr + offset);
        return instructionAddr + instructionSize + relOffset;
    }
#endif
    (void)instructionAddr; (void)offset; (void)instructionSize;
    return 0;
}

bool PatternScanner::UpdateAllOffsets()
{
    s_FoundCount = 0;
    s_Scanned = false;
    
#if DMA_ENABLED
    if (!DMAEngine::IsConnected())
    {
        // Use default offsets for simulation
        g_Offsets.PlayerBase = 0x17AA8E98;
        g_Offsets.ClientInfo = 0x17AA9000;
        g_Offsets.EntityList = 0x16D5B8D8;
        g_Offsets.ViewMatrix = 0x171B46A0;
        g_Offsets.Refdef = 0x17210718;
        s_Scanned = true;
        return true;
    }
    
    uintptr_t baseAddr = DMAEngine::GetBaseAddress();
    size_t moduleSize = DMAEngine::s_ModuleSize;
    
    // Scan for Player Base
    uintptr_t addr = FindPattern(baseAddr, moduleSize, Patterns::PlayerBase, Patterns::PlayerBaseMask);
    if (addr)
    {
        g_Offsets.PlayerBase = ResolveRelative(addr, 3, 7);
        if (g_Offsets.PlayerBase) s_FoundCount++;
    }
    
    // Scan for Client Info
    addr = FindPattern(baseAddr, moduleSize, Patterns::ClientInfo, Patterns::ClientInfoMask);
    if (addr)
    {
        g_Offsets.ClientInfo = ResolveRelative(addr, 3, 7);
        if (g_Offsets.ClientInfo) s_FoundCount++;
    }
    
    // Scan for Entity List
    addr = FindPattern(baseAddr, moduleSize, Patterns::EntityList, Patterns::EntityListMask);
    if (addr)
    {
        g_Offsets.EntityList = ResolveRelative(addr, 3, 7);
        if (g_Offsets.EntityList) s_FoundCount++;
    }
    
    // Scan for View Matrix
    addr = FindPattern(baseAddr, moduleSize, Patterns::ViewMatrix, Patterns::ViewMatrixMask);
    if (addr)
    {
        g_Offsets.ViewMatrix = ResolveRelative(addr, 3, 7);
        if (g_Offsets.ViewMatrix) s_FoundCount++;
    }
    
    // Scan for Refdef
    addr = FindPattern(baseAddr, moduleSize, Patterns::Refdef, Patterns::RefdefMask);
    if (addr)
    {
        g_Offsets.Refdef = ResolveRelative(addr, 3, 7);
        if (g_Offsets.Refdef) s_FoundCount++;
    }
    
    s_Scanned = true;
#endif
    
    return s_FoundCount > 0;
}

// ============================================================================
// DMA ENGINE IMPLEMENTATION
// ============================================================================
bool DMAEngine::Initialize()
{
    strcpy_s(s_StatusText, "INITIALIZING...");
    
#if DMA_ENABLED
    // Initialize FPGA DMA device
    LPCSTR args[] = {"", "-device", "fpga"};
    g_VMMDLL = VMMDLL_Initialize(3, args);
    
    if (!g_VMMDLL)
    {
        // Try alternative init
        LPCSTR args2[] = {"", "-device", "fpga://algo=0"};
        g_VMMDLL = VMMDLL_Initialize(3, args2);
    }
    
    if (!g_VMMDLL)
    {
        strcpy_s(s_StatusText, "FPGA ERROR");
        goto simulation;
    }
    
    // Find target process
    if (!VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)TARGET_PROCESS_NAME, &g_DMA_PID) || g_DMA_PID == 0)
    {
        strcpy_s(s_StatusText, "PROCESS NOT FOUND");
        goto simulation;
    }
    
    // Get base address
    s_BaseAddress = VMMDLL_ProcessGetModuleBaseW(g_VMMDLL, g_DMA_PID, TARGET_PROCESS_WIDE);
    if (s_BaseAddress == 0)
    {
        strcpy_s(s_StatusText, "BASE ADDR ERROR");
        goto simulation;
    }
    
    // Get module size for pattern scanning
    VMMDLL_MAP_MODULEENTRY moduleEntry = {};
    if (VMMDLL_Map_GetModuleFromNameW(g_VMMDLL, g_DMA_PID, TARGET_PROCESS_WIDE, &moduleEntry, 0))
    {
        s_ModuleSize = moduleEntry.cbImageSize;
    }
    else
    {
        s_ModuleSize = 0x5000000;  // Default 80MB
    }
    
    s_Connected = true;
    s_SimulationMode = false;
    strcpy_s(s_StatusText, "ONLINE");
    
    // Run pattern scanner
    PatternScanner::UpdateAllOffsets();
    
    return true;
    
simulation:
#endif
    // Simulation mode
    s_Connected = false;
    s_SimulationMode = true;
    s_BaseAddress = 0x140000000;
    s_ModuleSize = 0x5000000;
    strcpy_s(s_StatusText, "SIMULATION");
    
    // Use default offsets
    g_Offsets.PlayerBase = s_BaseAddress + 0x17AA8E98;
    g_Offsets.ClientInfo = s_BaseAddress + 0x17AA9000;
    g_Offsets.EntityList = s_BaseAddress + 0x16D5B8D8;
    
    return true;
}

void DMAEngine::Shutdown()
{
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        VMMDLL_Close(g_VMMDLL);
        g_VMMDLL = nullptr;
    }
#endif
    s_Connected = false;
    strcpy_s(s_StatusText, "OFFLINE");
}

bool DMAEngine::IsConnected()
{
    return s_Connected && !s_SimulationMode;
}

const char* DMAEngine::GetStatus()
{
    return s_StatusText;
}

template<typename T>
T DMAEngine::Read(uintptr_t address)
{
    T value = {};
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
    {
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)&value, sizeof(T), nullptr, VMMDLL_FLAG_NOCACHE);
    }
#else
    (void)address;
#endif
    return value;
}

bool DMAEngine::ReadBuffer(uintptr_t address, void* buffer, size_t size)
{
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
    {
        return VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)buffer, (DWORD)size, nullptr, VMMDLL_FLAG_NOCACHE);
    }
#endif
    (void)address; (void)buffer; (void)size;
    return false;
}

bool DMAEngine::ReadString(uintptr_t address, char* buffer, size_t maxLen)
{
    if (!buffer || maxLen == 0) return false;
    buffer[0] = 0;
    
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
    {
        if (VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)buffer, (DWORD)(maxLen - 1), nullptr, VMMDLL_FLAG_NOCACHE))
        {
            buffer[maxLen - 1] = 0;
            return true;
        }
    }
#endif
    (void)address;
    return false;
}

ScatterReader DMAEngine::CreateScatter()
{
    return ScatterReader();
}

void DMAEngine::ExecuteScatter(ScatterReader& scatter)
{
    scatter.Execute();
}

uintptr_t DMAEngine::GetBaseAddress()
{
    return s_BaseAddress;
}

uintptr_t DMAEngine::GetModuleBase(const wchar_t* moduleName)
{
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
    {
        return VMMDLL_ProcessGetModuleBaseW(g_VMMDLL, g_DMA_PID, moduleName);
    }
#endif
    (void)moduleName;
    return s_BaseAddress;
}

size_t DMAEngine::GetModuleSize(const wchar_t* moduleName)
{
    (void)moduleName;
    return s_ModuleSize;
}

// Template instantiations
template int8_t DMAEngine::Read<int8_t>(uintptr_t);
template uint8_t DMAEngine::Read<uint8_t>(uintptr_t);
template int16_t DMAEngine::Read<int16_t>(uintptr_t);
template uint16_t DMAEngine::Read<uint16_t>(uintptr_t);
template int32_t DMAEngine::Read<int32_t>(uintptr_t);
template uint32_t DMAEngine::Read<uint32_t>(uintptr_t);
template int64_t DMAEngine::Read<int64_t>(uintptr_t);
template uint64_t DMAEngine::Read<uint64_t>(uintptr_t);
template float DMAEngine::Read<float>(uintptr_t);
template double DMAEngine::Read<double>(uintptr_t);
template uintptr_t DMAEngine::Read<uintptr_t>(uintptr_t);
template Vec2 DMAEngine::Read<Vec2>(uintptr_t);
template Vec3 DMAEngine::Read<Vec3>(uintptr_t);

// ============================================================================
// PLAYER MANAGER IMPLEMENTATION
// ============================================================================
void PlayerManager::Initialize()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    s_Players.clear();
    s_Players.reserve(150);  // Max players
    
    // Create placeholder players
    for (int i = 0; i < 12; i++)
    {
        PlayerData p = {};
        p.valid = true;
        p.index = i;
        p.isEnemy = (i < 6);
        p.isAlive = true;
        p.health = 50 + rand() % 50;
        p.maxHealth = 100;
        p.team = p.isEnemy ? 1 : 2;
        sprintf_s(p.name, "Player_%d", i + 1);
        
        float angle = (float)i * (6.28318f / 12.0f);
        float dist = 30.0f + (float)(rand() % 100);
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.origin.z = 0;
        p.yaw = (float)(rand() % 360);
        p.distance = dist;
        
        s_Players.push_back(p);
    }
    
    s_LocalPlayer = {};
    s_LocalPlayer.valid = true;
    s_LocalPlayer.index = -1;
    s_LocalPlayer.isEnemy = false;
    s_LocalPlayer.health = 100;
    s_LocalPlayer.maxHealth = 100;
    strcpy_s(s_LocalPlayer.name, "LocalPlayer");
    
    s_Initialized = true;
    s_SimTime = 0;
}

void PlayerManager::Update()
{
    if (!s_Initialized)
        Initialize();
    
#if DMA_ENABLED
    if (DMAEngine::IsConnected())
    {
        RealUpdate();
    }
    else
#endif
    {
        SimulateUpdate();
    }
}

void PlayerManager::UpdateWithScatter()
{
    if (!s_Initialized)
        Initialize();
    
#if DMA_ENABLED
    if (DMAEngine::IsConnected() && g_Offsets.EntityList)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        
        ScatterReader scatter = DMAEngine::CreateScatter();
        
        // Prepare scatter reads for all entities
        struct EntityRawData {
            Vec3 pos;
            int health;
            int maxHealth;
            int team;
            float yaw;
            uint8_t valid;
        };
        
        std::vector<EntityRawData> rawData(s_Players.size());
        
        for (size_t i = 0; i < s_Players.size(); i++)
        {
            uintptr_t entityAddr = g_Offsets.EntityList + i * GameOffsets::EntitySize;
            
            scatter.Add(entityAddr + GameOffsets::EntityPos, &rawData[i].pos, sizeof(Vec3));
            scatter.Add(entityAddr + GameOffsets::EntityHealth, &rawData[i].health, sizeof(int));
            scatter.Add(entityAddr + GameOffsets::EntityMaxHealth, &rawData[i].maxHealth, sizeof(int));
            scatter.Add(entityAddr + GameOffsets::EntityTeam, &rawData[i].team, sizeof(int));
            scatter.Add(entityAddr + GameOffsets::EntityYaw, &rawData[i].yaw, sizeof(float));
            scatter.Add(entityAddr + GameOffsets::EntityValid, &rawData[i].valid, sizeof(uint8_t));
        }
        
        // Read local player
        Vec3 localPos;
        float localYaw;
        int localTeam;
        
        if (g_Offsets.ClientInfo)
        {
            uintptr_t clientBase = DMAEngine::Read<uintptr_t>(g_Offsets.ClientInfo);
            if (clientBase)
            {
                scatter.Add(clientBase + GameOffsets::EntityPos, &localPos, sizeof(Vec3));
                scatter.Add(clientBase + GameOffsets::EntityYaw, &localYaw, sizeof(float));
                scatter.Add(clientBase + GameOffsets::EntityTeam, &localTeam, sizeof(int));
            }
        }
        
        // Execute all reads at once
        scatter.Execute();
        
        // Process results
        s_LocalPlayer.origin = localPos;
        s_LocalPlayer.yaw = localYaw;
        s_LocalPlayer.team = localTeam;
        
        for (size_t i = 0; i < s_Players.size(); i++)
        {
            PlayerData& p = s_Players[i];
            EntityRawData& raw = rawData[i];
            
            p.valid = (raw.valid != 0 && raw.health > 0);
            p.origin = raw.pos;
            p.health = raw.health;
            p.maxHealth = raw.maxHealth > 0 ? raw.maxHealth : 100;
            p.team = raw.team;
            p.yaw = raw.yaw;
            p.isAlive = (raw.health > 0);
            p.isEnemy = (raw.team != localTeam);
            p.distance = (p.origin - localPos).Length();
        }
        
        return;
    }
#endif
    
    SimulateUpdate();
}

void PlayerManager::SimulateUpdate()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    s_SimTime += 0.016f;
    
    // Update local player
    s_LocalPlayer.yaw = fmodf(s_SimTime * 15.0f, 360.0f);
    s_LocalPlayer.origin = Vec3(0, 0, 0);
    
    // Animate players
    for (size_t i = 0; i < s_Players.size(); i++)
    {
        PlayerData& p = s_Players[i];
        
        float baseAngle = (float)i * (6.28318f / (float)s_Players.size());
        float angle = baseAngle + s_SimTime * 0.3f;
        float dist = 40.0f + sinf(s_SimTime * 0.5f + (float)i) * 30.0f;
        
        p.origin.x = cosf(angle) * dist;
        p.origin.y = sinf(angle) * dist;
        p.origin.z = sinf(s_SimTime + (float)i * 0.5f) * 5.0f;
        
        p.headPos = p.origin;
        p.headPos.z += 60.0f;
        
        p.yaw = fmodf(angle * 57.3f + 180.0f, 360.0f);
        p.distance = dist;
        
        // Health fluctuation
        float healthBase = 50.0f + sinf(s_SimTime * 0.2f + (float)i * 0.7f) * 40.0f;
        p.health = (int)healthBase;
        p.health = max(1, min(p.health, 100));
        p.isAlive = true;
        
        // Simulate visibility
        p.isVisible = ((int)(s_SimTime * 2 + i) % 3) != 0;
    }
}

void PlayerManager::RealUpdate()
{
    UpdateWithScatter();
}

std::vector<PlayerData>& PlayerManager::GetPlayers()
{
    return s_Players;
}

PlayerData& PlayerManager::GetLocalPlayer()
{
    return s_LocalPlayer;
}

int PlayerManager::GetAliveCount()
{
    int count = 0;
    for (const auto& p : s_Players)
        if (p.valid && p.isAlive) count++;
    return count;
}

int PlayerManager::GetEnemyCount()
{
    int count = 0;
    for (const auto& p : s_Players)
        if (p.valid && p.isAlive && p.isEnemy) count++;
    return count;
}

// ============================================================================
// WORLD TO SCREEN
// ============================================================================
static Matrix4x4 g_ViewMatrix;

bool WorldToScreen(const Vec3& worldPos, Vec2& screenPos, int screenW, int screenH)
{
    // Simple orthographic projection for demo
    // Real implementation would use game's view matrix
    
    float w = g_ViewMatrix.m[3][0] * worldPos.x + 
              g_ViewMatrix.m[3][1] * worldPos.y + 
              g_ViewMatrix.m[3][2] * worldPos.z + 
              g_ViewMatrix.m[3][3];
    
    if (w < 0.01f)
    {
        screenPos = Vec2(0, 0);
        return false;
    }
    
    float x = g_ViewMatrix.m[0][0] * worldPos.x + 
              g_ViewMatrix.m[0][1] * worldPos.y + 
              g_ViewMatrix.m[0][2] * worldPos.z + 
              g_ViewMatrix.m[0][3];
    
    float y = g_ViewMatrix.m[1][0] * worldPos.x + 
              g_ViewMatrix.m[1][1] * worldPos.y + 
              g_ViewMatrix.m[1][2] * worldPos.z + 
              g_ViewMatrix.m[1][3];
    
    screenPos.x = (screenW / 2.0f) * (1.0f + x / w);
    screenPos.y = (screenH / 2.0f) * (1.0f - y / w);
    
    return true;
}

bool WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float localYaw,
                  Vec2& radarPos, float radarCX, float radarCY, float radarScale)
{
    Vec3 delta = worldPos - localPos;
    
    float yawRad = localYaw * 3.14159265f / 180.0f;
    float cosY = cosf(-yawRad);
    float sinY = sinf(-yawRad);
    
    float rotX = delta.x * cosY - delta.y * sinY;
    float rotY = delta.x * sinY + delta.y * cosY;
    
    radarPos.x = radarCX + rotX * radarScale;
    radarPos.y = radarCY - rotY * radarScale;
    
    return true;
}

// ============================================================================
// AIMBOT HELPERS
// ============================================================================
float GetFOVTo(const Vec2& screenCenter, const Vec2& targetScreen)
{
    float dx = targetScreen.x - screenCenter.x;
    float dy = targetScreen.y - screenCenter.y;
    return sqrtf(dx * dx + dy * dy);
}

Vec3 CalcAngle(const Vec3& src, const Vec3& dst)
{
    Vec3 delta = dst - src;
    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y);
    
    Vec3 angles;
    angles.x = -atan2f(delta.z, hyp) * (180.0f / 3.14159265f);  // Pitch
    angles.y = atan2f(delta.y, delta.x) * (180.0f / 3.14159265f);  // Yaw
    angles.z = 0;
    
    return angles;
}

void SmoothAngle(Vec3& currentAngle, const Vec3& targetAngle, float smoothness)
{
    if (smoothness <= 0) smoothness = 1;
    
    Vec3 delta = targetAngle - currentAngle;
    
    // Normalize yaw delta
    while (delta.y > 180.0f) delta.y -= 360.0f;
    while (delta.y < -180.0f) delta.y += 360.0f;
    
    currentAngle.x += delta.x / smoothness;
    currentAngle.y += delta.y / smoothness;
}
