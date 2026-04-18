-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT GUARDS - NPC GUARD SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local SpawnedGuards = {}
local GuardsActive = false

-- ═══════════════════════════════════════════════════════════════════════════════
-- GUARD SPAWNING
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:spawnGuards', function(vehicleCoords, class, count)
    if not Config.Guards.enabled then return end
    if count <= 0 then return end
    
    CleanupGuards()
    
    local guardConfig = Config.Guards
    local weapons = guardConfig.weapons[class] or {}
    local accuracy = guardConfig.accuracy[class] or 30
    local health = guardConfig.health[class] or 150
    
    for i = 1, count do
        CreateThread(function()
            -- Get random position near vehicle
            local angle = math.random() * 2 * math.pi
            local distance = math.random(guardConfig.spawnRadius.min, guardConfig.spawnRadius.max)
            
            local spawnX = vehicleCoords.x + math.cos(angle) * distance
            local spawnY = vehicleCoords.y + math.sin(angle) * distance
            local spawnZ = vehicleCoords.z
            
            -- Get ground Z
            local found, groundZ = GetGroundZFor_3dCoord(spawnX, spawnY, spawnZ + 50.0, false)
            if found then
                spawnZ = groundZ
            end
            
            -- Load model
            local model = GetHashKey(guardConfig.models[math.random(#guardConfig.models)])
            RequestModel(model)
            while not HasModelLoaded(model) do
                Wait(10)
            end
            
            -- Create ped
            local guard = CreatePed(4, model, spawnX, spawnY, spawnZ, math.random(0, 360), true, true)
            
            if DoesEntityExist(guard) then
                -- Configure guard
                SetPedMaxHealth(guard, health)
                SetEntityHealth(guard, health)
                SetPedArmour(guard, class == 'S+' and 100 or (class == 'S' and 50 or 0))
                
                SetPedAccuracy(guard, accuracy)
                SetPedCombatAbility(guard, 2)
                SetPedCombatMovement(guard, 2)
                SetPedCombatRange(guard, 2)
                SetPedAlertness(guard, 3)
                
                SetPedFleeAttributes(guard, 0, false)
                SetBlockingOfNonTemporaryEvents(guard, true)
                
                -- Give weapon
                if #weapons > 0 then
                    local weapon = GetHashKey(weapons[math.random(#weapons)])
                    GiveWeaponToPed(guard, weapon, 999, false, true)
                    SetCurrentPedWeapon(guard, weapon, true)
                end
                
                -- Set relationship
                local relationship = GetHashKey('GUARD_HOSTILE')
                SetPedRelationshipGroupHash(guard, relationship)
                SetRelationshipBetweenGroups(5, relationship, GetHashKey('PLAYER'))
                SetRelationshipBetweenGroups(5, GetHashKey('PLAYER'), relationship)
                
                -- Guard the area
                TaskGuardCurrentPosition(guard, 10.0, 10.0, true)
                
                table.insert(SpawnedGuards, guard)
                
                Utils.Debug('Spawned guard #' .. i .. ' at:', spawnX, spawnY, spawnZ)
            end
            
            SetModelAsNoLongerNeeded(model)
        end)
        
        Wait(500) -- Stagger spawns
    end
    
    GuardsActive = true
    StartGuardAI()
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- GUARD AI
-- ═══════════════════════════════════════════════════════════════════════════════

function StartGuardAI()
    CreateThread(function()
        while GuardsActive and #SpawnedGuards > 0 do
            local ped = PlayerPedId()
            local playerCoords = GetEntityCoords(ped)
            
            for i = #SpawnedGuards, 1, -1 do
                local guard = SpawnedGuards[i]
                
                if DoesEntityExist(guard) then
                    local guardCoords = GetEntityCoords(guard)
                    local dist = #(playerCoords - guardCoords)
                    
                    -- Check if player is in alert radius
                    if dist < Config.Guards.alertRadius then
                        -- Check line of sight
                        local hasLOS = HasEntityClearLosToEntity(guard, ped, 17)
                        
                        if hasLOS then
                            -- Alert and attack
                            if not IsPedInCombat(guard, ped) then
                                TaskCombatPed(guard, ped, 0, 16)
                            end
                        end
                    end
                    
                    -- Despawn if too far
                    if dist > Config.Guards.despawnDistance then
                        DeleteEntity(guard)
                        table.remove(SpawnedGuards, i)
                    end
                    
                    -- Check if dead
                    if IsPedDeadOrDying(guard, true) then
                        Wait(5000) -- Leave body for a bit
                        if DoesEntityExist(guard) then
                            DeleteEntity(guard)
                        end
                        table.remove(SpawnedGuards, i)
                    end
                else
                    table.remove(SpawnedGuards, i)
                end
            end
            
            Wait(500)
        end
        
        GuardsActive = false
    end)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP
-- ═══════════════════════════════════════════════════════════════════════════════

function CleanupGuards()
    GuardsActive = false
    
    for _, guard in ipairs(SpawnedGuards) do
        if DoesEntityExist(guard) then
            DeleteEntity(guard)
        end
    end
    
    SpawnedGuards = {}
end

RegisterNetEvent('zr-carboosting:client:cleanupGuards', function()
    CleanupGuards()
end)

AddEventHandler('onResourceStop', function(resource)
    if resource == GetCurrentResourceName() then
        CleanupGuards()
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetActiveGuards', function()
    return SpawnedGuards
end)

exports('CleanupGuards', CleanupGuards)
