-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER MAIN - ZR CARBOOSTING
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- PLAYER DATA LOADING
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:loadPlayerData', function()
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    
    -- Load from database
    local result = MySQL.Sync.fetchSingle([[
        SELECT * FROM boosting_players WHERE citizenid = ?
    ]], { citizenid })
    
    local playerData
    
    if result then
        playerData = {
            level = result.level or 1,
            points = result.points or 0,
            currency = result.currency or 0,
            heat = result.heat or 0,
            completedContracts = result.completed_contracts or 0,
            failedContracts = result.failed_contracts or 0
        }
    else
        -- Create new player
        playerData = {
            level = Config.Progression.startingLevel,
            points = Config.Progression.startingPoints,
            currency = Config.Progression.startingCurrency,
            heat = 0,
            completedContracts = 0,
            failedContracts = 0
        }
        
        MySQL.Async.execute([[
            INSERT INTO boosting_players (citizenid, level, points, currency, heat, completed_contracts, failed_contracts)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ]], { citizenid, playerData.level, playerData.points, playerData.currency, 0, 0, 0 })
    end
    
    TriggerClientEvent('zr-carboosting:client:setPlayerData', src, playerData)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- GET CONTRACTS
-- ═══════════════════════════════════════════════════════════════════════════════

QBCore.Functions.CreateCallback('zr-carboosting:server:getContracts', function(source, cb)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then cb({}) return end
    
    local citizenid = Player.PlayerData.citizenid
    
    -- Get player data
    local playerData = GetPlayerBoostingData(citizenid)
    
    -- Get police count
    local policeCount = GetPoliceCount()
    
    -- Build contracts list
    local contracts = {}
    
    for classId, classConfig in pairs(Config.Classes) do
        local canAccess, reason, requirement = Utils.CanAccessClass(playerData, classId)
        local enoughPolice = policeCount >= classConfig.minPolice
        
        -- Get random vehicle for preview
        local vehicles = Config.Vehicles[classId] or {}
        local previewVehicle = #vehicles > 0 and vehicles[math.random(#vehicles)] or nil
        
        -- Build contract types for this class
        local contractTypes = {}
        for typeId, typeConfig in pairs(Config.ContractTypes) do
            if typeConfig.available then
                local typeRequirementMet = true
                if typeConfig.requiredLevel and playerData.level < typeConfig.requiredLevel then
                    typeRequirementMet = false
                end
                
                table.insert(contractTypes, {
                    id = typeId,
                    label = typeConfig.label,
                    description = typeConfig.description,
                    icon = typeConfig.icon,
                    available = typeRequirementMet,
                    requiredLevel = typeConfig.requiredLevel
                })
            end
        end
        
        table.insert(contracts, {
            class = classId,
            label = classConfig.label,
            color = classConfig.color,
            icon = classConfig.icon,
            locked = not canAccess,
            lockReason = reason,
            lockRequirement = requirement,
            enoughPolice = enoughPolice,
            requiredPolice = classConfig.minPolice,
            currentPolice = policeCount,
            rewards = classConfig.rewards,
            penalties = classConfig.penalties,
            timeLimit = classConfig.timeLimit,
            previewVehicle = previewVehicle,
            contractTypes = contractTypes
        })
    end
    
    cb(contracts)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- HELPERS
-- ═══════════════════════════════════════════════════════════════════════════════

function GetPlayerBoostingData(citizenid)
    local result = MySQL.Sync.fetchSingle([[
        SELECT * FROM boosting_players WHERE citizenid = ?
    ]], { citizenid })
    
    if result then
        return {
            level = result.level or 1,
            points = result.points or 0,
            currency = result.currency or 0,
            heat = result.heat or 0
        }
    end
    
    return {
        level = 1,
        points = 0,
        currency = 0,
        heat = 0
    }
end

function GetPoliceCount()
    local count = 0
    local players = QBCore.Functions.GetQBPlayers()
    
    for _, player in pairs(players) do
        if player and player.PlayerData and player.PlayerData.job then
            local job = player.PlayerData.job.name
            if job == 'police' or job == 'sheriff' or job == 'bcso' or job == 'sasp' then
                count = count + 1
            end
        end
    end
    
    return count
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- ITEM CALLBACKS
-- ═══════════════════════════════════════════════════════════════════════════════

QBCore.Functions.CreateCallback('zr-carboosting:server:hasItem', function(source, cb, itemName)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then cb(false) return end
    
    local item = Player.Functions.GetItemByName(itemName)
    cb(item and item.amount > 0)
end)

QBCore.Functions.CreateCallback('zr-carboosting:server:hasRequiredItems', function(source, cb, contractType)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then cb(false) return end
    
    local typeConfig = Config.ContractTypes[contractType]
    if not typeConfig or not typeConfig.requiredItems then
        cb(true)
        return
    end
    
    for _, req in ipairs(typeConfig.requiredItems) do
        local item = Player.Functions.GetItemByName(req.item)
        if not item or item.amount < req.amount then
            cb(false, req.item)
            return
        end
    end
    
    cb(true)
end)

RegisterNetEvent('zr-carboosting:server:removeItems', function(itemName, amount)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    amount = amount or 1
    Player.Functions.RemoveItem(itemName, amount)
    TriggerClientEvent('inventory:client:ItemBox', src, QBCore.Shared.Items[itemName], 'remove', amount)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- GPS ALERTS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:gpsAlert', function(contractId, coords)
    local src = source
    
    -- Send dispatch alert
    if Config.Dispatch == 'ps-dispatch' then
        exports['ps-dispatch']:CustomAlert({
            coords = coords,
            message = 'GPS Tracker Signal - Possible Vehicle Theft',
            dispatchCode = '10-35',
            description = 'GPS tracker signal detected from potentially stolen vehicle',
            radius = 0,
            sprite = 225,
            color = 1,
            scale = 1.0,
            length = 3
        })
    elseif Config.Dispatch == 'cd_dispatch' then
        TriggerEvent('cd_dispatch:AddNotification', {
            job_table = {'police'},
            coords = coords,
            title = '10-35 - Vehicle GPS Alert',
            message = 'GPS tracker signal from stolen vehicle',
            flash = 0,
            unique_id = contractId,
            sound = 1,
            blip = {
                sprite = 225,
                scale = 1.0,
                colour = 1,
                flashes = false,
                text = 'GPS Alert',
                time = 30
            }
        })
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- ONLINE PLAYERS CALLBACK
-- ═══════════════════════════════════════════════════════════════════════════════

QBCore.Functions.CreateCallback('zr-carboosting:server:getOnlinePlayers', function(source, cb)
    local src = source
    local players = {}
    local allPlayers = QBCore.Functions.GetQBPlayers()
    
    for _, player in pairs(allPlayers) do
        if player and player.PlayerData and player.PlayerData.source ~= src then
            table.insert(players, {
                id = player.PlayerData.source,
                name = player.PlayerData.charinfo.firstname .. ' ' .. player.PlayerData.charinfo.lastname,
                citizenid = player.PlayerData.citizenid
            })
        end
    end
    
    cb(players)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- TABLET ITEM
-- ═══════════════════════════════════════════════════════════════════════════════

QBCore.Functions.CreateUseableItem(Config.TabletItem, function(source, item)
    TriggerClientEvent('zr-carboosting:client:useTablet', source)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetPlayerBoostingData', GetPlayerBoostingData)
exports('GetPoliceCount', GetPoliceCount)

Utils.Debug('Server main loaded')
