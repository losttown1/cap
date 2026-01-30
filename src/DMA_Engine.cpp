// DMA_Engine.cpp - Professional DMA Implementation
// Features: Scatter Registry, Pattern Scanner, Config-driven Init, Map Textures

#include "DMA_Engine.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <sstream>

#if DMA_ENABLED
#include <Windows.h>
#include "vmmdll.h"
#pragma comment(lib, "vmmdll.lib")
static VMM_HANDLE g_VMMDLL = nullptr;
static DWORD g_DMA_PID = 0;
#else
#include <Windows.h>
#endif

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
DMAConfig g_Config;
GameOffsets g_Offsets;
ScatterReadRegistry g_ScatterRegistry;

// ============================================================================
// STATIC MEMBERS
// ============================================================================
bool DMAEngine::s_Connected = false;
bool DMAEngine::s_SimulationMode = false;
uintptr_t DMAEngine::s_BaseAddress = 0;
size_t DMAEngine::s_ModuleSize = 0;
char DMAEngine::s_StatusText[64] = "OFFLINE";
char DMAEngine::s_DeviceInfo[128] = "No device";

bool PatternScanner::s_Scanned = false;
int PatternScanner::s_FoundCount = 0;

std::vector<PlayerData> PlayerManager::s_Players;
PlayerData PlayerManager::s_LocalPlayer;
std::mutex PlayerManager::s_Mutex;
bool PlayerManager::s_Initialized = false;
float PlayerManager::s_SimTime = 0;

MapInfo MapTextureManager::s_CurrentMap;
std::unordered_map<std::string, MapInfo> MapTextureManager::s_MapDatabase;

// ============================================================================
// CONFIG FILE HANDLING
// ============================================================================
static void TrimString(char* str)
{
    char* end;
    while (*str == ' ' || *str == '\t') str++;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end + 1) = 0;
}

bool LoadConfig(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        CreateDefaultConfig(filename);
        file.open(filename);
        if (!file.is_open()) return false;
    }
    
    std::string line;
    std::string section;
    
    while (std::getline(file, line))
    {
        // Skip comments and empty lines
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        
        // Section header
        if (line[0] == '[')
        {
            size_t end = line.find(']');
            if (end != std::string::npos)
                section = line.substr(1, end - 1);
            continue;
        }
        
        // Key=Value
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        
        // Trim whitespace
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) value.erase(0, 1);
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) value.pop_back();
        
        // Parse values
        if (section == "Device")
        {
            if (key == "Type") strncpy_s(g_Config.deviceType, value.c_str(), 31);
            else if (key == "Arg") strncpy_s(g_Config.deviceArg, value.c_str(), 63);
            else if (key == "Algorithm") strncpy_s(g_Config.deviceAlgo, value.c_str(), 15);
            else if (key == "UseCustomPCIe") g_Config.useCustomPCIe = (value == "1" || value == "true");
            else if (key == "CustomPCIeID") strncpy_s(g_Config.customPCIeID, value.c_str(), 31);
        }
        else if (section == "Target")
        {
            if (key == "ProcessName")
            {
                strncpy_s(g_Config.processName, value.c_str(), 63);
                mbstowcs_s(nullptr, g_Config.processNameW, g_Config.processName, 63);
            }
        }
        else if (section == "Performance")
        {
            if (key == "ScatterBatchSize") g_Config.scatterBatchSize = std::stoi(value);
            else if (key == "UpdateRateHz") g_Config.updateRateHz = std::stoi(value);
            else if (key == "UseScatterRegistry") g_Config.useScatterRegistry = (value == "1" || value == "true");
        }
        else if (section == "Map")
        {
            if (key == "ImagePath") strncpy_s(g_Config.mapImagePath, value.c_str(), 255);
            else if (key == "ScaleX") g_Config.mapScaleX = std::stof(value);
            else if (key == "ScaleY") g_Config.mapScaleY = std::stof(value);
            else if (key == "OffsetX") g_Config.mapOffsetX = std::stof(value);
            else if (key == "OffsetY") g_Config.mapOffsetY = std::stof(value);
            else if (key == "Rotation") g_Config.mapRotation = std::stof(value);
        }
        else if (section == "Debug")
        {
            if (key == "DebugMode") g_Config.debugMode = (value == "1" || value == "true");
            else if (key == "LogReads") g_Config.logReads = (value == "1" || value == "true");
        }
    }
    
    return true;
}

bool SaveConfig(const char* filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "; PROJECT ZERO - Configuration File\n";
    file << "; Hardware-ID Masking: Change device settings to avoid detection\n\n";
    
    file << "[Device]\n";
    file << "; Device type: fpga, usb3380, etc.\n";
    file << "Type=" << g_Config.deviceType << "\n";
    file << "; Additional device arguments\n";
    file << "Arg=" << g_Config.deviceArg << "\n";
    file << "; Algorithm: 0, 1, 2 (varies by device)\n";
    file << "Algorithm=" << g_Config.deviceAlgo << "\n";
    file << "; Custom PCIe ID for Hardware-ID masking\n";
    file << "UseCustomPCIe=" << (g_Config.useCustomPCIe ? "1" : "0") << "\n";
    file << "CustomPCIeID=" << g_Config.customPCIeID << "\n\n";
    
    file << "[Target]\n";
    file << "; Target process name (cod.exe for Call of Duty)\n";
    file << "ProcessName=" << g_Config.processName << "\n\n";
    
    file << "[Performance]\n";
    file << "; Number of reads to batch together\n";
    file << "ScatterBatchSize=" << g_Config.scatterBatchSize << "\n";
    file << "; Update rate in Hz (60-240)\n";
    file << "UpdateRateHz=" << g_Config.updateRateHz << "\n";
    file << "; Use Scatter Registry for maximum performance\n";
    file << "UseScatterRegistry=" << (g_Config.useScatterRegistry ? "1" : "0") << "\n\n";
    
    file << "[Map]\n";
    file << "; Path to map background image (PNG/JPG)\n";
    file << "ImagePath=" << g_Config.mapImagePath << "\n";
    file << "; Scale and offset for coordinate mapping\n";
    file << "ScaleX=" << g_Config.mapScaleX << "\n";
    file << "ScaleY=" << g_Config.mapScaleY << "\n";
    file << "OffsetX=" << g_Config.mapOffsetX << "\n";
    file << "OffsetY=" << g_Config.mapOffsetY << "\n";
    file << "Rotation=" << g_Config.mapRotation << "\n\n";
    
    file << "[Debug]\n";
    file << "DebugMode=" << (g_Config.debugMode ? "1" : "0") << "\n";
    file << "LogReads=" << (g_Config.logReads ? "1" : "0") << "\n";
    
    return true;
}

void CreateDefaultConfig(const char* filename)
{
    // Set defaults
    strcpy_s(g_Config.deviceType, "fpga");
    strcpy_s(g_Config.deviceArg, "");
    strcpy_s(g_Config.deviceAlgo, "0");
    g_Config.useCustomPCIe = false;
    strcpy_s(g_Config.customPCIeID, "");
    
    strcpy_s(g_Config.processName, "cod.exe");
    wcscpy_s(g_Config.processNameW, L"cod.exe");
    
    g_Config.scatterBatchSize = 128;
    g_Config.updateRateHz = 120;
    g_Config.useScatterRegistry = true;
    
    strcpy_s(g_Config.mapImagePath, "");
    g_Config.mapScaleX = 1.0f;
    g_Config.mapScaleY = 1.0f;
    g_Config.mapOffsetX = 0.0f;
    g_Config.mapOffsetY = 0.0f;
    g_Config.mapRotation = 0.0f;
    
    g_Config.debugMode = false;
    g_Config.logReads = false;
    
    SaveConfig(filename);
}

// ============================================================================
// SCATTER READ REGISTRY IMPLEMENTATION
// ============================================================================
void ScatterReadRegistry::RegisterPlayerData(int playerIndex, uintptr_t baseAddr)
{
    if (playerIndex < 0 || baseAddr == 0) return;
    
    // Ensure buffer exists
    while ((int)m_PlayerBuffers.size() <= playerIndex)
    {
        m_PlayerBuffers.push_back({});
    }
    
    PlayerRawData& buf = m_PlayerBuffers[playerIndex];
    
    // Register all player data reads
    m_Entries.push_back({baseAddr + GameOffsets::EntityPos, &buf.position, sizeof(Vec3), ScatterDataType::PLAYER_POSITION, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityHealth, &buf.health, sizeof(int), ScatterDataType::PLAYER_HEALTH, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityMaxHealth, &buf.maxHealth, sizeof(int), ScatterDataType::PLAYER_HEALTH, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityTeam, &buf.team, sizeof(int), ScatterDataType::PLAYER_TEAM, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityYaw, &buf.yaw, sizeof(float), ScatterDataType::PLAYER_YAW, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityValid, &buf.valid, sizeof(uint8_t), ScatterDataType::PLAYER_VALID, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityStance, &buf.stance, sizeof(uint8_t), ScatterDataType::PLAYER_STANCE, playerIndex});
    m_Entries.push_back({baseAddr + GameOffsets::EntityName, &buf.name, 32, ScatterDataType::PLAYER_NAME, playerIndex});
}

void ScatterReadRegistry::RegisterLocalPlayer(uintptr_t baseAddr)
{
    if (baseAddr == 0) return;
    
    m_Entries.push_back({baseAddr + GameOffsets::EntityPos, &m_LocalPosition, sizeof(Vec3), ScatterDataType::LOCAL_POSITION, -1});
    m_Entries.push_back({baseAddr + GameOffsets::EntityYaw, &m_LocalYaw, sizeof(float), ScatterDataType::LOCAL_YAW, -1});
    m_Entries.push_back({baseAddr + GameOffsets::EntityTeam, &m_LocalTeam, sizeof(int), ScatterDataType::PLAYER_TEAM, -1});
}

void ScatterReadRegistry::RegisterViewMatrix(uintptr_t addr)
{
    if (addr == 0) return;
    m_Entries.push_back({addr, &m_ViewMatrix, sizeof(Matrix4x4), ScatterDataType::VIEW_MATRIX, -1});
}

void ScatterReadRegistry::RegisterCustomRead(uintptr_t addr, void* buf, size_t size)
{
    if (addr == 0 || buf == nullptr || size == 0) return;
    m_Entries.push_back({addr, buf, size, ScatterDataType::CUSTOM, -1});
}

void ScatterReadRegistry::ExecuteAll()
{
    if (m_Entries.empty()) return;
    
    m_TransactionCount++;
    
#if DMA_ENABLED
    if (DMAEngine::IsConnected())
    {
        DMAEngine::ExecuteScatter(m_Entries);
        
        // Copy data to PlayerManager
        std::lock_guard<std::mutex> lock(PlayerManager::GetMutex());
        auto& players = PlayerManager::GetPlayers();
        auto& local = PlayerManager::GetLocalPlayer();
        
        // Update local player
        local.origin = m_LocalPosition;
        local.yaw = m_LocalYaw;
        local.team = m_LocalTeam;
        
        // Update all players from buffers
        for (size_t i = 0; i < m_PlayerBuffers.size() && i < players.size(); i++)
        {
            PlayerRawData& raw = m_PlayerBuffers[i];
            PlayerData& p = players[i];
            
            p.origin = raw.position;
            p.health = raw.health;
            p.maxHealth = raw.maxHealth > 0 ? raw.maxHealth : 100;
            p.team = raw.team;
            p.yaw = raw.yaw;
            p.valid = (raw.valid != 0 && raw.health > 0);
            p.stance = raw.stance;
            p.isAlive = (raw.health > 0);
            p.isEnemy = (raw.team != m_LocalTeam);
            p.distance = (p.origin - m_LocalPosition).Length();
            
            // Copy name if valid
            if (raw.name[0] != 0)
            {
                strncpy_s(p.name, raw.name, 31);
            }
        }
        
        return;
    }
#endif
    
    // Simulation - zero fill
    for (auto& entry : m_Entries)
    {
        if (entry.buffer)
            memset(entry.buffer, 0, entry.size);
    }
}

void ScatterReadRegistry::Clear()
{
    m_Entries.clear();
}

int ScatterReadRegistry::GetTotalBytes() const
{
    int total = 0;
    for (const auto& e : m_Entries)
        total += (int)e.size;
    return total;
}

// ============================================================================
// MAP TEXTURE MANAGER IMPLEMENTATION
// ============================================================================
void MapTextureManager::InitializeMapDatabase()
{
    // BO6 Maps (example coordinates - need calibration)
    MapInfo nuketown;
    strcpy_s(nuketown.name, "Nuketown");
    nuketown.minX = -2000.0f; nuketown.maxX = 2000.0f;
    nuketown.minY = -2000.0f; nuketown.maxY = 2000.0f;
    s_MapDatabase["mp_nuketown"] = nuketown;
    
    MapInfo skyline;
    strcpy_s(skyline.name, "Skyline");
    skyline.minX = -4000.0f; skyline.maxX = 4000.0f;
    skyline.minY = -4000.0f; skyline.maxY = 4000.0f;
    s_MapDatabase["mp_skyline"] = skyline;
    
    MapInfo rewind;
    strcpy_s(rewind.name, "Rewind");
    rewind.minX = -3000.0f; rewind.maxX = 3000.0f;
    rewind.minY = -3000.0f; rewind.maxY = 3000.0f;
    s_MapDatabase["mp_rewind"] = rewind;
    
    // Add more maps as needed
}

bool MapTextureManager::LoadMapConfig(const char* mapName)
{
    auto it = s_MapDatabase.find(mapName);
    if (it != s_MapDatabase.end())
    {
        s_CurrentMap = it->second;
        return true;
    }
    
    // Use default bounds
    strcpy_s(s_CurrentMap.name, mapName);
    s_CurrentMap.minX = -5000.0f;
    s_CurrentMap.maxX = 5000.0f;
    s_CurrentMap.minY = -5000.0f;
    s_CurrentMap.maxY = 5000.0f;
    
    return false;
}

bool MapTextureManager::LoadMapTexture(const char* imagePath)
{
    // In a real implementation, this would load a texture using WIC or similar
    // For now, just store the path
    if (imagePath && imagePath[0])
    {
        strcpy_s(s_CurrentMap.imagePath, imagePath);
        s_CurrentMap.hasTexture = true;
        
        // Apply config scaling
        s_CurrentMap.scaleX = g_Config.mapScaleX;
        s_CurrentMap.scaleY = g_Config.mapScaleY;
        s_CurrentMap.offsetX = g_Config.mapOffsetX;
        s_CurrentMap.offsetY = g_Config.mapOffsetY;
        s_CurrentMap.rotation = g_Config.mapRotation;
        
        return true;
    }
    
    s_CurrentMap.hasTexture = false;
    return false;
}

Vec2 MapTextureManager::GameToMapCoords(const Vec3& gamePos)
{
    MapInfo& map = s_CurrentMap;
    
    // Normalize to 0-1 range
    float nx = (gamePos.x - map.minX) / (map.maxX - map.minX);
    float ny = (gamePos.y - map.minY) / (map.maxY - map.minY);
    
    // Apply scale and offset
    nx = nx * map.scaleX + map.offsetX;
    ny = ny * map.scaleY + map.offsetY;
    
    // Apply rotation
    if (map.rotation != 0.0f)
    {
        float rad = map.rotation * 3.14159265f / 180.0f;
        float cx = 0.5f, cy = 0.5f;  // Rotate around center
        float dx = nx - cx, dy = ny - cy;
        float cosR = cosf(rad), sinR = sinf(rad);
        nx = cx + dx * cosR - dy * sinR;
        ny = cy + dx * sinR + dy * cosR;
    }
    
    // Scale to image dimensions
    return Vec2(nx * map.imageWidth, (1.0f - ny) * map.imageHeight);
}

Vec2 MapTextureManager::GameToRadarCoords(const Vec3& gamePos, const Vec3& localPos, float localYaw,
                                           float radarCX, float radarCY, float radarSize, float zoom)
{
    Vec3 delta = gamePos - localPos;
    
    // Rotate relative to player view
    float yawRad = -localYaw * 3.14159265f / 180.0f;
    float cosY = cosf(yawRad);
    float sinY = sinf(yawRad);
    
    float rotX = delta.x * cosY - delta.y * sinY;
    float rotY = delta.x * sinY + delta.y * cosY;
    
    // Scale to radar size
    float scale = (radarSize * 0.4f) / (100.0f / zoom);
    
    return Vec2(radarCX + rotX * scale, radarCY - rotY * scale);
}

void MapTextureManager::SetMapBounds(const char* mapName, float minX, float maxX, float minY, float maxY)
{
    MapInfo info;
    strcpy_s(info.name, mapName);
    info.minX = minX;
    info.maxX = maxX;
    info.minY = minY;
    info.maxY = maxY;
    s_MapDatabase[mapName] = info;
}

// ============================================================================
// DMA ENGINE IMPLEMENTATION
// ============================================================================
bool DMAEngine::Initialize()
{
    // Load config first
    LoadConfig("zero.ini");
    return InitializeWithConfig(g_Config);
}

bool DMAEngine::InitializeWithConfig(const DMAConfig& config)
{
    strcpy_s(s_StatusText, "INITIALIZING...");
    
#if DMA_ENABLED
    // Build device string based on config
    char deviceString[256];
    
    if (config.useCustomPCIe && config.customPCIeID[0])
    {
        // Hardware-ID Masking: Use custom PCIe ID
        snprintf(deviceString, sizeof(deviceString), "%s://pcieid=%s,algo=%s%s%s",
                 config.deviceType,
                 config.customPCIeID,
                 config.deviceAlgo,
                 config.deviceArg[0] ? "," : "",
                 config.deviceArg);
        
        snprintf(s_DeviceInfo, sizeof(s_DeviceInfo), "Custom: %s (ID: %s)", 
                 config.deviceType, config.customPCIeID);
    }
    else if (config.deviceArg[0])
    {
        snprintf(deviceString, sizeof(deviceString), "%s://%s,algo=%s",
                 config.deviceType, config.deviceArg, config.deviceAlgo);
        
        snprintf(s_DeviceInfo, sizeof(s_DeviceInfo), "Device: %s (%s)", 
                 config.deviceType, config.deviceArg);
    }
    else
    {
        snprintf(deviceString, sizeof(deviceString), "%s://algo=%s",
                 config.deviceType, config.deviceAlgo);
        
        snprintf(s_DeviceInfo, sizeof(s_DeviceInfo), "Device: %s (default)", config.deviceType);
    }
    
    // Initialize VMMDLL with config-driven device string
    LPCSTR args[] = {"", "-device", deviceString};
    g_VMMDLL = VMMDLL_Initialize(3, args);
    
    if (!g_VMMDLL)
    {
        // Fallback to simple init
        LPCSTR args2[] = {"", "-device", config.deviceType};
        g_VMMDLL = VMMDLL_Initialize(3, args2);
    }
    
    if (!g_VMMDLL)
    {
        strcpy_s(s_StatusText, "FPGA ERROR");
        strcpy_s(s_DeviceInfo, "Connection failed");
        goto simulation;
    }
    
    // Find target process
    if (!VMMDLL_PidGetFromName(g_VMMDLL, (LPSTR)config.processName, &g_DMA_PID) || g_DMA_PID == 0)
    {
        snprintf(s_StatusText, sizeof(s_StatusText), "%s NOT FOUND", config.processName);
        goto simulation;
    }
    
    // Get base address
    s_BaseAddress = VMMDLL_ProcessGetModuleBaseW(g_VMMDLL, g_DMA_PID, config.processNameW);
    if (s_BaseAddress == 0)
    {
        strcpy_s(s_StatusText, "BASE ADDR ERROR");
        goto simulation;
    }
    
    // Get module size
    VMMDLL_MAP_MODULEENTRY moduleEntry = {};
    if (VMMDLL_Map_GetModuleFromNameW(g_VMMDLL, g_DMA_PID, config.processNameW, &moduleEntry, 0))
    {
        s_ModuleSize = moduleEntry.cbImageSize;
    }
    else
    {
        s_ModuleSize = 0x5000000;
    }
    
    s_Connected = true;
    s_SimulationMode = false;
    strcpy_s(s_StatusText, "ONLINE");
    
    // Run pattern scanner
    PatternScanner::UpdateAllOffsets();
    
    // Initialize map database
    MapTextureManager::InitializeMapDatabase();
    
    // Load map texture if configured
    if (config.mapImagePath[0])
    {
        MapTextureManager::LoadMapTexture(config.mapImagePath);
    }
    
    return true;
    
simulation:
#endif
    
    // Simulation mode
    s_Connected = false;
    s_SimulationMode = true;
    s_BaseAddress = 0x140000000;
    s_ModuleSize = 0x5000000;
    strcpy_s(s_StatusText, "SIMULATION");
    strcpy_s(s_DeviceInfo, "Demo mode (no hardware)");
    
    // Use default offsets
    g_Offsets.PlayerBase = s_BaseAddress + 0x17AA8E98;
    g_Offsets.ClientInfo = s_BaseAddress + 0x17AA9000;
    g_Offsets.EntityList = s_BaseAddress + 0x16D5B8D8;
    
    // Initialize map database even in simulation
    MapTextureManager::InitializeMapDatabase();
    
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
    
    // Save config on exit
    SaveConfig("zero.ini");
    
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

const char* DMAEngine::GetDeviceInfo()
{
    return s_DeviceInfo;
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

void DMAEngine::ExecuteScatter(std::vector<ScatterEntry>& entries)
{
    if (entries.empty()) return;
    
#if DMA_ENABLED
    if (s_Connected && g_VMMDLL && g_DMA_PID)
    {
        // Create scatter handle
        VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(g_VMMDLL, g_DMA_PID, VMMDLL_FLAG_NOCACHE);
        if (hScatter)
        {
            // Prepare all reads
            for (auto& entry : entries)
            {
                VMMDLL_Scatter_Prepare(hScatter, entry.address, (DWORD)entry.size);
            }
            
            // Execute in single transaction
            VMMDLL_Scatter_Execute(hScatter);
            
            // Read all results
            for (auto& entry : entries)
            {
                VMMDLL_Scatter_Read(hScatter, entry.address, (DWORD)entry.size, (PBYTE)entry.buffer, nullptr);
            }
            
            VMMDLL_Scatter_CloseHandle(hScatter);
            return;
        }
        
        // Fallback to individual reads
        for (auto& entry : entries)
        {
            ReadBuffer(entry.address, entry.buffer, entry.size);
        }
        return;
    }
#endif
    
    // Simulation - zero fill
    for (auto& entry : entries)
    {
        if (entry.buffer)
            memset(entry.buffer, 0, entry.size);
    }
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

size_t DMAEngine::GetModuleSize()
{
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
        const size_t chunkSize = 0x10000;
        std::vector<uint8_t> buffer(chunkSize + patternLen);
        
        for (size_t offset = 0; offset < size; offset += chunkSize)
        {
            size_t readSize = min(chunkSize + patternLen, size - offset);
            if (!DMAEngine::ReadBuffer(start + offset, buffer.data(), readSize))
                continue;
            
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
    return FindPattern(DMAEngine::GetBaseAddress(), DMAEngine::GetModuleSize(), pattern, mask);
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
        g_Offsets.PlayerBase = 0x17AA8E98;
        g_Offsets.ClientInfo = 0x17AA9000;
        g_Offsets.EntityList = 0x16D5B8D8;
        s_Scanned = true;
        return true;
    }
    
    uintptr_t baseAddr = DMAEngine::GetBaseAddress();
    size_t moduleSize = DMAEngine::GetModuleSize();
    
    uintptr_t addr = FindPattern(baseAddr, moduleSize, Patterns::PlayerBase, Patterns::PlayerBaseMask);
    if (addr)
    {
        g_Offsets.PlayerBase = ResolveRelative(addr, 3, 7);
        if (g_Offsets.PlayerBase) s_FoundCount++;
    }
    
    addr = FindPattern(baseAddr, moduleSize, Patterns::ClientInfo, Patterns::ClientInfoMask);
    if (addr)
    {
        g_Offsets.ClientInfo = ResolveRelative(addr, 3, 7);
        if (g_Offsets.ClientInfo) s_FoundCount++;
    }
    
    addr = FindPattern(baseAddr, moduleSize, Patterns::EntityList, Patterns::EntityListMask);
    if (addr)
    {
        g_Offsets.EntityList = ResolveRelative(addr, 3, 7);
        if (g_Offsets.EntityList) s_FoundCount++;
    }
    
    addr = FindPattern(baseAddr, moduleSize, Patterns::ViewMatrix, Patterns::ViewMatrixMask);
    if (addr)
    {
        g_Offsets.ViewMatrix = ResolveRelative(addr, 3, 7);
        if (g_Offsets.ViewMatrix) s_FoundCount++;
    }
    
    s_Scanned = true;
#endif
    
    return s_FoundCount > 0;
}

// ============================================================================
// PLAYER MANAGER IMPLEMENTATION
// ============================================================================
void PlayerManager::Initialize()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    s_Players.clear();
    s_Players.reserve(150);
    
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
    
    if (g_Config.useScatterRegistry)
    {
        UpdateWithScatterRegistry();
    }
    else
    {
#if DMA_ENABLED
        if (DMAEngine::IsConnected())
            RealUpdate();
        else
#endif
            SimulateUpdate();
    }
}

void PlayerManager::UpdateWithScatterRegistry()
{
    if (!s_Initialized)
        Initialize();
    
#if DMA_ENABLED
    if (DMAEngine::IsConnected() && g_Offsets.EntityList)
    {
        // Clear registry
        g_ScatterRegistry.Clear();
        
        // Register all player reads
        for (size_t i = 0; i < s_Players.size(); i++)
        {
            uintptr_t entityAddr = g_Offsets.EntityList + i * GameOffsets::EntitySize;
            g_ScatterRegistry.RegisterPlayerData((int)i, entityAddr);
        }
        
        // Register local player
        if (g_Offsets.ClientInfo)
        {
            uintptr_t clientBase = DMAEngine::Read<uintptr_t>(g_Offsets.ClientInfo);
            if (clientBase)
            {
                g_ScatterRegistry.RegisterLocalPlayer(clientBase);
            }
        }
        
        // Register view matrix
        if (g_Offsets.ViewMatrix)
        {
            g_ScatterRegistry.RegisterViewMatrix(g_Offsets.ViewMatrix);
        }
        
        // Execute all reads in ONE transaction
        g_ScatterRegistry.ExecuteAll();
        
        return;
    }
#endif
    
    SimulateUpdate();
}

void PlayerManager::SimulateUpdate()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    s_SimTime += 1.0f / (float)g_Config.updateRateHz;
    
    s_LocalPlayer.yaw = fmodf(s_SimTime * 15.0f, 360.0f);
    s_LocalPlayer.origin = Vec3(0, 0, 0);
    
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
        
        float healthBase = 50.0f + sinf(s_SimTime * 0.2f + (float)i * 0.7f) * 40.0f;
        p.health = (int)healthBase;
        p.health = max(1, min(p.health, 100));
        p.isAlive = true;
        p.isVisible = ((int)(s_SimTime * 2 + i) % 3) != 0;
    }
}

void PlayerManager::RealUpdate()
{
    UpdateWithScatterRegistry();
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
// UTILITY FUNCTIONS
// ============================================================================
bool WorldToScreen(const Vec3& worldPos, Vec2& screenPos, int screenW, int screenH)
{
    (void)worldPos;
    screenPos = Vec2((float)screenW / 2, (float)screenH / 2);
    return true;
}

bool WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float localYaw,
                  Vec2& radarPos, float radarCX, float radarCY, float radarScale)
{
    return MapTextureManager::GameToRadarCoords(worldPos, localPos, localYaw,
                                                 radarCX, radarCY, radarScale * 2, 1.0f).x != 0 ||
           MapTextureManager::GameToRadarCoords(worldPos, localPos, localYaw,
                                                 radarCX, radarCY, radarScale * 2, 1.0f).y != 0;
}

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
    angles.x = -atan2f(delta.z, hyp) * (180.0f / 3.14159265f);
    angles.y = atan2f(delta.y, delta.x) * (180.0f / 3.14159265f);
    angles.z = 0;
    
    return angles;
}

void SmoothAngle(Vec3& currentAngle, const Vec3& targetAngle, float smoothness)
{
    if (smoothness <= 0) smoothness = 1;
    
    Vec3 delta = targetAngle - currentAngle;
    
    while (delta.y > 180.0f) delta.y -= 360.0f;
    while (delta.y < -180.0f) delta.y += 360.0f;
    
    currentAngle.x += delta.x / smoothness;
    currentAngle.y += delta.y / smoothness;
}
