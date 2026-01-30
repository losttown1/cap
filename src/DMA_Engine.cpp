// DMA_Engine.cpp - DMA Implementation with Scatter Reads
#include "DMA_Engine.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <ctime>

#if DMA_ENABLED
#include <vmmdll.h>
#pragma comment(lib, "vmmdll.lib")
static VMM_HANDLE g_VMMDLL = nullptr;
static DWORD g_DMA_PID = 0;
#endif

// ============================================================================
// STATIC MEMBERS
// ============================================================================
bool DMAEngine::s_Connected = false;
uintptr_t DMAEngine::s_BaseAddress = 0;
uint32_t DMAEngine::s_ProcessId = 0;

std::vector<PlayerData> PlayerManager::s_Players;
PlayerData PlayerManager::s_LocalPlayer;

// ============================================================================
// SCATTER READER
// ============================================================================
void ScatterReader::AddRead(uintptr_t addr, void* buf, size_t size)
{
    requests.push_back({addr, buf, size});
}

void ScatterReader::Execute()
{
#if DMA_ENABLED
    // Real DMA scatter implementation would batch all reads here
    // VMMDLL_Scatter_Execute is used for this
    for (auto& req : requests)
    {
        if (req.buffer && g_VMMDLL)
        {
            VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, req.address, 
                            (PBYTE)req.buffer, (DWORD)req.size, nullptr, 0);
        }
    }
#else
    // Simulation - fills with zeros
    for (auto& req : requests)
    {
        if (req.buffer)
            memset(req.buffer, 0, req.size);
    }
#endif
}

void ScatterReader::Clear()
{
    requests.clear();
}

// ============================================================================
// DMA ENGINE
// ============================================================================
bool DMAEngine::Initialize()
{
#if DMA_ENABLED
    // Initialize FPGA connection
    g_VMMDLL = VMMDLL_Initialize(3, (LPCSTR[]){"", "-device", "fpga"});
    if (!g_VMMDLL)
    {
        // Try backup FPGA init
        g_VMMDLL = VMMDLL_Initialize(3, (LPCSTR[]){"", "-device", "fpga://algo=0"});
        if (!g_VMMDLL)
        {
            OutputDebugStringA("[ZERO] Failed to connect to FPGA device\n");
            goto simulation_mode;
        }
    }
    
    OutputDebugStringA("[ZERO] FPGA Connected!\n");
    
    // Find target process
    if (!VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)TARGET_PROCESS_NAME, &g_DMA_PID) || g_DMA_PID == 0)
    {
        OutputDebugStringA("[ZERO] Target process not found\n");
        goto simulation_mode;
    }
    
    // Get base address
    s_BaseAddress = VMMDLL_ProcessGetModuleBase(g_VMMDLL, g_DMA_PID, (LPWSTR)L"cod.exe");
    if (s_BaseAddress == 0)
    {
        OutputDebugStringA("[ZERO] Failed to get base address\n");
        goto simulation_mode;
    }
    
    s_ProcessId = g_DMA_PID;
    s_Connected = true;
    return true;
    
simulation_mode:
    OutputDebugStringA("[ZERO] Running in simulation mode\n");
#endif
    
    // Simulation mode
    s_Connected = false;
    s_BaseAddress = 0x140000000;  // Fake base
    s_ProcessId = 12345;
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
}

bool DMAEngine::IsConnected()
{
    return s_Connected;
}

template<typename T>
T DMAEngine::Read(uintptr_t address)
{
    T value = {};
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)&value, sizeof(T), nullptr, 0);
    }
#else
    (void)address;
#endif
    return value;
}

bool DMAEngine::ReadBuffer(uintptr_t address, void* buffer, size_t size)
{
#if DMA_ENABLED
    if (g_VMMDLL)
    {
        return VMMDLL_MemReadEx(g_VMMDLL, g_DMA_PID, address, (PBYTE)buffer, (DWORD)size, nullptr, 0);
    }
#else
    (void)address; (void)buffer; (void)size;
#endif
    return false;
}

ScatterReader DMAEngine::CreateScatter()
{
    return ScatterReader();
}

uintptr_t DMAEngine::GetBaseAddress()
{
    return s_BaseAddress;
}

uint32_t DMAEngine::GetProcessId()
{
    return s_ProcessId;
}

// ============================================================================
// PATTERN SCANNER
// ============================================================================
uintptr_t PatternScanner::Find(const char* module, const char* pattern, const char* mask)
{
    (void)module; (void)pattern; (void)mask;
    
#if DMA_ENABLED
    // Real implementation would:
    // 1. Read module memory
    // 2. Scan for byte pattern
    // 3. Return address of match
    
    // Example pseudo-code:
    // uintptr_t moduleBase = DMAEngine::GetBaseAddress();
    // size_t moduleSize = GetModuleSize(module);
    // std::vector<uint8_t> buffer(moduleSize);
    // DMAEngine::ReadBuffer(moduleBase, buffer.data(), moduleSize);
    // 
    // for (size_t i = 0; i < moduleSize - strlen(mask); i++)
    // {
    //     bool found = true;
    //     for (size_t j = 0; mask[j]; j++)
    //     {
    //         if (mask[j] == 'x' && buffer[i+j] != (uint8_t)pattern[j])
    //         {
    //             found = false;
    //             break;
    //         }
    //     }
    //     if (found) return moduleBase + i;
    // }
#endif
    
    return 0;
}

bool PatternScanner::UpdateAllOffsets()
{
    // Example patterns (need real BO6 signatures)
    // These would auto-update when game patches
    
    // Pattern patterns[] = {
    //     { "ClientInfo", "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74", "xxx????xxxx", 3 },
    //     { "EntityList", "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x85\xC0", "xxx????x????xxx", 3 },
    //     { "ViewMatrix", "\x48\x8D\x15\x00\x00\x00\x00\x48\x8B\xC8\xE8", "xxx????xxxx", 3 },
    // };
    
    // for (auto& p : patterns)
    // {
    //     uintptr_t addr = Find("cod.exe", p.pattern, p.mask);
    //     if (addr != 0)
    //     {
    //         // Read relative offset and calculate absolute
    //         int32_t offset = DMAEngine::Read<int32_t>(addr + p.offset);
    //         uintptr_t final = addr + p.offset + 4 + offset;
    //         // Store in offset table
    //     }
    // }
    
    return true;
}

// ============================================================================
// PLAYER MANAGER
// ============================================================================
void PlayerManager::Update()
{
    static bool initialized = false;
    static float simTime = 0;
    
    if (!initialized)
    {
        srand((unsigned)time(nullptr));
        s_Players.clear();
        
        // Create simulated players
        for (int i = 0; i < 12; i++)
        {
            PlayerData p = {};
            p.valid = true;
            p.isEnemy = (i < 6);
            p.isAlive = true;
            p.health = 30 + rand() % 70;
            p.maxHealth = 100;
            p.team = p.isEnemy ? 1 : 2;
            sprintf_s(p.name, "%s_%d", p.isEnemy ? "Enemy" : "Team", i + 1);
            
            float angle = (float)i * (6.28318f / 12.0f);
            p.worldPos.x = cosf(angle) * (50.0f + rand() % 100);
            p.worldPos.y = sinf(angle) * (50.0f + rand() % 100);
            p.worldPos.z = 0;
            p.yaw = (float)(rand() % 360);
            p.distance = p.worldPos.Length();
            
            s_Players.push_back(p);
        }
        
        s_LocalPlayer = {};
        s_LocalPlayer.valid = true;
        s_LocalPlayer.isEnemy = false;
        s_LocalPlayer.health = 100;
        s_LocalPlayer.yaw = 0;
        
        initialized = true;
    }
    
    // Update simulation
    simTime += 0.016f;
    
#if DMA_ENABLED
    if (DMAEngine::IsConnected())
    {
        // Real DMA read implementation would go here
        // Using scatter reads for efficiency:
        
        ScatterReader scatter = DMAEngine::CreateScatter();
        
        // Read client info
        uintptr_t clientInfo = DMAEngine::Read<uintptr_t>(
            DMAEngine::GetBaseAddress() + BO6Offsets::ClientInfo);
        
        if (clientInfo)
        {
            uintptr_t clientBase = DMAEngine::Read<uintptr_t>(
                clientInfo + BO6Offsets::ClientInfoBase);
            
            // Read local player data
            scatter.AddRead(clientBase + BO6Offsets::EntityPos, 
                           &s_LocalPlayer.worldPos, sizeof(Vec3));
            scatter.AddRead(clientBase + BO6Offsets::EntityYaw,
                           &s_LocalPlayer.yaw, sizeof(float));
            
            // Read entity list
            uintptr_t entityList = DMAEngine::Read<uintptr_t>(
                DMAEngine::GetBaseAddress() + BO6Offsets::EntityList);
            
            for (size_t i = 0; i < s_Players.size() && i < 150; i++)
            {
                uintptr_t entity = entityList + i * 0x568;  // Entity stride
                
                scatter.AddRead(entity + BO6Offsets::EntityPos,
                               &s_Players[i].worldPos, sizeof(Vec3));
                scatter.AddRead(entity + BO6Offsets::EntityHealth,
                               &s_Players[i].health, sizeof(int));
                scatter.AddRead(entity + BO6Offsets::EntityYaw,
                               &s_Players[i].yaw, sizeof(float));
                scatter.AddRead(entity + BO6Offsets::EntityTeam,
                               &s_Players[i].team, sizeof(int));
            }
            
            // Execute all reads at once
            scatter.Execute();
            
            // Post-process
            for (auto& p : s_Players)
            {
                p.distance = (p.worldPos - s_LocalPlayer.worldPos).Length();
                p.isEnemy = (p.team != s_LocalPlayer.team);
                p.isAlive = (p.health > 0);
                p.valid = p.isAlive;
            }
        }
        
        return;
    }
#endif
    
    // Simulation mode - animate players
    s_LocalPlayer.yaw = fmodf(simTime * 20.0f, 360.0f);
    
    for (size_t i = 0; i < s_Players.size(); i++)
    {
        PlayerData& p = s_Players[i];
        
        float angle = simTime * 0.3f + (float)i * 0.5f;
        float dist = 50.0f + sinf(simTime + i) * 30.0f;
        
        p.worldPos.x = cosf(angle) * dist;
        p.worldPos.y = sinf(angle) * dist;
        p.distance = dist;
        p.yaw = fmodf(angle * 57.3f, 360.0f);
        p.health = 30 + (int)(sinf(simTime * 0.5f + i) * 35 + 35);
        p.isAlive = (p.health > 0);
    }
}

std::vector<PlayerData>& PlayerManager::GetPlayers()
{
    return s_Players;
}

PlayerData& PlayerManager::GetLocalPlayer()
{
    return s_LocalPlayer;
}

int PlayerManager::GetPlayerCount()
{
    int count = 0;
    for (const auto& p : s_Players)
    {
        if (p.valid && p.isAlive)
            count++;
    }
    return count;
}

// Explicit template instantiations
template int DMAEngine::Read<int>(uintptr_t);
template float DMAEngine::Read<float>(uintptr_t);
template uintptr_t DMAEngine::Read<uintptr_t>(uintptr_t);
template uint64_t DMAEngine::Read<uint64_t>(uintptr_t);
template uint32_t DMAEngine::Read<uint32_t>(uintptr_t);
