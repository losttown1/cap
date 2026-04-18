-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT DRONE - DRONE DELIVERY SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local ActiveDrones = {}
local DroppedPackages = {}

-- ═══════════════════════════════════════════════════════════════════════════════
-- DRONE DELIVERY
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:startDroneDelivery', function(deliveryId, dropLocation, items)
    if not Config.Drone.enabled then return end
    
    CreateThread(function()
        -- Get spawn position (high above drop location)
        local startPos = vector3(
            dropLocation.x + math.random(-50, 50),
            dropLocation.y + math.random(-50, 50),
            dropLocation.z + Config.Drone.spawnHeight
        )
        
        -- Load drone model
        local droneModel = GetHashKey(Config.Drone.model)
        RequestModel(droneModel)
        while not HasModelLoaded(droneModel) do
            Wait(10)
        end
        
        -- Create drone
        local drone = CreateObject(droneModel, startPos.x, startPos.y, startPos.z, true, true, false)
        
        if DoesEntityExist(drone) then
            SetEntityDynamic(drone, false)
            FreezeEntityPosition(drone, false)
            
            -- Store drone info
            ActiveDrones[deliveryId] = {
                entity = drone,
                target = dropLocation,
                items = items,
                phase = 'flying'
            }
            
            -- Start drone flight
            FlightDrone(deliveryId, drone, dropLocation)
        end
        
        SetModelAsNoLongerNeeded(droneModel)
    end)
end)

function FlightDrone(deliveryId, drone, targetPos)
    CreateThread(function()
        local droneData = ActiveDrones[deliveryId]
        if not droneData then return end
        
        -- Calculate flight path
        local startPos = GetEntityCoords(drone)
        local midPoint = vector3(targetPos.x, targetPos.y, startPos.z)
        local dropPoint = vector3(targetPos.x, targetPos.y, targetPos.z + Config.Drone.dropHeight)
        
        -- Phase 1: Fly to position above target
        local phase1Complete = false
        
        while not phase1Complete and DoesEntityExist(drone) do
            local currentPos = GetEntityCoords(drone)
            local dist = #(currentPos - midPoint)
            
            if dist < 2.0 then
                phase1Complete = true
            else
                -- Move towards target
                local direction = norm(midPoint - currentPos)
                local newPos = currentPos + direction * (Config.Drone.flySpeed * 0.03)
                SetEntityCoords(drone, newPos.x, newPos.y, newPos.z, false, false, false, false)
                
                -- Rotate drone towards direction
                local heading = math.deg(math.atan(direction.y, direction.x)) - 90
                SetEntityHeading(drone, heading)
                
                -- Play sound effect periodically
                if math.random(100) < 5 then
                    -- PlaySoundFromEntity(-1, Config.Drone.sounds.flying, drone, 'DLC_BTL_DVCA_SOUNDS', false, 0)
                end
            end
            
            Wait(30)
        end
        
        -- Phase 2: Descend to drop height
        local phase2Complete = false
        
        while not phase2Complete and DoesEntityExist(drone) do
            local currentPos = GetEntityCoords(drone)
            
            if currentPos.z <= dropPoint.z + 0.5 then
                phase2Complete = true
            else
                local newZ = currentPos.z - (Config.Drone.flySpeed * 0.02)
                SetEntityCoords(drone, currentPos.x, currentPos.y, newZ, false, false, false, false)
            end
            
            Wait(30)
        end
        
        -- Phase 3: Drop package
        if DoesEntityExist(drone) then
            DropPackage(deliveryId, GetEntityCoords(drone), droneData.items)
        end
        
        -- Phase 4: Fly away
        Wait(1000)
        
        if DoesEntityExist(drone) then
            local finalTarget = vector3(
                targetPos.x + math.random(-200, 200),
                targetPos.y + math.random(-200, 200),
                Config.Drone.spawnHeight + 50
            )
            
            while DoesEntityExist(drone) do
                local currentPos = GetEntityCoords(drone)
                local dist = #(currentPos - finalTarget)
                
                if dist < 10.0 or currentPos.z > Config.Drone.spawnHeight then
                    DeleteEntity(drone)
                    break
                end
                
                local direction = norm(finalTarget - currentPos)
                local newPos = currentPos + direction * (Config.Drone.flySpeed * 0.04)
                SetEntityCoords(drone, newPos.x, newPos.y, newPos.z, false, false, false, false)
                
                local heading = math.deg(math.atan(direction.y, direction.x)) - 90
                SetEntityHeading(drone, heading)
                
                Wait(30)
            end
        end
        
        -- Cleanup
        ActiveDrones[deliveryId] = nil
    end)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- PACKAGE DROP
-- ═══════════════════════════════════════════════════════════════════════════════

function DropPackage(deliveryId, dropPos, items)
    -- Load package model
    local packageModel = GetHashKey(Config.Drone.packageModel)
    RequestModel(packageModel)
    while not HasModelLoaded(packageModel) do
        Wait(10)
    end
    
    -- Create package
    local package = CreateObject(packageModel, dropPos.x, dropPos.y, dropPos.z, true, true, false)
    
    if DoesEntityExist(package) then
        -- Enable physics
        SetEntityDynamic(package, true)
        ApplyForceToEntity(package, 1, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0, false, true, true, false, true)
        
        -- Play drop sound
        PlaySoundFromCoord(-1, 'WEAPON_IMPACT', dropPos.x, dropPos.y, dropPos.z, 0, false, 10.0, false)
        
        -- Store package info
        DroppedPackages[deliveryId] = {
            entity = package,
            items = items,
            dropTime = GetGameTimer(),
            claimed = false
        }
        
        -- Create blip
        local blip = AddBlipForEntity(package)
        SetBlipSprite(blip, 478)
        SetBlipColour(blip, 5)
        SetBlipScale(blip, 0.8)
        BeginTextCommandSetBlipName('STRING')
        AddTextComponentString('Delivery Package')
        EndTextCommandSetBlipName(blip)
        
        DroppedPackages[deliveryId].blip = blip
        
        -- Start package interaction
        StartPackageInteraction(deliveryId)
        
        -- Auto despawn timer
        SetTimeout(Config.Drone.packageDespawnTime * 1000, function()
            if DroppedPackages[deliveryId] and not DroppedPackages[deliveryId].claimed then
                CleanupPackage(deliveryId)
                QBCore.Functions.Notify('Package despawned - unclaimed', 'error')
            end
        end)
    end
    
    SetModelAsNoLongerNeeded(packageModel)
end

function StartPackageInteraction(deliveryId)
    CreateThread(function()
        while DroppedPackages[deliveryId] and not DroppedPackages[deliveryId].claimed do
            local package = DroppedPackages[deliveryId].entity
            
            if DoesEntityExist(package) then
                local ped = PlayerPedId()
                local playerCoords = GetEntityCoords(ped)
                local packageCoords = GetEntityCoords(package)
                local dist = #(playerCoords - packageCoords)
                
                if dist < Config.Drone.packageStealRadius then
                    DrawText3D(packageCoords + vector3(0, 0, 0.5), '[E] Collect Package')
                    
                    if IsControlJustPressed(0, 38) then
                        CollectPackage(deliveryId)
                    end
                end
            else
                break
            end
            
            Wait(DroppedPackages[deliveryId] and 0 or 500)
        end
    end)
end

function CollectPackage(deliveryId)
    local packageData = DroppedPackages[deliveryId]
    if not packageData or packageData.claimed then return end
    
    packageData.claimed = true
    
    -- Animation
    local ped = PlayerPedId()
    RequestAnimDict('anim@heists@box_carry@')
    while not HasAnimDictLoaded('anim@heists@box_carry@') do
        Wait(10)
    end
    
    TaskPlayAnim(ped, 'anim@heists@box_carry@', 'idle', 2.0, 2.0, 2000, 49, 0, false, false, false)
    
    Wait(2000)
    ClearPedTasks(ped)
    
    -- Give items
    TriggerServerEvent('zr-carboosting:server:collectPackage', deliveryId)
    
    -- Cleanup
    CleanupPackage(deliveryId)
    
    QBCore.Functions.Notify('Package collected!', 'success')
    PlaySoundFrontend(-1, 'PICK_UP', 'HUD_FRONTEND_DEFAULT_SOUNDSET', true)
end

function CleanupPackage(deliveryId)
    local packageData = DroppedPackages[deliveryId]
    if not packageData then return end
    
    if DoesEntityExist(packageData.entity) then
        DeleteEntity(packageData.entity)
    end
    
    if packageData.blip then
        RemoveBlip(packageData.blip)
    end
    
    DroppedPackages[deliveryId] = nil
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function norm(vec)
    local len = math.sqrt(vec.x^2 + vec.y^2 + vec.z^2)
    if len == 0 then return vector3(0, 0, 0) end
    return vector3(vec.x/len, vec.y/len, vec.z/len)
end

function DrawText3D(coords, text)
    local onScreen, x, y = World3dToScreen2d(coords.x, coords.y, coords.z)
    
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

AddEventHandler('onResourceStop', function(resource)
    if resource == GetCurrentResourceName() then
        for _, droneData in pairs(ActiveDrones) do
            if DoesEntityExist(droneData.entity) then
                DeleteEntity(droneData.entity)
            end
        end
        
        for deliveryId, _ in pairs(DroppedPackages) do
            CleanupPackage(deliveryId)
        end
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetActiveDrones', function()
    return ActiveDrones
end)

exports('GetDroppedPackages', function()
    return DroppedPackages
end)
