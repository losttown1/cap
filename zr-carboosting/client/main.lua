-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT MAIN - ZR CARBOOSTING
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local PlayerData = {}
local BoostingData = {
    level = 1,
    points = 0,
    currency = 0,
    heat = 0,
    completedContracts = 0,
    failedContracts = 0
}

local ActiveContract = nil
local ContractVehicle = nil
local ContractBlip = nil
local SearchAreaBlip = nil
local DropOffBlip = nil

local IsTabletOpen = false
local IsInMission = false
local GPSDisabled = false
local TimerThread = false

local Guards = {}
local Party = {
    isLeader = false,
    members = {},
    invites = {}
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- INITIALIZATION
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('QBCore:Client:OnPlayerLoaded', function()
    PlayerData = QBCore.Functions.GetPlayerData()
    TriggerServerEvent('zr-carboosting:server:loadPlayerData')
end)

RegisterNetEvent('QBCore:Client:OnPlayerUnload', function()
    CleanupContract(false)
    PlayerData = {}
    BoostingData = {}
end)

AddEventHandler('onResourceStart', function(resource)
    if resource == GetCurrentResourceName() then
        PlayerData = QBCore.Functions.GetPlayerData()
        if PlayerData and PlayerData.citizenid then
            TriggerServerEvent('zr-carboosting:server:loadPlayerData')
        end
    end
end)

AddEventHandler('onResourceStop', function(resource)
    if resource == GetCurrentResourceName() then
        CleanupContract(false)
        CloseTablet()
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- DATA HANDLERS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:setPlayerData', function(data)
    BoostingData = data
    Utils.Debug('Player data loaded:', json.encode(data))
    
    -- Update UI if open
    if IsTabletOpen then
        SendNUIMessage({
            action = 'updatePlayerData',
            data = GetPlayerUIData()
        })
    end
end)

RegisterNetEvent('zr-carboosting:client:updateStats', function(stats)
    for key, value in pairs(stats) do
        BoostingData[key] = value
    end
    
    if IsTabletOpen then
        SendNUIMessage({
            action = 'updatePlayerData',
            data = GetPlayerUIData()
        })
    end
end)

function GetPlayerUIData()
    local nextLevelPoints = Utils.GetPointsForNextLevel(BoostingData.level)
    local currentLevelPoints = 0
    
    for i = 1, BoostingData.level - 1 do
        currentLevelPoints = currentLevelPoints + Config.Progression.pointsPerLevel(i)
    end
    
    local progressPoints = BoostingData.points - currentLevelPoints
    local progressPercent = (progressPoints / nextLevelPoints) * 100
    
    return {
        level = BoostingData.level,
        points = BoostingData.points,
        currency = BoostingData.currency,
        heat = BoostingData.heat,
        completedContracts = BoostingData.completedContracts or 0,
        failedContracts = BoostingData.failedContracts or 0,
        nextLevelPoints = nextLevelPoints,
        progressPercent = math.floor(progressPercent)
    }
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- TABLET COMMANDS & ITEM
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterCommand(Config.TabletCommand, function()
    if Config.DevMode then
        ToggleTablet()
    end
end, false)

RegisterKeyMapping(Config.TabletCommand, 'Open Boosting Tablet', 'keyboard', Config.Key)

-- Item usage
RegisterNetEvent('zr-carboosting:client:useTablet', function()
    ToggleTablet()
end)

function ToggleTablet()
    if IsTabletOpen then
        CloseTablet()
    else
        OpenTablet()
    end
end

function OpenTablet()
    if IsTabletOpen then return end
    if IsInMission and ActiveContract then
        -- Allow tablet during mission for status
    end
    
    IsTabletOpen = true
    
    -- Request contract data
    QBCore.Functions.TriggerCallback('zr-carboosting:server:getContracts', function(contracts)
        SetNuiFocus(true, true)
        
        SendNUIMessage({
            action = 'open',
            data = {
                player = GetPlayerUIData(),
                contracts = contracts,
                classes = Config.Classes,
                contractTypes = Config.ContractTypes,
                store = Config.Store.items,
                activeContract = ActiveContract,
                party = Party
            }
        })
        
        -- Animation
        local ped = PlayerPedId()
        RequestAnimDict('amb@world_human_seat_wall_tablet@female@base')
        while not HasAnimDictLoaded('amb@world_human_seat_wall_tablet@female@base') do
            Wait(10)
        end
        
        -- Create tablet prop
        local tabletModel = `prop_cs_tablet`
        RequestModel(tabletModel)
        while not HasModelLoaded(tabletModel) do
            Wait(10)
        end
    end)
end

function CloseTablet()
    if not IsTabletOpen then return end
    
    IsTabletOpen = false
    SetNuiFocus(false, false)
    
    SendNUIMessage({
        action = 'close'
    })
    
    ClearPedTasks(PlayerPedId())
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- NUI CALLBACKS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNUICallback('close', function(_, cb)
    CloseTablet()
    cb('ok')
end)

RegisterNUICallback('acceptContract', function(data, cb)
    if ActiveContract then
        QBCore.Functions.Notify(Config.Notifications.messages.already_in_contract, 'error')
        cb({ success = false, message = 'Already in contract' })
        return
    end
    
    TriggerServerEvent('zr-carboosting:server:acceptContract', data.class, data.contractType)
    cb({ success = true })
end)

RegisterNUICallback('purchaseItem', function(data, cb)
    TriggerServerEvent('zr-carboosting:server:purchaseItem', data.itemId)
    cb({ success = true })
end)

RegisterNUICallback('createParty', function(_, cb)
    TriggerServerEvent('zr-carboosting:server:createParty')
    cb({ success = true })
end)

RegisterNUICallback('invitePlayer', function(data, cb)
    TriggerServerEvent('zr-carboosting:server:inviteToParty', data.playerId)
    cb({ success = true })
end)

RegisterNUICallback('leaveParty', function(_, cb)
    TriggerServerEvent('zr-carboosting:server:leaveParty')
    cb({ success = true })
end)

RegisterNUICallback('getOnlinePlayers', function(_, cb)
    QBCore.Functions.TriggerCallback('zr-carboosting:server:getOnlinePlayers', function(players)
        cb(players)
    end)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT HANDLERS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:startContract', function(contractData)
    if ActiveContract then
        CleanupContract(false)
    end
    
    ActiveContract = contractData
    IsInMission = true
    GPSDisabled = false
    
    CloseTablet()
    
    -- Create search area blip
    if contractData.searchArea then
        SearchAreaBlip = AddBlipForRadius(
            contractData.searchArea.x,
            contractData.searchArea.y,
            contractData.searchArea.z,
            contractData.searchRadius
        )
        SetBlipHighDetail(SearchAreaBlip, true)
        SetBlipColour(SearchAreaBlip, Config.Blips.searchArea.color)
        SetBlipAlpha(SearchAreaBlip, Config.Blips.searchArea.alpha)
    end
    
    -- Show contract info
    QBCore.Functions.Notify(Config.Notifications.messages.contract_accepted, 'success')
    
    -- Start timer
    StartContractTimer(contractData.timeLimit)
    
    -- Start GPS alerts if not disabled
    StartGPSAlerts()
    
    -- Update UI
    SendNUIMessage({
        action = 'contractStarted',
        data = {
            contract = contractData,
            timeRemaining = contractData.timeLimit
        }
    })
    
    Utils.Debug('Contract started:', contractData.id)
end)

RegisterNetEvent('zr-carboosting:client:vehicleSpawned', function(netId, plate, coords)
    if not ActiveContract then return end
    
    local vehicle = NetToVeh(netId)
    if not DoesEntityExist(vehicle) then
        -- Wait for vehicle to stream in
        local timeout = 0
        while not DoesEntityExist(NetToVeh(netId)) and timeout < 100 do
            Wait(100)
            timeout = timeout + 1
        end
        vehicle = NetToVeh(netId)
    end
    
    if DoesEntityExist(vehicle) then
        ContractVehicle = {
            entity = vehicle,
            netId = netId,
            plate = plate,
            coords = coords
        }
        
        ActiveContract.vehicleNetId = netId
        ActiveContract.vehiclePlate = plate
        
        -- Create vehicle blip
        if ContractBlip then
            RemoveBlip(ContractBlip)
        end
        
        ContractBlip = AddBlipForCoord(coords.x, coords.y, coords.z)
        SetBlipSprite(ContractBlip, Config.Blips.vehicle.sprite)
        SetBlipColour(ContractBlip, Config.Blips.vehicle.color)
        SetBlipScale(ContractBlip, Config.Blips.vehicle.scale)
        SetBlipAsShortRange(ContractBlip, false)
        BeginTextCommandSetBlipName('STRING')
        AddTextComponentString('Target Vehicle')
        EndTextCommandSetBlipName(ContractBlip)
        
        QBCore.Functions.Notify('Target: ' .. ActiveContract.vehicle.label .. ' | Plate: ' .. plate, 'primary', 7500)
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT TIMER
-- ═══════════════════════════════════════════════════════════════════════════════

function StartContractTimer(duration)
    if TimerThread then return end
    
    TimerThread = true
    ActiveContract.timeRemaining = duration
    
    CreateThread(function()
        while TimerThread and ActiveContract and ActiveContract.timeRemaining > 0 do
            Wait(1000)
            
            if ActiveContract then
                ActiveContract.timeRemaining = ActiveContract.timeRemaining - 1
                
                -- Update UI
                SendNUIMessage({
                    action = 'updateTimer',
                    data = {
                        timeRemaining = ActiveContract.timeRemaining,
                        formatted = Utils.FormatTime(ActiveContract.timeRemaining)
                    }
                })
                
                -- Warning at 60 seconds
                if ActiveContract.timeRemaining == 60 then
                    QBCore.Functions.Notify('60 seconds remaining!', 'error')
                    PlaySoundFrontend(-1, 'Beep_Red', 'DLC_HEIST_HACKING_SNAKE_SOUNDS', true)
                end
                
                -- Warning at 30 seconds
                if ActiveContract.timeRemaining == 30 then
                    QBCore.Functions.Notify('30 seconds remaining!', 'error')
                    PlaySoundFrontend(-1, 'Beep_Red', 'DLC_HEIST_HACKING_SNAKE_SOUNDS', true)
                end
            end
        end
        
        -- Time expired
        if TimerThread and ActiveContract and ActiveContract.timeRemaining <= 0 then
            QBCore.Functions.Notify(Config.Notifications.messages.contract_expired, 'error')
            TriggerServerEvent('zr-carboosting:server:contractFailed', ActiveContract.id, 'timeout')
            CleanupContract(true)
        end
        
        TimerThread = false
    end)
end

function StopContractTimer()
    TimerThread = false
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- GPS SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════

local GPSThread = false

function StartGPSAlerts()
    if GPSThread or GPSDisabled then return end
    if not ActiveContract then return end
    
    GPSThread = true
    local alertInterval = Config.Classes[ActiveContract.class].gpsAlertInterval * 1000
    
    CreateThread(function()
        while GPSThread and ActiveContract and not GPSDisabled do
            Wait(alertInterval)
            
            if GPSThread and ActiveContract and not GPSDisabled then
                -- Check if in vehicle
                local ped = PlayerPedId()
                local currentVehicle = GetVehiclePedIsIn(ped, false)
                
                if currentVehicle and currentVehicle == ContractVehicle?.entity then
                    -- Send GPS alert to police
                    local coords = GetEntityCoords(ped)
                    TriggerServerEvent('zr-carboosting:server:gpsAlert', ActiveContract.id, coords)
                    QBCore.Functions.Notify(Config.Notifications.messages.police_alerted, 'error')
                end
            end
        end
        
        GPSThread = false
    end)
end

function StopGPSAlerts()
    GPSThread = false
end

RegisterNetEvent('zr-carboosting:client:gpsDisabled', function()
    GPSDisabled = true
    StopGPSAlerts()
    QBCore.Functions.Notify(Config.Notifications.messages.gps_disabled, 'success')
    PlaySoundFrontend(-1, 'Hack_Success', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
    
    -- Update UI
    SendNUIMessage({
        action = 'gpsDisabled'
    })
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT COMPLETION
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:contractCompleted', function(rewards)
    QBCore.Functions.Notify(Config.Notifications.messages.contract_completed, 'success')
    
    -- Show rewards
    if rewards.points then
        QBCore.Functions.Notify(string.format(Config.Notifications.messages.points_earned, rewards.points), 'primary')
    end
    
    if rewards.currency then
        QBCore.Functions.Notify(string.format(Config.Notifications.messages.currency_earned, Utils.FormatNumber(rewards.currency)), 'primary')
    end
    
    -- Play success sound
    PlaySoundFrontend(-1, 'Mission_Pass_Notify', 'DLC_HEISTS_GENERAL_FRONTEND_SOUNDS', true)
    
    -- Update UI
    SendNUIMessage({
        action = 'contractCompleted',
        data = rewards
    })
    
    CleanupContract(false)
end)

RegisterNetEvent('zr-carboosting:client:contractFailed', function(reason)
    QBCore.Functions.Notify(Config.Notifications.messages.contract_failed .. ' - ' .. (reason or 'Unknown'), 'error')
    PlaySoundFrontend(-1, 'ScreenFlash', 'MissionFailedSounds', true)
    
    SendNUIMessage({
        action = 'contractFailed',
        data = { reason = reason }
    })
    
    CleanupContract(true)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP
-- ═══════════════════════════════════════════════════════════════════════════════

function CleanupContract(failed)
    StopContractTimer()
    StopGPSAlerts()
    
    -- Remove blips
    if SearchAreaBlip then
        RemoveBlip(SearchAreaBlip)
        SearchAreaBlip = nil
    end
    
    if ContractBlip then
        RemoveBlip(ContractBlip)
        ContractBlip = nil
    end
    
    if DropOffBlip then
        RemoveBlip(DropOffBlip)
        DropOffBlip = nil
    end
    
    -- Cleanup guards
    CleanupGuards()
    
    -- Reset state
    ActiveContract = nil
    ContractVehicle = nil
    IsInMission = false
    GPSDisabled = false
    
    Utils.Debug('Contract cleanup completed')
end

function CleanupGuards()
    for _, guard in pairs(Guards) do
        if DoesEntityExist(guard) then
            DeleteEntity(guard)
        end
    end
    Guards = {}
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- NOTIFICATIONS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:notify', function(message, type)
    QBCore.Functions.Notify(message, type or 'primary')
end)

RegisterNetEvent('zr-carboosting:client:levelUp', function(newLevel)
    QBCore.Functions.Notify(string.format(Config.Notifications.messages.level_up, newLevel), 'success', 7500)
    PlaySoundFrontend(-1, 'RANK_UP', 'HUD_AWARDS', true)
    
    -- Update UI
    SendNUIMessage({
        action = 'levelUp',
        data = { level = newLevel }
    })
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetActiveContract', function()
    return ActiveContract
end)

exports('GetBoostingData', function()
    return BoostingData
end)

exports('IsInContract', function()
    return ActiveContract ~= nil
end)

exports('GetPlayerLevel', function()
    return BoostingData.level or 1
end)
