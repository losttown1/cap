-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER CONTRACTS - CONTRACT MANAGEMENT
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- ACTIVE CONTRACTS STORAGE
-- ═══════════════════════════════════════════════════════════════════════════════

local ActiveContracts = {}
local PlayerCooldowns = {}

-- ═══════════════════════════════════════════════════════════════════════════════
-- ACCEPT CONTRACT
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:acceptContract', function(class, contractType)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    
    -- Validation
    if not Config.Classes[class] then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Invalid contract class!', 'error')
        return
    end
    
    if not Config.ContractTypes[contractType] then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Invalid contract type!', 'error')
        return
    end
    
    -- Check if already has active contract
    if ActiveContracts[citizenid] then
        TriggerClientEvent('zr-carboosting:client:notify', src, Config.Notifications.messages.already_in_contract, 'error')
        return
    end
    
    -- Check cooldown
    if PlayerCooldowns[citizenid] and PlayerCooldowns[citizenid][class] then
        local cooldownEnd = PlayerCooldowns[citizenid][class]
        if os.time() < cooldownEnd then
            local remaining = cooldownEnd - os.time()
            TriggerClientEvent('zr-carboosting:client:notify', src, 'Cooldown: ' .. Utils.FormatTime(remaining), 'error')
            return
        end
    end
    
    -- Get player data
    local playerData = exports['zr-carboosting']:GetPlayerBoostingData(citizenid)
    
    -- Check requirements
    local canAccess, reason, requirement = Utils.CanAccessClass(playerData, class)
    if not canAccess then
        TriggerClientEvent('zr-carboosting:client:notify', src, Config.Notifications.messages.class_locked, 'error')
        return
    end
    
    -- Check heat
    if Config.Heat.enabled and playerData.heat >= Config.Heat.maxHeat then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Too hot! Wait for your heat to cool down.', 'error')
        return
    end
    
    -- Check police count
    local classConfig = Config.Classes[class]
    local policeCount = exports['zr-carboosting']:GetPoliceCount()
    
    if policeCount < classConfig.minPolice then
        TriggerClientEvent('zr-carboosting:client:notify', src, Config.Notifications.messages.not_enough_police, 'error')
        return
    end
    
    -- Generate contract
    local contractId = Utils.GenerateContractId()
    local vehicle = GetRandomVehicle(class)
    local plate = Utils.GeneratePlate()
    
    -- Get spawn zone
    local zones = GetSpawnZonesForClass(class)
    local zone = GetWeightedRandomZone(zones)
    
    if not zone then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'No available spawn zones!', 'error')
        return
    end
    
    -- Find safe spawn point
    local spawnPoint = FindSafeSpawnPoint(zone.coords, zone.radius)
    
    if not spawnPoint then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Could not find spawn location!', 'error')
        return
    end
    
    -- Create contract data
    local contract = {
        id = contractId,
        class = class,
        contractType = contractType,
        citizenid = citizenid,
        source = src,
        vehicle = vehicle,
        plate = plate,
        searchArea = zone.coords,
        searchRadius = classConfig.searchRadius,
        spawnPoint = spawnPoint,
        timeLimit = classConfig.timeLimit,
        startTime = os.time(),
        gpsDisabled = false,
        phase = 'search',
        vehicleNetId = nil
    }
    
    -- Store contract
    ActiveContracts[citizenid] = contract
    
    -- Set cooldown
    if not PlayerCooldowns[citizenid] then
        PlayerCooldowns[citizenid] = {}
    end
    PlayerCooldowns[citizenid][class] = os.time() + classConfig.cooldown
    
    -- Send contract to client
    TriggerClientEvent('zr-carboosting:client:startContract', src, contract)
    
    -- Spawn vehicle when player enters area
    CreateThread(function()
        Wait(2000)
        SpawnContractVehicle(contract)
    end)
    
    -- Log
    LogContractEvent(contract, 'start')
    
    Utils.Debug('Contract started:', contractId, 'Class:', class, 'Type:', contractType)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- SPAWN VEHICLE
-- ═══════════════════════════════════════════════════════════════════════════════

function SpawnContractVehicle(contract)
    local vehicleHash = GetHashKey(contract.vehicle.model)
    
    -- Create vehicle
    local vehicle = CreateVehicleServerSetter(vehicleHash, 'automobile', contract.spawnPoint.x, contract.spawnPoint.y, contract.spawnPoint.z, math.random(0, 360))
    
    if not DoesEntityExist(vehicle) then
        Utils.Debug('Failed to spawn vehicle for contract:', contract.id)
        return
    end
    
    local netId = NetworkGetNetworkIdFromEntity(vehicle)
    
    -- Set plate
    SetVehicleNumberPlateText(vehicle, contract.plate)
    
    -- Lock vehicle
    local state = Entity(vehicle).state
    state.locked = true
    state.hotwired = false
    state.contractId = contract.id
    
    -- Update contract
    contract.vehicleNetId = netId
    contract.vehicleEntity = vehicle
    
    -- Send to client
    local Player = QBCore.Functions.GetPlayer(contract.source)
    if Player then
        TriggerClientEvent('zr-carboosting:client:vehicleSpawned', contract.source, netId, contract.plate, contract.spawnPoint)
        
        -- Spawn guards
        local guardCount = Config.Classes[contract.class].guardCount
        if guardCount > 0 then
            TriggerClientEvent('zr-carboosting:client:spawnGuards', contract.source, contract.spawnPoint, contract.class, guardCount)
        end
    end
    
    Utils.Debug('Vehicle spawned:', contract.vehicle.model, 'Plate:', contract.plate)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT EVENTS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:vehicleUnlocked', function(contractId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local contract = ActiveContracts[Player.PlayerData.citizenid]
    if not contract or contract.id ~= contractId then return end
    
    contract.phase = 'unlocked'
    LogContractEvent(contract, 'vehicle_unlocked')
end)

RegisterNetEvent('zr-carboosting:server:vehicleHotwired', function(contractId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local contract = ActiveContracts[Player.PlayerData.citizenid]
    if not contract or contract.id ~= contractId then return end
    
    contract.phase = 'hotwired'
    LogContractEvent(contract, 'vehicle_hotwired')
end)

RegisterNetEvent('zr-carboosting:server:disableGPS', function(contractId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local contract = ActiveContracts[Player.PlayerData.citizenid]
    if not contract or contract.id ~= contractId then return end
    
    contract.gpsDisabled = true
    TriggerClientEvent('zr-carboosting:client:gpsDisabled', src)
    LogContractEvent(contract, 'gps_disabled')
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- DESTINATION REQUESTS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:requestDropOff', function(contractId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local contract = ActiveContracts[Player.PlayerData.citizenid]
    if not contract or contract.id ~= contractId then return end
    
    local dropOff = GetRandomDropOff()
    contract.dropOff = dropOff
    
    TriggerClientEvent('zr-carboosting:client:setDropOff', src, dropOff)
end)

RegisterNetEvent('zr-carboosting:server:requestChopShop', function(contractId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local contract = ActiveContracts[Player.PlayerData.citizenid]
    if not contract or contract.id ~= contractId then return end
    
    local chopShop = GetRandomChopShop()
    contract.chopShop = chopShop
    
    TriggerClientEvent('zr-carboosting:client:setChopShop', src, chopShop)
end)

RegisterNetEvent('zr-carboosting:server:requestVinLocation', function(contractId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local contract = ActiveContracts[Player.PlayerData.citizenid]
    if not contract or contract.id ~= contractId then return end
    
    local vinLocation = GetRandomVinLocation()
    contract.vinLocation = vinLocation
    
    TriggerClientEvent('zr-carboosting:client:setVinLocation', src, vinLocation)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT COMPLETION
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:completeContract', function(contractId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    local contract = ActiveContracts[citizenid]
    
    if not contract or contract.id ~= contractId then return end
    
    -- Calculate rewards
    local classConfig = Config.Classes[contract.class]
    local rewards = Utils.CalculateReward(classConfig.rewards)
    
    -- Apply party bonus if in party
    local partyData = exports['zr-carboosting']:GetPartyForPlayer(citizenid)
    if partyData then
        rewards.points = math.floor(rewards.points * Config.Party.partyBonus.points)
        rewards.currency = math.floor(rewards.currency * Config.Party.partyBonus.currency)
    end
    
    -- Give rewards
    GiveRewards(src, citizenid, rewards)
    
    -- Update stats
    UpdatePlayerStats(citizenid, {
        points = rewards.points,
        currency = rewards.currency,
        completedContracts = 1
    })
    
    -- Cleanup
    if contract.vehicleEntity and DoesEntityExist(contract.vehicleEntity) then
        DeleteEntity(contract.vehicleEntity)
    end
    
    ActiveContracts[citizenid] = nil
    
    -- Notify client
    TriggerClientEvent('zr-carboosting:client:contractCompleted', src, rewards)
    TriggerClientEvent('zr-carboosting:client:cleanupGuards', src)
    
    -- Log
    LogContractEvent(contract, 'completed', rewards)
    
    Utils.Debug('Contract completed:', contractId, 'Rewards:', json.encode(rewards))
end)

RegisterNetEvent('zr-carboosting:server:contractFailed', function(contractId, reason)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    local contract = ActiveContracts[citizenid]
    
    if not contract or contract.id ~= contractId then return end
    
    -- Apply penalties
    local classConfig = Config.Classes[contract.class]
    local penalties = classConfig.penalties
    
    UpdatePlayerStats(citizenid, {
        points = -penalties.points,
        currency = -penalties.currency,
        heat = penalties.heat,
        failedContracts = 1
    })
    
    -- Cleanup
    if contract.vehicleEntity and DoesEntityExist(contract.vehicleEntity) then
        DeleteEntity(contract.vehicleEntity)
    end
    
    ActiveContracts[citizenid] = nil
    
    -- Notify client
    TriggerClientEvent('zr-carboosting:client:contractFailed', src, reason)
    TriggerClientEvent('zr-carboosting:client:cleanupGuards', src)
    TriggerClientEvent('zr-carboosting:client:cleanupChop', src)
    TriggerClientEvent('zr-carboosting:client:cleanupVin', src)
    
    -- Log
    LogContractEvent(contract, 'failed', { reason = reason })
    
    Utils.Debug('Contract failed:', contractId, 'Reason:', reason)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- VIN SCRATCH COMPLETION
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:registerVehicle', function(contractId, props, newPlate)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    local contract = ActiveContracts[citizenid]
    
    if not contract or contract.id ~= contractId then return end
    
    -- Register vehicle in player garage
    MySQL.Async.execute([[
        INSERT INTO player_vehicles (license, citizenid, vehicle, hash, mods, plate, garage, state)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    ]], {
        Player.PlayerData.license,
        citizenid,
        contract.vehicle.model,
        GetHashKey(contract.vehicle.model),
        json.encode(props),
        newPlate,
        'pillboxgarage', -- Default garage
        0
    })
    
    -- Give rewards (VIN scratch gives vehicle, less money)
    local classConfig = Config.Classes[contract.class]
    local rewards = Utils.CalculateReward(classConfig.rewards)
    rewards.money = math.floor((rewards.money or 0) * 0.3) -- Reduced money for VIN scratch
    rewards.vehicle = contract.vehicle.label
    
    GiveRewards(src, citizenid, rewards)
    
    UpdatePlayerStats(citizenid, {
        points = rewards.points,
        currency = rewards.currency,
        completedContracts = 1
    })
    
    -- Cleanup
    ActiveContracts[citizenid] = nil
    
    TriggerClientEvent('zr-carboosting:client:contractCompleted', src, rewards)
    TriggerClientEvent('zr-carboosting:client:notify', src, 'Vehicle registered to your garage!', 'success')
    
    LogContractEvent(contract, 'vin_completed', { plate = newPlate, vehicle = contract.vehicle.model })
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CHOP REWARDS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:chopPartReward', function(contractId, partId, reward)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local contract = ActiveContracts[Player.PlayerData.citizenid]
    if not contract or contract.id ~= contractId then return end
    
    -- Give item reward
    if reward and reward.item then
        local amount = 1
        if reward.amount then
            amount = math.random(reward.amount[1], reward.amount[2])
        end
        
        Player.Functions.AddItem(reward.item, amount)
        TriggerClientEvent('inventory:client:ItemBox', src, QBCore.Shared.Items[reward.item], 'add', amount)
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetActiveContract', function(citizenid)
    return ActiveContracts[citizenid]
end)

exports('GetAllActiveContracts', function()
    return ActiveContracts
end)

exports('CancelContract', function(citizenid, reason)
    local contract = ActiveContracts[citizenid]
    if not contract then return false end
    
    TriggerClientEvent('zr-carboosting:client:contractFailed', contract.source, reason or 'cancelled')
    
    if contract.vehicleEntity and DoesEntityExist(contract.vehicleEntity) then
        DeleteEntity(contract.vehicleEntity)
    end
    
    ActiveContracts[citizenid] = nil
    return true
end)
