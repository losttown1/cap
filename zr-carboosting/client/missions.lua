-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT MISSIONS - VEHICLE STEALING & DELIVERY
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local IsLockpicking = false
local IsHotwiring = false
local IsDisablingGPS = false
local VehicleUnlocked = false
local VehicleHotwired = false

-- ═══════════════════════════════════════════════════════════════════════════════
-- VEHICLE INTERACTION
-- ═══════════════════════════════════════════════════════════════════════════════

CreateThread(function()
    while true do
        local sleep = 1000
        
        local ActiveContract = exports['zr-carboosting']:GetActiveContract()
        
        if ActiveContract then
            sleep = 500
            local ped = PlayerPedId()
            local coords = GetEntityCoords(ped)
            local vehicle = GetClosestVehicle(coords.x, coords.y, coords.z, 3.0, 0, 71)
            
            if DoesEntityExist(vehicle) then
                local plate = GetVehicleNumberPlateText(vehicle)
                
                -- Check if this is target vehicle
                if ActiveContract.vehiclePlate and string.gsub(plate, '%s+', '') == string.gsub(ActiveContract.vehiclePlate, '%s+', '') then
                    sleep = 0
                    local isInVehicle = GetVehiclePedIsIn(ped, false) == vehicle
                    
                    if not isInVehicle then
                        -- Show interaction prompt for locked vehicle
                        if not VehicleUnlocked then
                            DrawText3D(GetEntityCoords(vehicle), '[E] Lockpick Vehicle')
                            
                            if IsControlJustPressed(0, 38) and not IsLockpicking then -- E key
                                StartLockpicking(vehicle)
                            end
                        else
                            DrawText3D(GetEntityCoords(vehicle), '[E] Enter Vehicle')
                        end
                    else
                        -- Inside vehicle
                        if not VehicleHotwired then
                            DrawText3D(GetEntityCoords(vehicle), '[E] Hotwire Vehicle')
                            
                            if IsControlJustPressed(0, 38) and not IsHotwiring then
                                StartHotwiring(vehicle)
                            end
                        else
                            -- Vehicle is ready, show GPS disable option if not disabled
                            local isGPSDisabled = false
                            local success, result = pcall(function()
                                return exports['zr-carboosting']:IsGPSDisabled()
                            end)
                            if success then
                                isGPSDisabled = result
                            end
                            
                            if not isGPSDisabled and not IsDisablingGPS then
                                DrawText3D(GetEntityCoords(vehicle) + vector3(0, 0, 1.0), '[G] Disable GPS Tracker')
                                
                                if IsControlJustPressed(0, 47) then -- G key
                                    StartGPSDisable(vehicle)
                                end
                            end
                        end
                    end
                end
            end
        end
        
        Wait(sleep)
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCKPICKING
-- ═══════════════════════════════════════════════════════════════════════════════

function StartLockpicking(vehicle)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    IsLockpicking = true
    
    -- Check for required items
    QBCore.Functions.TriggerCallback('zr-carboosting:server:hasRequiredItems', function(hasItems, missingItem)
        if not hasItems then
            QBCore.Functions.Notify(Config.Notifications.messages.missing_item .. ': ' .. missingItem, 'error')
            IsLockpicking = false
            return
        end
        
        -- Animation
        local ped = PlayerPedId()
        TaskTurnPedToFaceEntity(ped, vehicle, 1000)
        Wait(1000)
        
        RequestAnimDict('anim@amb@clubhouse@tutorial@bkr_tut_ig3@')
        while not HasAnimDictLoaded('anim@amb@clubhouse@tutorial@bkr_tut_ig3@') do
            Wait(10)
        end
        
        TaskPlayAnim(ped, 'anim@amb@clubhouse@tutorial@bkr_tut_ig3@', 'machinic_loop_mechandplayer', 2.0, 2.0, -1, 49, 0, false, false, false)
        
        -- Minigame
        local minigameConfig = Config.Minigames.unlock
        local args = minigameConfig.args(ActiveContract.class)
        
        local success = exports[minigameConfig.export][minigameConfig.name](table.unpack(args))
        
        ClearPedTasks(ped)
        
        if success then
            -- Remove item server-side
            TriggerServerEvent('zr-carboosting:server:removeItems', 'lockpick')
            
            -- Unlock vehicle
            SetVehicleDoorsLocked(vehicle, 1)
            SetVehicleDoorsLockedForAllPlayers(vehicle, false)
            VehicleUnlocked = true
            
            QBCore.Functions.Notify(Config.Notifications.messages.vehicle_unlocked, 'success')
            PlaySoundFrontend(-1, 'Hack_Success', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
            
            -- Alert server for logging
            TriggerServerEvent('zr-carboosting:server:vehicleUnlocked', ActiveContract.id)
        else
            QBCore.Functions.Notify('Lockpicking failed!', 'error')
            PlaySoundFrontend(-1, 'Hack_Failed', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
            
            -- Alert police
            local coords = GetEntityCoords(ped)
            TriggerServerEvent('zr-carboosting:server:alertFailed', ActiveContract.id, coords, 'lockpick')
        end
        
        IsLockpicking = false
    end, ActiveContract.contractType)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- HOTWIRING
-- ═══════════════════════════════════════════════════════════════════════════════

function StartHotwiring(vehicle)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    IsHotwiring = true
    
    -- Animation
    local ped = PlayerPedId()
    
    RequestAnimDict('anim@amb@clubhouse@tutorial@bkr_tut_ig3@')
    while not HasAnimDictLoaded('anim@amb@clubhouse@tutorial@bkr_tut_ig3@') do
        Wait(10)
    end
    
    TaskPlayAnim(ped, 'anim@amb@clubhouse@tutorial@bkr_tut_ig3@', 'machinic_loop_mechandplayer', 2.0, 2.0, -1, 49, 0, false, false, false)
    
    -- Minigame
    local minigameConfig = Config.Minigames.hotwire
    local args = minigameConfig.args(ActiveContract.class)
    
    local success = exports[minigameConfig.export][minigameConfig.name](table.unpack(args))
    
    ClearPedTasks(ped)
    
    if success then
        -- Start engine
        SetVehicleEngineOn(vehicle, true, true, false)
        SetVehicleNeedsToBeHotwired(vehicle, false)
        VehicleHotwired = true
        
        QBCore.Functions.Notify(Config.Notifications.messages.vehicle_hotwired, 'success')
        PlaySoundFrontend(-1, 'Hack_Success', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
        
        -- Notify server
        TriggerServerEvent('zr-carboosting:server:vehicleHotwired', ActiveContract.id)
        
        -- Set destination based on contract type
        SetContractDestination()
    else
        QBCore.Functions.Notify('Hotwiring failed!', 'error')
        PlaySoundFrontend(-1, 'Hack_Failed', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
        
        -- Set off alarm
        SetVehicleAlarm(vehicle, true)
        StartVehicleAlarm(vehicle)
        
        -- Alert police
        local coords = GetEntityCoords(ped)
        TriggerServerEvent('zr-carboosting:server:alertFailed', ActiveContract.id, coords, 'hotwire')
    end
    
    IsHotwiring = false
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- GPS DISABLE
-- ═══════════════════════════════════════════════════════════════════════════════

function StartGPSDisable(vehicle)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    local contractConfig = Config.ContractTypes[ActiveContract.contractType]
    if not contractConfig or not contractConfig.gpsDisable then return end
    
    IsDisablingGPS = true
    
    -- Check for required item
    QBCore.Functions.TriggerCallback('zr-carboosting:server:hasItem', function(hasItem)
        if not hasItem then
            QBCore.Functions.Notify(Config.Notifications.messages.missing_item .. ': Electronic Kit', 'error')
            IsDisablingGPS = false
            return
        end
        
        -- Animation
        local ped = PlayerPedId()
        
        RequestAnimDict('anim@heists@prison_heiststation@cop_reactions')
        while not HasAnimDictLoaded('anim@heists@prison_heiststation@cop_reactions') do
            Wait(10)
        end
        
        local stages = contractConfig.gpsDisable.minigameStages
        local stagesPassed = 0
        
        for i = 1, stages do
            TaskPlayAnim(ped, 'anim@heists@prison_heiststation@cop_reactions', 'yourgang_01', 2.0, 2.0, -1, 49, 0, false, false, false)
            
            QBCore.Functions.Notify('Disabling GPS... Stage ' .. i .. '/' .. stages, 'primary')
            
            -- Minigame for each stage
            local minigameConfig = Config.Minigames.gpsDisable
            local args = minigameConfig.args(ActiveContract.class, i)
            
            local success = exports[minigameConfig.export][minigameConfig.name](table.unpack(args))
            
            if success then
                stagesPassed = stagesPassed + 1
                PlaySoundFrontend(-1, 'HACKING_MOVE_CURSOR', 'HUD_FRONTEND_DEFAULT_SOUNDSET', true)
            else
                ClearPedTasks(ped)
                QBCore.Functions.Notify('GPS disable failed at stage ' .. i, 'error')
                PlaySoundFrontend(-1, 'Hack_Failed', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
                IsDisablingGPS = false
                return
            end
            
            Wait(500)
        end
        
        ClearPedTasks(ped)
        
        if stagesPassed == stages then
            -- Remove item
            if contractConfig.gpsDisable.removeItem then
                TriggerServerEvent('zr-carboosting:server:removeItems', contractConfig.gpsDisable.requiredItem)
            end
            
            -- Disable GPS
            TriggerServerEvent('zr-carboosting:server:disableGPS', ActiveContract.id)
        end
        
        IsDisablingGPS = false
    end, contractConfig.gpsDisable.requiredItem)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- DESTINATION HANDLING
-- ═══════════════════════════════════════════════════════════════════════════════

function SetContractDestination()
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    if ActiveContract.contractType == 'dropoff' then
        -- Set drop-off location
        if ActiveContract.dropOff then
            SetDropOffDestination(ActiveContract.dropOff)
        else
            -- Request drop-off from server
            TriggerServerEvent('zr-carboosting:server:requestDropOff', ActiveContract.id)
        end
    elseif ActiveContract.contractType == 'chopchop' then
        -- Set chop shop location
        if ActiveContract.chopShop then
            SetChopDestination(ActiveContract.chopShop)
        else
            TriggerServerEvent('zr-carboosting:server:requestChopShop', ActiveContract.id)
        end
    elseif ActiveContract.contractType == 'vinscratch' then
        -- Set VIN location
        if ActiveContract.vinLocation then
            SetVinDestination(ActiveContract.vinLocation)
        else
            TriggerServerEvent('zr-carboosting:server:requestVinLocation', ActiveContract.id)
        end
    end
end

RegisterNetEvent('zr-carboosting:client:setDropOff', function(location)
    SetDropOffDestination(location)
end)

function SetDropOffDestination(location)
    -- Remove old blip
    local success, existingBlip = pcall(function()
        return exports['zr-carboosting']:GetDropOffBlip()
    end)
    if success and existingBlip then
        RemoveBlip(existingBlip)
    end
    
    -- Create new blip
    local blip = AddBlipForCoord(location.coords.x, location.coords.y, location.coords.z)
    SetBlipSprite(blip, Config.Blips.dropoff.sprite)
    SetBlipColour(blip, Config.Blips.dropoff.color)
    SetBlipScale(blip, Config.Blips.dropoff.scale)
    SetBlipRoute(blip, true)
    SetBlipRouteColour(blip, Config.Blips.dropoff.color)
    BeginTextCommandSetBlipName('STRING')
    AddTextComponentString('Drop-Off Location')
    EndTextCommandSetBlipName(blip)
    
    QBCore.Functions.Notify('Drop-off location marked on your GPS', 'primary')
    
    -- Start delivery zone check
    StartDeliveryZone(location)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- DROP-OFF DELIVERY
-- ═══════════════════════════════════════════════════════════════════════════════

local InDropOffZone = false
local DropOffNPC = nil

function StartDeliveryZone(location)
    CreateThread(function()
        while true do
            local ActiveContract = exports['zr-carboosting']:GetActiveContract()
            if not ActiveContract or ActiveContract.contractType ~= 'dropoff' then
                break
            end
            
            local ped = PlayerPedId()
            local coords = GetEntityCoords(ped)
            local dist = #(coords - location.coords)
            
            if dist < 50.0 then
                -- Spawn NPC if not exists
                if not DoesEntityExist(DropOffNPC) then
                    SpawnDropOffNPC(location)
                end
                
                if dist < 5.0 then
                    local vehicle = GetVehiclePedIsIn(ped, false)
                    
                    if DoesEntityExist(vehicle) then
                        local plate = GetVehicleNumberPlateText(vehicle)
                        
                        if string.gsub(plate, '%s+', '') == string.gsub(ActiveContract.vehiclePlate, '%s+', '') then
                            InDropOffZone = true
                            
                            DrawText3D(location.coords, '[E] Deliver Vehicle')
                            
                            if IsControlJustPressed(0, 38) then
                                CompleteDropOff(vehicle, location)
                            end
                        end
                    end
                else
                    InDropOffZone = false
                end
            else
                -- Cleanup NPC if too far
                if DoesEntityExist(DropOffNPC) then
                    DeleteEntity(DropOffNPC)
                    DropOffNPC = nil
                end
            end
            
            Wait(InDropOffZone and 0 or 500)
        end
    end)
end

function SpawnDropOffNPC(location)
    local model = GetHashKey(location.npcModel or 's_m_y_dealer_01')
    
    RequestModel(model)
    while not HasModelLoaded(model) do
        Wait(10)
    end
    
    DropOffNPC = CreatePed(4, model, location.coords.x, location.coords.y, location.coords.z - 1.0, location.heading, false, true)
    SetEntityInvincible(DropOffNPC, true)
    SetBlockingOfNonTemporaryEvents(DropOffNPC, true)
    FreezeEntityPosition(DropOffNPC, true)
    
    -- Animation
    RequestAnimDict('amb@world_human_leaning@male@wall@back@hands_together@idle_a')
    while not HasAnimDictLoaded('amb@world_human_leaning@male@wall@back@hands_together@idle_a') do
        Wait(10)
    end
    TaskPlayAnim(DropOffNPC, 'amb@world_human_leaning@male@wall@back@hands_together@idle_a', 'idle_a', 8.0, 8.0, -1, 1, 0, false, false, false)
end

function CompleteDropOff(vehicle, location)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    local ped = PlayerPedId()
    
    -- Exit vehicle
    TaskLeaveVehicle(ped, vehicle, 0)
    Wait(2000)
    
    -- NPC enters vehicle
    if DoesEntityExist(DropOffNPC) then
        FreezeEntityPosition(DropOffNPC, false)
        ClearPedTasks(DropOffNPC)
        
        TaskEnterVehicle(DropOffNPC, vehicle, -1, -1, 2.0, 1, 0)
        
        Wait(5000)
        
        -- NPC drives away
        TaskVehicleDriveWander(DropOffNPC, vehicle, 30.0, 786603)
        
        Wait(3000)
        
        -- Cleanup and complete
        TriggerServerEvent('zr-carboosting:server:completeContract', ActiveContract.id)
        
        -- Fade out and delete
        Wait(5000)
        if DoesEntityExist(vehicle) then
            DeleteEntity(vehicle)
        end
        if DoesEntityExist(DropOffNPC) then
            DeleteEntity(DropOffNPC)
        end
        DropOffNPC = nil
    end
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- UTILITY FUNCTIONS
-- ═══════════════════════════════════════════════════════════════════════════════

function DrawText3D(coords, text)
    local onScreen, x, y = World3dToScreen2d(coords.x, coords.y, coords.z + 1.0)
    
    if onScreen then
        SetTextScale(0.35, 0.35)
        SetTextFont(4)
        SetTextProportional(1)
        SetTextColour(255, 255, 255, 215)
        SetTextDropShadow(0, 0, 0, 0, 255)
        SetTextEdge(2, 0, 0, 0, 150)
        SetTextDropShadow()
        SetTextOutline()
        SetTextEntry('STRING')
        SetTextCentre(1)
        AddTextComponentString(text)
        DrawText(x, y)
    end
end

function GetClosestVehicleToCoords(coords, radius)
    local vehicles = GetGamePool('CVehicle')
    local closest = nil
    local closestDist = radius
    
    for _, vehicle in ipairs(vehicles) do
        local vehCoords = GetEntityCoords(vehicle)
        local dist = #(coords - vehCoords)
        
        if dist < closestDist then
            closest = vehicle
            closestDist = dist
        end
    end
    
    return closest, closestDist
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP ON RESOURCE STOP
-- ═══════════════════════════════════════════════════════════════════════════════

AddEventHandler('onResourceStop', function(resource)
    if resource == GetCurrentResourceName() then
        if DoesEntityExist(DropOffNPC) then
            DeleteEntity(DropOffNPC)
        end
    end
end)
