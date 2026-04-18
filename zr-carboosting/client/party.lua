-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT PARTY - PARTY/CREW SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local PartyData = {
    inParty = false,
    isLeader = false,
    partyId = nil,
    members = {},
    pendingInvites = {}
}

local MemberBlips = {}

-- ═══════════════════════════════════════════════════════════════════════════════
-- PARTY EVENTS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:partyCreated', function(partyId)
    PartyData.inParty = true
    PartyData.isLeader = true
    PartyData.partyId = partyId
    PartyData.members = { QBCore.Functions.GetPlayerData().citizenid }
    
    QBCore.Functions.Notify('Crew created! Invite players from the tablet.', 'success')
    
    SendNUIMessage({
        action = 'partyUpdate',
        data = GetPartyUIData()
    })
end)

RegisterNetEvent('zr-carboosting:client:partyInvite', function(inviteData)
    -- Show invite notification
    PartyData.pendingInvites[inviteData.partyId] = inviteData
    
    local message = string.format(Config.Notifications.messages.party_invite, inviteData.leaderName)
    
    -- Use ox_lib dialog or custom notification
    lib.notify({
        title = 'Crew Invite',
        description = message,
        type = 'inform',
        duration = 10000
    })
    
    -- Auto-show accept dialog
    local alert = lib.alertDialog({
        header = 'Crew Invitation',
        content = inviteData.leaderName .. ' has invited you to join their boosting crew.\n\nDo you want to accept?',
        centered = true,
        cancel = true
    })
    
    if alert == 'confirm' then
        TriggerServerEvent('zr-carboosting:server:acceptPartyInvite', inviteData.partyId)
    else
        TriggerServerEvent('zr-carboosting:server:declinePartyInvite', inviteData.partyId)
    end
    
    PartyData.pendingInvites[inviteData.partyId] = nil
end)

RegisterNetEvent('zr-carboosting:client:partyJoined', function(partyId, members, isLeader)
    PartyData.inParty = true
    PartyData.isLeader = isLeader or false
    PartyData.partyId = partyId
    PartyData.members = members
    
    QBCore.Functions.Notify('You joined the crew!', 'success')
    
    SendNUIMessage({
        action = 'partyUpdate',
        data = GetPartyUIData()
    })
    
    -- Start member blip updates
    if Config.Party.shareBlips then
        StartMemberBlips()
    end
end)

RegisterNetEvent('zr-carboosting:client:partyMemberJoined', function(memberData)
    if not PartyData.inParty then return end
    
    table.insert(PartyData.members, memberData)
    
    local message = string.format(Config.Notifications.messages.party_joined, memberData.name)
    QBCore.Functions.Notify(message, 'primary')
    
    SendNUIMessage({
        action = 'partyUpdate',
        data = GetPartyUIData()
    })
end)

RegisterNetEvent('zr-carboosting:client:partyMemberLeft', function(citizenId, name)
    if not PartyData.inParty then return end
    
    -- Remove from members
    for i, member in ipairs(PartyData.members) do
        if member.citizenid == citizenId then
            table.remove(PartyData.members, i)
            break
        end
    end
    
    -- Remove blip
    if MemberBlips[citizenId] then
        RemoveBlip(MemberBlips[citizenId])
        MemberBlips[citizenId] = nil
    end
    
    local message = string.format(Config.Notifications.messages.party_left, name)
    QBCore.Functions.Notify(message, 'primary')
    
    SendNUIMessage({
        action = 'partyUpdate',
        data = GetPartyUIData()
    })
end)

RegisterNetEvent('zr-carboosting:client:partyDisbanded', function()
    CleanupParty()
    QBCore.Functions.Notify('The crew has been disbanded.', 'error')
end)

RegisterNetEvent('zr-carboosting:client:leftParty', function()
    CleanupParty()
    QBCore.Functions.Notify('You left the crew.', 'primary')
end)

RegisterNetEvent('zr-carboosting:client:promotedToLeader', function()
    PartyData.isLeader = true
    QBCore.Functions.Notify('You are now the crew leader!', 'success')
    
    SendNUIMessage({
        action = 'partyUpdate',
        data = GetPartyUIData()
    })
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- MEMBER BLIPS
-- ═══════════════════════════════════════════════════════════════════════════════

function StartMemberBlips()
    CreateThread(function()
        while PartyData.inParty and Config.Party.shareBlips do
            -- Request member positions from server
            TriggerServerEvent('zr-carboosting:server:requestMemberPositions', PartyData.partyId)
            Wait(2000)
        end
    end)
end

RegisterNetEvent('zr-carboosting:client:updateMemberPositions', function(positions)
    if not PartyData.inParty then return end
    
    local myId = QBCore.Functions.GetPlayerData().citizenid
    
    for citizenId, pos in pairs(positions) do
        if citizenId ~= myId then
            if not MemberBlips[citizenId] then
                -- Create new blip
                local blip = AddBlipForCoord(pos.x, pos.y, pos.z)
                SetBlipSprite(blip, 1)
                SetBlipColour(blip, 3) -- Blue
                SetBlipScale(blip, 0.8)
                SetBlipAsShortRange(blip, false)
                BeginTextCommandSetBlipName('STRING')
                AddTextComponentString('Crew Member')
                EndTextCommandSetBlipName(blip)
                
                MemberBlips[citizenId] = blip
            else
                -- Update blip position
                SetBlipCoords(MemberBlips[citizenId], pos.x, pos.y, pos.z)
            end
        end
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- SHARED OBJECTIVES
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:client:shareContractObjective', function(objectiveType, data)
    if not PartyData.inParty then return end
    
    -- Share contract info with party members
    if objectiveType == 'vehicle_found' then
        QBCore.Functions.Notify('A crew member found the target vehicle!', 'success')
    elseif objectiveType == 'gps_disabled' then
        QBCore.Functions.Notify('GPS tracker disabled by crew member!', 'success')
    elseif objectiveType == 'destination_set' then
        -- Add waypoint if not the one who set it
        if data.coords then
            SetNewWaypoint(data.coords.x, data.coords.y)
            QBCore.Functions.Notify('Destination shared by crew member', 'primary')
        end
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function GetPartyUIData()
    return {
        inParty = PartyData.inParty,
        isLeader = PartyData.isLeader,
        partyId = PartyData.partyId,
        members = PartyData.members,
        maxMembers = Config.Party.maxMembers
    }
end

function CleanupParty()
    -- Remove all member blips
    for _, blip in pairs(MemberBlips) do
        RemoveBlip(blip)
    end
    MemberBlips = {}
    
    -- Reset party data
    PartyData = {
        inParty = false,
        isLeader = false,
        partyId = nil,
        members = {},
        pendingInvites = {}
    }
    
    SendNUIMessage({
        action = 'partyUpdate',
        data = GetPartyUIData()
    })
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP
-- ═══════════════════════════════════════════════════════════════════════════════

AddEventHandler('onResourceStop', function(resource)
    if resource == GetCurrentResourceName() then
        for _, blip in pairs(MemberBlips) do
            RemoveBlip(blip)
        end
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetPartyData', function()
    return PartyData
end)

exports('IsInParty', function()
    return PartyData.inParty
end)

exports('IsPartyLeader', function()
    return PartyData.isLeader
end)
