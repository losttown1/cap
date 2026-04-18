-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT VIN - VIN SCRATCH MISSIONS
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local IsScratching = false
local VinZoneActive = false
local CurrentVinLocation = nil
local VinBlip = nil

-- ═══════════════════════════════════════════════════════════════════════════════
-- VIN DESTINATION
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:setVinLocation', function(location)
    SetVinDestination(location)
end)

function SetVinDestination(location)
    CurrentVinLocation = location
    
    -- Remove old blip
    if VinBlip then
        RemoveBlip(VinBlip)
    end
    
    -- Create blip
    VinBlip = AddBlipForCoord(location.coords.x, location.coords.y, location.coords.z)
    SetBlipSprite(VinBlip, Config.Blips.vin.sprite)
    SetBlipColour(VinBlip, Config.Blips.vin.color)
    SetBlipScale(VinBlip, Config.Blips.vin.scale)
    SetBlipRoute(VinBlip, true)
    SetBlipRouteColour(VinBlip, Config.Blips.vin.color)
    BeginTextCommandSetBlipName('STRING')
    AddTextComponentString('VIN Location')
    EndTextCommandSetBlipName(VinBlip)
    
    QBCore.Functions.Notify('VIN scratch location marked on your GPS', 'primary')
    
    -- Start VIN zone check
    StartVinZone(location)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- VIN ZONE
-- ═══════════════════════════════════════════════════════════════════════════════

function StartVinZone(location)
    VinZoneActive = true
    
    CreateThread(function()
        while VinZoneActive do
            local ActiveContract = exports['zr-carboosting']:GetActiveContract()
            if not ActiveContract or ActiveContract.contractType ~= 'vinscratch' then
                VinZoneActive = false
                break
            end
            
            local ped = PlayerPedId()
            local coords = GetEntityCoords(ped)
            local dist = #(coords - location.coords)
            
            if dist < 50.0 then
                local vehicle = GetClosestVehicleToContract()
                
                if dist < 10.0 and DoesEntityExist(vehicle) then
                    -- Show interaction
                    DrawText3D(location.coords + vector3(0, 0, 1.0), '[E] Scratch VIN Number')
                    
                    if IsControlJustPressed(0, 38) and not IsScratching then
                        StartVinScratch(vehicle, location)
                    end
                end
            end
            
            Wait(dist < 50.0 and 0 or 500)
        end
    end)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- VIN SCRATCH PROCESS
-- ═══════════════════════════════════════════════════════════════════════════════

function StartVinScratch(vehicle, location)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    local vinConfig = Config.ContractTypes.vinscratch.vinScratch
    
    IsScratching = true
    
    -- Check for required item
    QBCore.Functions.TriggerCallback('zr-carboosting:server:hasItem', function(hasItem)
        if not hasItem then
            QBCore.Functions.Notify(Config.Notifications.messages.missing_item .. ': VIN Scratch Tool', 'error')
            IsScratching = false
            return
        end
        
        -- Must be in the vehicle
        local ped = PlayerPedId()
        local currentVehicle = GetVehiclePedIsIn(ped, false)
        
        if currentVehicle ~= vehicle then
            -- Get in vehicle first
            QBCore.Functions.Notify('You must be in the vehicle to scratch the VIN', 'error')
            IsScratching = false
            return
        end
        
        -- Animation
        RequestAnimDict('anim@heists@prison_heiststation@cop_reactions')
        while not HasAnimDictLoaded('anim@heists@prison_heiststation@cop_reactions') do
            Wait(10)
        end
        
        local stages = vinConfig.minigameStages
        local stagesPassed = 0
        
        QBCore.Functions.Notify('Starting VIN scratch process...', 'primary')
        
        for i = 1, stages do
            TaskPlayAnim(ped, 'anim@heists@prison_heiststation@cop_reactions', 'yourgang_01', 2.0, 2.0, -1, 49, 0, false, false, false)
            
            QBCore.Functions.Notify('Stage ' .. i .. '/' .. stages .. ' - Focus!', 'primary')
            
            -- Minigame for each stage
            local minigameConfig = Config.Minigames.vinScratch
            local args = minigameConfig.args(ActiveContract.class, i)
            
            local success = exports[minigameConfig.export][minigameConfig.name](table.unpack(args))
            
            if success then
                stagesPassed = stagesPassed + 1
                PlaySoundFrontend(-1, 'HACKING_MOVE_CURSOR', 'HUD_FRONTEND_DEFAULT_SOUNDSET', true)
                QBCore.Functions.Notify('Stage ' .. i .. ' complete!', 'success')
            else
                ClearPedTasks(ped)
                QBCore.Functions.Notify('VIN scratch failed at stage ' .. i .. '! Tool damaged.', 'error')
                PlaySoundFrontend(-1, 'Hack_Failed', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
                
                -- Remove tool even on fail
                if vinConfig.removeItem then
                    TriggerServerEvent('zr-carboosting:server:removeItems', vinConfig.requiredItem)
                end
                
                -- Fail the contract
                TriggerServerEvent('zr-carboosting:server:contractFailed', ActiveContract.id, 'vin_failed')
                
                IsScratching = false
                VinZoneActive = false
                return
            end
            
            Wait(800)
        end
        
        ClearPedTasks(ped)
        
        if stagesPassed == stages then
            -- Success rate check
            local successChance = vinConfig.successChance
            
            -- Modify by player level (bonus for higher levels)
            local boostingData = exports['zr-carboosting']:GetBoostingData()
            if boostingData and boostingData.level then
                successChance = successChance + math.min(boostingData.level * 0.2, 15) -- Max 15% bonus
            end
            
            local roll = math.random(100)
            
            if roll <= successChance then
                -- Success!
                VinScratchSuccess(vehicle)
            else
                -- Bad luck
                QBCore.Functions.Notify('VIN scratch corrupted! The vehicle is now unusable.', 'error')
                PlaySoundFrontend(-1, 'ScreenFlash', 'MissionFailedSounds', true)
                
                -- Fail contract
                TriggerServerEvent('zr-carboosting:server:contractFailed', ActiveContract.id, 'vin_corrupted')
                
                -- Remove tool
                if vinConfig.removeItem then
                    TriggerServerEvent('zr-carboosting:server:removeItems', vinConfig.requiredItem)
                end
            end
        end
        
        IsScratching = false
        VinZoneActive = false
        
    end, vinConfig.requiredItem)
end

function VinScratchSuccess(vehicle)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    -- Remove tool
    local vinConfig = Config.ContractTypes.vinscratch.vinScratch
    if vinConfig.removeItem then
        TriggerServerEvent('zr-carboosting:server:removeItems', vinConfig.requiredItem)
    end
    
    -- Get vehicle properties
    local props = QBCore.Functions.GetVehicleProperties(vehicle)
    
    if not props then
        QBCore.Functions.Notify('Error getting vehicle properties!', 'error')
        TriggerServerEvent('zr-carboosting:server:contractFailed', ActiveContract.id, 'props_error')
        return
    end
    
    -- Generate new plate
    local newPlate = GenerateCleanPlate()
    
    -- Set new plate
    SetVehicleNumberPlateText(vehicle, newPlate)
    props.plate = newPlate
    
    -- Update properties
    QBCore.Functions.SetVehicleProperties(vehicle, props)
    
    QBCore.Functions.Notify('VIN scratched successfully! Vehicle is now yours.', 'success')
    PlaySoundFrontend(-1, 'Mission_Pass_Notify', 'DLC_HEISTS_GENERAL_FRONTEND_SOUNDS', true)
    
    -- Register vehicle to player
    TriggerServerEvent('zr-carboosting:server:registerVehicle', ActiveContract.id, props, newPlate)
    
    -- Cleanup
    if VinBlip then
        RemoveBlip(VinBlip)
        VinBlip = nil
    end
    
    CurrentVinLocation = nil
end

function GenerateCleanPlate()
    local chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    local nums = '0123456789'
    local plate = ''
    
    -- Format: ABC 1234
    for i = 1, 3 do
        local idx = math.random(1, #chars)
        plate = plate .. string.sub(chars, idx, idx)
    end
    
    plate = plate .. ' '
    
    for i = 1, 4 do
        local idx = math.random(1, #nums)
        plate = plate .. string.sub(nums, idx, idx)
    end
    
    return plate
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function GetClosestVehicleToContract()
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract or not ActiveContract.vehiclePlate then return nil end
    
    local vehicles = GetGamePool('CVehicle')
    
    for _, vehicle in ipairs(vehicles) do
        local plate = GetVehicleNumberPlateText(vehicle)
        if string.gsub(plate, '%s+', '') == string.gsub(ActiveContract.vehiclePlate, '%s+', '') then
            return vehicle
        end
    end
    
    return nil
end

function DrawText3D(coords, text)
    local onScreen, x, y = World3dToScreen2d(coords.x, coords.y, coords.z)
    
    if onScreen then
        SetTextScale(0.4, 0.4)
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

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:cleanupVin', function()
    IsScratching = false
    VinZoneActive = false
    CurrentVinLocation = nil
    
    if VinBlip then
        RemoveBlip(VinBlip)
        VinBlip = nil
    end
end)

AddEventHandler('onResourceStop', function(resource)
    if resource == GetCurrentResourceName() then
        if VinBlip then
            RemoveBlip(VinBlip)
        end
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('SetVinDestination', SetVinDestination)
exports('IsVinScratching', function() return IsScratching end)
