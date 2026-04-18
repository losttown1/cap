-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER ANTICHEAT - ANTI-EXPLOIT SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- RATE LIMITING
-- ═══════════════════════════════════════════════════════════════════════════════

local EventRates = {} -- source -> { eventName -> { count, lastReset } }
local Violations = {} -- source -> violation count

local function CheckRateLimit(source, eventName)
    if not Config.AntiExploit.enabled then return true end
    
    local now = GetGameTimer()
    
    if not EventRates[source] then
        EventRates[source] = {}
    end
    
    if not EventRates[source][eventName] then
        EventRates[source][eventName] = { count = 0, lastReset = now }
    end
    
    local eventData = EventRates[source][eventName]
    
    -- Reset counter every minute
    if now - eventData.lastReset > 60000 then
        eventData.count = 0
        eventData.lastReset = now
    end
    
    eventData.count = eventData.count + 1
    
    -- Check rate
    if eventData.count > Config.AntiExploit.maxEventsPerMinute then
        HandleViolation(source, 'rate_limit', eventName)
        return false
    end
    
    return true
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- VALIDATION FUNCTIONS
-- ═══════════════════════════════════════════════════════════════════════════════

local function ValidateContract(source, contractId)
    local Player = QBCore.Functions.GetPlayer(source)
    if not Player then return false, 'no_player' end
    
    local citizenid = Player.PlayerData.citizenid
    local contract = exports['zr-carboosting']:GetActiveContract(citizenid)
    
    if not contract then
        return false, 'no_contract'
    end
    
    if contract.id ~= contractId then
        return false, 'wrong_contract'
    end
    
    return true, contract
end

local function ValidateDistance(source, targetCoords, maxDistance)
    if not Config.AntiExploit.validateDistance then return true end
    
    local ped = GetPlayerPed(source)
    if not ped then return false end
    
    local playerCoords = GetEntityCoords(ped)
    local distance = #(playerCoords - vector3(targetCoords.x, targetCoords.y, targetCoords.z))
    
    return distance <= (maxDistance or Config.AntiExploit.maxValidDistance)
end

local function ValidatePlate(source, plate, contractId)
    if not Config.AntiExploit.validatePlate then return true end
    
    local valid, contract = ValidateContract(source, contractId)
    if not valid then return false end
    
    local cleanPlate = plate:gsub('%s+', '')
    local contractPlate = contract.plate:gsub('%s+', '')
    
    return cleanPlate == contractPlate
end

local function ValidateVehicleOwnership(source, vehicleNetId, contractId)
    if not Config.AntiExploit.validateOwnership then return true end
    
    local valid, contract = ValidateContract(source, contractId)
    if not valid then return false end
    
    return contract.vehicleNetId == vehicleNetId
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- VIOLATION HANDLING
-- ═══════════════════════════════════════════════════════════════════════════════

function HandleViolation(source, violationType, details)
    local Player = QBCore.Functions.GetPlayer(source)
    local playerName = Player and (Player.PlayerData.charinfo.firstname .. ' ' .. Player.PlayerData.charinfo.lastname) or 'Unknown'
    local citizenid = Player and Player.PlayerData.citizenid or 'Unknown'
    
    -- Increment violations
    Violations[source] = (Violations[source] or 0) + 1
    
    -- Log violation
    if Config.AntiExploit.actions.log then
        LogExploit(source, playerName, citizenid, violationType, details)
    end
    
    -- Notify admins
    if Config.AntiExploit.actions.notify_admins then
        NotifyAdmins(source, playerName, violationType, details)
    end
    
    -- Kick after multiple violations
    if Config.AntiExploit.actions.kick and Violations[source] >= 3 then
        DropPlayer(source, 'Kicked for suspicious activity')
    end
    
    -- Ban (if enabled)
    if Config.AntiExploit.actions.ban and Violations[source] >= 5 then
        -- Implement ban logic here (depends on your ban system)
        DropPlayer(source, 'Banned for exploit attempt')
    end
    
    Utils.Debug('Violation detected:', violationType, 'Source:', source, 'Details:', details)
end

function LogExploit(source, playerName, citizenid, violationType, details)
    local webhook = Config.AntiExploit.adminWebhook
    if not webhook or webhook == '' then return end
    
    local embed = {
        title = '⚠️ Exploit Detection',
        color = 16711680, -- Red
        fields = {
            { name = 'Player', value = playerName, inline = true },
            { name = 'CitizenID', value = citizenid, inline = true },
            { name = 'Server ID', value = tostring(source), inline = true },
            { name = 'Violation Type', value = violationType, inline = true },
            { name = 'Details', value = tostring(details or 'N/A'), inline = false }
        },
        footer = {
            text = os.date('%Y-%m-%d %H:%M:%S')
        }
    }
    
    PerformHttpRequest(webhook, function() end, 'POST', json.encode({
        username = 'Anti-Exploit',
        embeds = { embed }
    }), { ['Content-Type'] = 'application/json' })
end

function NotifyAdmins(source, playerName, violationType, details)
    local players = QBCore.Functions.GetQBPlayers()
    
    for _, player in pairs(players) do
        if player then
            local job = player.PlayerData.job
            if job and Utils.TableContains(Config.AdminGroups or {'admin'}, job.name) then
                TriggerClientEvent('zr-carboosting:client:notify', player.PlayerData.source, 
                    '⚠️ Exploit: ' .. playerName .. ' - ' .. violationType, 'error')
            end
        end
    end
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- SECURE EVENT WRAPPER
-- ═══════════════════════════════════════════════════════════════════════════════

function SecureEvent(eventName, handler, validations)
    RegisterNetEvent(eventName, function(...)
        local src = source
        
        -- Rate limit check
        if not CheckRateLimit(src, eventName) then
            Utils.Debug('Rate limited:', eventName, 'Source:', src)
            return
        end
        
        -- Run validations
        if validations then
            local args = {...}
            
            for _, validation in ipairs(validations) do
                local passed = false
                
                if validation.type == 'contract' then
                    passed = ValidateContract(src, args[validation.argIndex or 1])
                elseif validation.type == 'distance' then
                    passed = ValidateDistance(src, args[validation.argIndex or 1], validation.maxDistance)
                elseif validation.type == 'plate' then
                    passed = ValidatePlate(src, args[validation.argIndex or 1], args[validation.contractArgIndex or 2])
                elseif validation.type == 'ownership' then
                    passed = ValidateVehicleOwnership(src, args[validation.argIndex or 1], args[validation.contractArgIndex or 2])
                end
                
                if not passed then
                    HandleViolation(src, 'validation_failed', eventName .. ' - ' .. validation.type)
                    return
                end
            end
        end
        
        -- Call handler
        handler(src, ...)
    end)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP
-- ═══════════════════════════════════════════════════════════════════════════════

AddEventHandler('playerDropped', function(reason)
    local src = source
    EventRates[src] = nil
    Violations[src] = nil
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('ValidateContract', ValidateContract)
exports('ValidateDistance', ValidateDistance)
exports('ValidatePlate', ValidatePlate)
exports('HandleViolation', HandleViolation)
exports('SecureEvent', SecureEvent)
