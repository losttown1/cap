-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER STATE - STATE MANAGEMENT & PERSISTENCE
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- STATE PERSISTENCE
-- ═══════════════════════════════════════════════════════════════════════════════

-- Save active contracts periodically
if Config.State.saveInterval > 0 then
    CreateThread(function()
        while true do
            Wait(Config.State.saveInterval)
            SaveAllActiveContracts()
        end
    end)
end

function SaveAllActiveContracts()
    local contracts = exports['zr-carboosting']:GetAllActiveContracts()
    
    for citizenid, contract in pairs(contracts) do
        SaveContractState(contract)
    end
end

function SaveContractState(contract)
    local searchArea = contract.searchArea and json.encode({
        x = contract.searchArea.x,
        y = contract.searchArea.y,
        z = contract.searchArea.z
    }) or nil
    
    local spawnPoint = contract.spawnPoint and json.encode({
        x = contract.spawnPoint.x,
        y = contract.spawnPoint.y,
        z = contract.spawnPoint.z
    }) or nil
    
    local timeRemaining = contract.timeLimit - (os.time() - contract.startTime)
    
    local choppedParts = nil
    if contract.contractType == 'chopchop' and contract.choppedParts then
        choppedParts = json.encode(contract.choppedParts)
    end
    
    MySQL.Async.execute([[
        INSERT INTO boosting_active_contracts 
        (contract_id, citizenid, class, contract_type, vehicle_model, plate, vehicle_netid, 
         search_area, spawn_point, time_remaining, gps_disabled, phase, chopped_parts)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON DUPLICATE KEY UPDATE
        time_remaining = VALUES(time_remaining),
        gps_disabled = VALUES(gps_disabled),
        phase = VALUES(phase),
        chopped_parts = VALUES(chopped_parts)
    ]], {
        contract.id,
        contract.citizenid,
        contract.class,
        contract.contractType,
        contract.vehicle and contract.vehicle.model or nil,
        contract.plate,
        contract.vehicleNetId,
        searchArea,
        spawnPoint,
        timeRemaining,
        contract.gpsDisabled and 1 or 0,
        contract.phase,
        choppedParts
    })
end

function LoadContractState(citizenid)
    local result = MySQL.Sync.fetchSingle([[
        SELECT * FROM boosting_active_contracts WHERE citizenid = ?
    ]], { citizenid })
    
    if not result then return nil end
    
    -- Check if contract is still valid (within resume timeout)
    if result.time_remaining <= 0 then
        DeleteContractState(result.contract_id)
        return nil
    end
    
    return {
        id = result.contract_id,
        citizenid = result.citizenid,
        class = result.class,
        contractType = result.contract_type,
        vehicle = result.vehicle_model and { model = result.vehicle_model } or nil,
        plate = result.plate,
        vehicleNetId = result.vehicle_netid,
        searchArea = result.search_area and json.decode(result.search_area) or nil,
        spawnPoint = result.spawn_point and json.decode(result.spawn_point) or nil,
        timeLimit = result.time_remaining,
        startTime = os.time(),
        gpsDisabled = result.gps_disabled == 1,
        phase = result.phase,
        choppedParts = result.chopped_parts and json.decode(result.chopped_parts) or {}
    }
end

function DeleteContractState(contractId)
    MySQL.Async.execute([[
        DELETE FROM boosting_active_contracts WHERE contract_id = ?
    ]], { contractId })
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- PLAYER RECONNECT
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:checkResumeContract', function()
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    
    -- Check for saved contract
    local savedContract = LoadContractState(citizenid)
    
    if savedContract then
        -- Offer to resume
        TriggerClientEvent('zr-carboosting:client:offerResume', src, savedContract)
    end
end)

RegisterNetEvent('zr-carboosting:server:resumeContract', function(accept)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    local savedContract = LoadContractState(citizenid)
    
    if not savedContract then return end
    
    if accept then
        -- Resume contract
        savedContract.source = src
        
        -- Re-add to active contracts
        -- Note: Vehicle needs to be respawned
        TriggerClientEvent('zr-carboosting:client:startContract', src, savedContract)
        
        -- Respawn vehicle if needed
        if savedContract.spawnPoint then
            CreateThread(function()
                Wait(2000)
                -- SpawnContractVehicle would need to be accessible here
                TriggerEvent('zr-carboosting:server:respawnVehicle', savedContract)
            end)
        end
        
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Contract resumed!', 'success')
    else
        -- Decline - fail the contract
        DeleteContractState(savedContract.id)
        
        -- Apply penalties
        local classConfig = Config.Classes[savedContract.class]
        if classConfig then
            exports['zr-carboosting']:UpdatePlayerStats(citizenid, {
                points = -classConfig.penalties.points,
                heat = classConfig.penalties.heat,
                failedContracts = 1
            })
        end
        
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Contract abandoned. Penalties applied.', 'error')
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER RESTART HANDLING
-- ═══════════════════════════════════════════════════════════════════════════════

if Config.State.restoreOnRestart then
    AddEventHandler('onResourceStart', function(resource)
        if resource == GetCurrentResourceName() then
            -- Clean up old contracts on restart
            MySQL.Async.execute([[
                DELETE FROM boosting_active_contracts 
                WHERE time_remaining <= 0 
                   OR created_at < DATE_SUB(NOW(), INTERVAL 1 HOUR)
            ]])
        end
    end)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- PLAYER DISCONNECT
-- ═══════════════════════════════════════════════════════════════════════════════

AddEventHandler('playerDropped', function(reason)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    
    if Player then
        local citizenid = Player.PlayerData.citizenid
        local contract = exports['zr-carboosting']:GetActiveContract(citizenid)
        
        if contract then
            -- Save contract state
            SaveContractState(contract)
            
            -- Cleanup vehicle
            if contract.vehicleEntity and DoesEntityExist(contract.vehicleEntity) then
                DeleteEntity(contract.vehicleEntity)
            end
            
            Utils.Debug('Player disconnected with active contract:', contract.id)
        end
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('SaveContractState', SaveContractState)
exports('LoadContractState', LoadContractState)
exports('DeleteContractState', DeleteContractState)
