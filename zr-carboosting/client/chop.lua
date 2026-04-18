-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT CHOP - CHOP SHOP MISSIONS
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local ChoppedParts = {}
local IsChopping = false
local ChopZoneActive = false
local CurrentChopShop = nil

-- Part indices for vehicle
local PartBones = {
    wheel_lf = { bone = 'wheel_lf', offset = vector3(-0.8, 1.0, -0.3) },
    wheel_rf = { bone = 'wheel_rf', offset = vector3(0.8, 1.0, -0.3) },
    wheel_lr = { bone = 'wheel_lr', offset = vector3(-0.8, -1.2, -0.3) },
    wheel_rr = { bone = 'wheel_rr', offset = vector3(0.8, -1.2, -0.3) },
    door_lf = { bone = 'door_dside_f', offset = vector3(-1.0, 0.5, 0.3) },
    door_rf = { bone = 'door_pside_f', offset = vector3(1.0, 0.5, 0.3) },
    door_lr = { bone = 'door_dside_r', offset = vector3(-1.0, -0.5, 0.3) },
    door_rr = { bone = 'door_pside_r', offset = vector3(1.0, -0.5, 0.3) },
    hood = { bone = 'bonnet', offset = vector3(0.0, 2.0, 0.5) },
    trunk = { bone = 'boot', offset = vector3(0.0, -2.0, 0.5) }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- CHOP DESTINATION
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:setChopShop', function(location)
    SetChopDestination(location)
end)

function SetChopDestination(location)
    CurrentChopShop = location
    
    -- Create blip
    local blip = AddBlipForCoord(location.coords.x, location.coords.y, location.coords.z)
    SetBlipSprite(blip, Config.Blips.chop.sprite)
    SetBlipColour(blip, Config.Blips.chop.color)
    SetBlipScale(blip, Config.Blips.chop.scale)
    SetBlipRoute(blip, true)
    SetBlipRouteColour(blip, Config.Blips.chop.color)
    BeginTextCommandSetBlipName('STRING')
    AddTextComponentString('Chop Shop')
    EndTextCommandSetBlipName(blip)
    
    QBCore.Functions.Notify('Chop shop location marked on your GPS', 'primary')
    
    -- Start chop zone check
    StartChopZone(location)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CHOP ZONE
-- ═══════════════════════════════════════════════════════════════════════════════

function StartChopZone(location)
    ChopZoneActive = true
    ChoppedParts = {}
    
    CreateThread(function()
        while ChopZoneActive do
            local ActiveContract = exports['zr-carboosting']:GetActiveContract()
            if not ActiveContract or ActiveContract.contractType ~= 'chopchop' then
                ChopZoneActive = false
                break
            end
            
            local ped = PlayerPedId()
            local coords = GetEntityCoords(ped)
            local dist = #(coords - location.coords)
            
            if dist < 30.0 then
                local vehicle = GetClosestVehicleToContract()
                
                if DoesEntityExist(vehicle) then
                    -- Check if all parts chopped
                    if AreAllPartsChopped() then
                        DrawText3D(GetEntityCoords(vehicle), '[E] Finish Chopping')
                        
                        if IsControlJustPressed(0, 38) and not IsChopping then
                            FinishChopping(vehicle)
                        end
                    else
                        -- Show available parts
                        ShowChopInteractions(vehicle)
                    end
                end
            end
            
            Wait(dist < 30.0 and 0 or 500)
        end
    end)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CHOP INTERACTIONS
-- ═══════════════════════════════════════════════════════════════════════════════

function ShowChopInteractions(vehicle)
    if IsChopping then return end
    
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    local ped = PlayerPedId()
    local pedCoords = GetEntityCoords(ped)
    local chopParts = Config.ContractTypes.chopchop.chopParts
    
    local closestPart = nil
    local closestDist = 2.0
    
    for _, part in ipairs(chopParts) do
        if not ChoppedParts[part.id] then
            local partPos = GetPartPosition(vehicle, part.id)
            
            if partPos then
                local dist = #(pedCoords - partPos)
                
                if dist < closestDist then
                    closestPart = part
                    closestDist = dist
                end
                
                -- Draw marker for unchoped parts
                if dist < 5.0 then
                    DrawMarker(25, partPos.x, partPos.y, partPos.z, 0, 0, 0, 0, 0, 0, 0.3, 0.3, 0.3, 255, 100, 0, 150, false, false, 2, false, nil, nil, false)
                end
            end
        end
    end
    
    -- Show interaction for closest part
    if closestPart then
        local partPos = GetPartPosition(vehicle, closestPart.id)
        DrawText3D(partPos, '[E] Remove ' .. closestPart.label)
        
        if IsControlJustPressed(0, 38) then
            StartChopPart(vehicle, closestPart)
        end
    end
end

function GetPartPosition(vehicle, partId)
    local boneData = PartBones[partId]
    if not boneData then return nil end
    
    local vehCoords = GetEntityCoords(vehicle)
    local vehHeading = GetEntityHeading(vehicle)
    local rad = math.rad(vehHeading)
    
    local offset = boneData.offset
    local x = vehCoords.x + offset.x * math.cos(rad) - offset.y * math.sin(rad)
    local y = vehCoords.y + offset.x * math.sin(rad) + offset.y * math.cos(rad)
    local z = vehCoords.z + offset.z
    
    return vector3(x, y, z)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CHOP PART
-- ═══════════════════════════════════════════════════════════════════════════════

function StartChopPart(vehicle, part)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    IsChopping = true
    
    local ped = PlayerPedId()
    local partPos = GetPartPosition(vehicle, part.id)
    
    -- Face the part
    TaskTurnPedToFaceCoord(ped, partPos.x, partPos.y, partPos.z, 1000)
    Wait(1000)
    
    -- Animation
    RequestAnimDict('mini@repair')
    while not HasAnimDictLoaded('mini@repair') do
        Wait(10)
    end
    
    -- Create prop (wrench)
    local propModel = `prop_tool_wrench`
    RequestModel(propModel)
    while not HasModelLoaded(propModel) do
        Wait(10)
    end
    
    local prop = CreateObject(propModel, 0, 0, 0, true, true, true)
    local boneIndex = GetPedBoneIndex(ped, 57005)
    AttachEntityToEntity(prop, ped, boneIndex, 0.1, 0.02, 0.01, 300.0, 260.0, 0.0, true, true, false, true, 1, true)
    
    -- Minigame
    QBCore.Functions.Notify('Removing ' .. part.label .. '...', 'primary')
    
    local minigameConfig = Config.Minigames.chop
    local args = minigameConfig.args(ActiveContract.class, part.id)
    
    TaskPlayAnim(ped, 'mini@repair', 'fixing_a_ped', 2.0, 2.0, part.time, 1, 0, false, false, false)
    
    local success = exports[minigameConfig.export][minigameConfig.name](table.unpack(args))
    
    ClearPedTasks(ped)
    DeleteObject(prop)
    
    if success then
        -- Mark part as chopped
        ChoppedParts[part.id] = true
        
        -- Apply visual damage to vehicle
        ApplyPartDamage(vehicle, part.id)
        
        -- Give reward
        TriggerServerEvent('zr-carboosting:server:chopPartReward', ActiveContract.id, part.id, part.reward)
        
        QBCore.Functions.Notify(part.label .. ' removed!', 'success')
        PlaySoundFrontend(-1, 'WEAPON_PURCHASE', 'HUD_AMMO_SHOP_SOUNDSET', true)
        
        -- Check if all parts done
        if AreAllPartsChopped() then
            QBCore.Functions.Notify('All parts removed! Finish the job.', 'success')
        end
    else
        QBCore.Functions.Notify('Failed to remove ' .. part.label, 'error')
        PlaySoundFrontend(-1, 'Hack_Failed', 'DLC_HEIST_BIOLAB_PREP_HACKING_SOUNDS', true)
    end
    
    IsChopping = false
end

function ApplyPartDamage(vehicle, partId)
    -- Apply visual damage based on part
    if partId == 'wheel_lf' then
        SetVehicleTyreBurst(vehicle, 0, true, 1000.0)
        SetVehicleWheelHealth(vehicle, 0, 0.0)
    elseif partId == 'wheel_rf' then
        SetVehicleTyreBurst(vehicle, 1, true, 1000.0)
        SetVehicleWheelHealth(vehicle, 1, 0.0)
    elseif partId == 'wheel_lr' then
        SetVehicleTyreBurst(vehicle, 4, true, 1000.0)
        SetVehicleWheelHealth(vehicle, 4, 0.0)
    elseif partId == 'wheel_rr' then
        SetVehicleTyreBurst(vehicle, 5, true, 1000.0)
        SetVehicleWheelHealth(vehicle, 5, 0.0)
    elseif partId == 'door_lf' then
        SetVehicleDoorBroken(vehicle, 0, true)
    elseif partId == 'door_rf' then
        SetVehicleDoorBroken(vehicle, 1, true)
    elseif partId == 'door_lr' then
        SetVehicleDoorBroken(vehicle, 2, true)
    elseif partId == 'door_rr' then
        SetVehicleDoorBroken(vehicle, 3, true)
    elseif partId == 'hood' then
        SetVehicleDoorBroken(vehicle, 4, true)
    elseif partId == 'trunk' then
        SetVehicleDoorBroken(vehicle, 5, true)
    end
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- FINISH CHOPPING
-- ═══════════════════════════════════════════════════════════════════════════════

function AreAllPartsChopped()
    local chopParts = Config.ContractTypes.chopchop.chopParts
    
    for _, part in ipairs(chopParts) do
        if not ChoppedParts[part.id] then
            return false
        end
    end
    
    return true
end

function FinishChopping(vehicle)
    local ActiveContract = exports['zr-carboosting']:GetActiveContract()
    if not ActiveContract then return end
    
    IsChopping = true
    
    local ped = PlayerPedId()
    
    -- Animation
    RequestAnimDict('mp_car_bomb')
    while not HasAnimDictLoaded('mp_car_bomb') do
        Wait(10)
    end
    
    TaskPlayAnim(ped, 'mp_car_bomb', 'car_bomb_mechanic', 2.0, 2.0, 3000, 16, 0, false, false, false)
    
    QBCore.Functions.Notify('Finishing the chop job...', 'primary')
    Wait(3000)
    
    ClearPedTasks(ped)
    
    -- Effects
    local vehCoords = GetEntityCoords(vehicle)
    
    -- Explosion effect (small, no damage)
    AddExplosion(vehCoords.x, vehCoords.y, vehCoords.z, 5, 0.0, true, false, 0.5)
    
    Wait(500)
    
    -- Delete vehicle
    SetEntityAsMissionEntity(vehicle, true, true)
    DeleteEntity(vehicle)
    
    -- Complete contract
    TriggerServerEvent('zr-carboosting:server:completeContract', ActiveContract.id)
    
    -- Cleanup
    ChoppedParts = {}
    ChopZoneActive = false
    CurrentChopShop = nil
    IsChopping = false
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
    local onScreen, x, y = World3dToScreen2d(coords.x, coords.y, coords.z + 0.5)
    
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

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:cleanupChop', function()
    ChoppedParts = {}
    ChopZoneActive = false
    CurrentChopShop = nil
    IsChopping = false
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetChoppedParts', function()
    return ChoppedParts
end)

exports('SetChopDestination', SetChopDestination)
