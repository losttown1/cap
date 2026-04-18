-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER WEBHOOKS - DISCORD LOGGING
-- ═══════════════════════════════════════════════════════════════════════════════

-- Store purchase logging
RegisterNetEvent('zr-carboosting:server:purchaseItem', function(itemId)
    local src = source
    local Player = exports['qb-core']:GetCoreObject().Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    local playerData = exports['zr-carboosting']:GetPlayerBoostingData(citizenid)
    
    -- Find item
    local item = nil
    for _, i in ipairs(Config.Store.items) do
        if i.id == itemId then
            item = i
            break
        end
    end
    
    if not item then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Item not found!', 'error')
        return
    end
    
    -- Check level
    if playerData.level < item.requiredLevel then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Level requirement not met!', 'error')
        return
    end
    
    -- Check currency
    if playerData.currency < item.price then
        TriggerClientEvent('zr-carboosting:client:notify', src, Config.Notifications.messages.insufficient_funds, 'error')
        return
    end
    
    -- Deduct currency
    exports['zr-carboosting']:UpdatePlayerStats(citizenid, {
        currency = -item.price
    })
    
    -- Record purchase
    MySQL.Async.execute([[
        INSERT INTO boosting_purchases (citizenid, item_id, amount, price)
        VALUES (?, ?, ?, ?)
    ]], { citizenid, itemId, 1, item.price })
    
    -- Start drone delivery
    if Config.Drone.enabled then
        local deliveryLocation = GetRandomDroneLocation()
        local deliveryId = 'DEL-' .. Utils.RandomString(8)
        local deliveryTime = math.random(Config.Drone.minDeliveryTime, Config.Drone.maxDeliveryTime)
        
        TriggerClientEvent('zr-carboosting:client:notify', src, Config.Notifications.messages.purchase_success, 'success')
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Delivery in ' .. math.floor(deliveryTime / 60) .. ' minutes', 'primary')
        
        -- Schedule delivery
        SetTimeout(deliveryTime * 1000, function()
            local TargetPlayer = exports['qb-core']:GetCoreObject().Functions.GetPlayer(src)
            if TargetPlayer then
                TriggerClientEvent('zr-carboosting:client:startDroneDelivery', src, deliveryId, deliveryLocation.coords, {
                    { item = item.id, amount = 1 }
                })
            end
        end)
    else
        -- Direct give item
        Player.Functions.AddItem(item.id, 1)
        TriggerClientEvent('inventory:client:ItemBox', src, exports['qb-core']:GetCoreObject().Shared.Items[item.id], 'add', 1)
        TriggerClientEvent('zr-carboosting:client:notify', src, Config.Notifications.messages.purchase_success, 'success')
    end
    
    -- Log
    LogPurchase(citizenid, Player.PlayerData.charinfo.firstname .. ' ' .. Player.PlayerData.charinfo.lastname, item)
end)

RegisterNetEvent('zr-carboosting:server:collectPackage', function(deliveryId)
    local src = source
    local Player = exports['qb-core']:GetCoreObject().Functions.GetPlayer(src)
    if not Player then return end
    
    -- Items should be given here based on delivery tracking
    -- For now, simplified
end)

function LogPurchase(citizenid, playerName, item)
    if not Config.Logging.enabled or not Config.Logging.logEvents.storePurchase then return end
    
    local webhook = Config.Logging.webhook
    if not webhook or webhook == '' then return end
    
    PerformHttpRequest(webhook, function() end, 'POST', json.encode({
        username = 'Boosting Logs',
        embeds = {{
            title = '🛒 Store Purchase',
            color = 5763719,
            fields = {
                { name = 'Player', value = playerName, inline = true },
                { name = 'CitizenID', value = citizenid, inline = true },
                { name = 'Item', value = item.label, inline = true },
                { name = 'Price', value = '$' .. tostring(item.price), inline = true }
            },
            footer = {
                text = Config.Logging.serverName .. ' | ' .. os.date('%Y-%m-%d %H:%M:%S')
            }
        }}
    }), { ['Content-Type'] = 'application/json' })
end
