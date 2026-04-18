-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER REWARDS - REWARD & STATS MANAGEMENT
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- GIVE REWARDS
-- ═══════════════════════════════════════════════════════════════════════════════

function GiveRewards(source, citizenid, rewards)
    local Player = QBCore.Functions.GetPlayer(source)
    if not Player then return end
    
    -- Give money
    if rewards.money and rewards.money > 0 then
        Player.Functions.AddMoney('cash', rewards.money, 'boosting-reward')
    end
    
    -- Give items
    if rewards.items then
        for _, itemData in ipairs(rewards.items) do
            Player.Functions.AddItem(itemData.item, itemData.amount)
            TriggerClientEvent('inventory:client:ItemBox', source, QBCore.Shared.Items[itemData.item], 'add', itemData.amount)
        end
    end
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- UPDATE PLAYER STATS
-- ═══════════════════════════════════════════════════════════════════════════════

function UpdatePlayerStats(citizenid, changes)
    -- Get current data
    local currentData = exports['zr-carboosting']:GetPlayerBoostingData(citizenid)
    
    -- Calculate new values
    local newPoints = math.max(0, currentData.points + (changes.points or 0))
    local newCurrency = math.max(0, currentData.currency + (changes.currency or 0))
    local newHeat = Utils.Clamp((currentData.heat or 0) + (changes.heat or 0), 0, Config.Heat.maxHeat)
    local newCompleted = (currentData.completedContracts or 0) + (changes.completedContracts or 0)
    local newFailed = (currentData.failedContracts or 0) + (changes.failedContracts or 0)
    
    -- Calculate level
    local newLevel = Utils.CalculateLevelFromPoints(newPoints)
    local leveledUp = newLevel > currentData.level
    
    -- Update database
    MySQL.Async.execute([[
        UPDATE boosting_players 
        SET level = ?, points = ?, currency = ?, heat = ?, completed_contracts = ?, failed_contracts = ?
        WHERE citizenid = ?
    ]], { newLevel, newPoints, newCurrency, newHeat, newCompleted, newFailed, citizenid })
    
    -- Get player source
    local Player = QBCore.Functions.GetPlayerByCitizenId(citizenid)
    if Player then
        local source = Player.PlayerData.source
        
        -- Send updated stats
        TriggerClientEvent('zr-carboosting:client:updateStats', source, {
            level = newLevel,
            points = newPoints,
            currency = newCurrency,
            heat = newHeat,
            completedContracts = newCompleted,
            failedContracts = newFailed
        })
        
        -- Level up notification
        if leveledUp then
            TriggerClientEvent('zr-carboosting:client:levelUp', source, newLevel)
            
            -- Give level up rewards
            if Config.Progression.levelUpRewards then
                local levelReward = Config.Progression.levelUpRewards
                if levelReward.currency and levelReward.currency > 0 then
                    newCurrency = newCurrency + levelReward.currency
                    MySQL.Async.execute([[
                        UPDATE boosting_players SET currency = ? WHERE citizenid = ?
                    ]], { newCurrency, citizenid })
                    
                    TriggerClientEvent('zr-carboosting:client:updateStats', source, {
                        currency = newCurrency
                    })
                end
            end
        end
    end
    
    return {
        level = newLevel,
        points = newPoints,
        currency = newCurrency,
        heat = newHeat,
        leveledUp = leveledUp
    }
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- HEAT DECAY
-- ═══════════════════════════════════════════════════════════════════════════════

if Config.Heat.enabled then
    CreateThread(function()
        while true do
            Wait(Config.Heat.decayInterval)
            
            -- Decay heat for all players
            MySQL.Async.execute([[
                UPDATE boosting_players SET heat = GREATEST(0, heat - ?) WHERE heat > 0
            ]], { Config.Heat.decayRate })
        end
    end)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOGGING
-- ═══════════════════════════════════════════════════════════════════════════════

function LogContractEvent(contract, eventType, data)
    if not Config.Logging.enabled then return end
    if not Config.Logging.logEvents[eventType] then return end
    
    local webhook = Config.Logging.webhook
    if not webhook or webhook == '' then return end
    
    -- Build embed
    local embed = {
        title = 'Boosting Contract - ' .. string.upper(eventType:gsub('_', ' ')),
        color = Config.Logging.embedColor,
        fields = {
            { name = 'Contract ID', value = contract.id, inline = true },
            { name = 'Class', value = contract.class, inline = true },
            { name = 'Type', value = contract.contractType, inline = true },
            { name = 'Player', value = contract.citizenid, inline = true },
            { name = 'Vehicle', value = contract.vehicle and contract.vehicle.label or 'N/A', inline = true },
            { name = 'Plate', value = contract.plate or 'N/A', inline = true }
        },
        footer = {
            text = Config.Logging.serverName .. ' | ' .. os.date('%Y-%m-%d %H:%M:%S')
        }
    }
    
    -- Add extra data
    if data then
        if data.reason then
            table.insert(embed.fields, { name = 'Reason', value = data.reason, inline = false })
        end
        if data.points then
            table.insert(embed.fields, { name = 'Points', value = tostring(data.points), inline = true })
        end
        if data.currency then
            table.insert(embed.fields, { name = 'Currency', value = tostring(data.currency), inline = true })
        end
        if data.money then
            table.insert(embed.fields, { name = 'Money', value = '$' .. tostring(data.money), inline = true })
        end
    end
    
    -- Send to Discord
    PerformHttpRequest(webhook, function(err, text, headers) end, 'POST', json.encode({
        username = 'Boosting Logs',
        embeds = { embed }
    }), { ['Content-Type'] = 'application/json' })
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GiveRewards', GiveRewards)
exports('UpdatePlayerStats', UpdatePlayerStats)
exports('LogContractEvent', LogContractEvent)
