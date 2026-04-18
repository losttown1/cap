-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER PARTY - PARTY/CREW SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- PARTY STORAGE
-- ═══════════════════════════════════════════════════════════════════════════════

local Parties = {}
local PlayerParties = {} -- citizenid -> partyId mapping
local PendingInvites = {} -- citizenid -> { partyId, expiry }

-- ═══════════════════════════════════════════════════════════════════════════════
-- CREATE PARTY
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:createParty', function()
    if not Config.Party.enabled then return end
    
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    
    -- Check if already in party
    if PlayerParties[citizenid] then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'You are already in a crew!', 'error')
        return
    end
    
    -- Generate party ID
    local partyId = 'CREW-' .. Utils.RandomString(8)
    
    -- Create party
    Parties[partyId] = {
        id = partyId,
        leader = citizenid,
        members = {
            {
                citizenid = citizenid,
                name = Player.PlayerData.charinfo.firstname .. ' ' .. Player.PlayerData.charinfo.lastname,
                source = src,
                isLeader = true
            }
        },
        createdAt = os.time()
    }
    
    PlayerParties[citizenid] = partyId
    
    TriggerClientEvent('zr-carboosting:client:partyCreated', src, partyId)
    
    Utils.Debug('Party created:', partyId, 'Leader:', citizenid)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- INVITE TO PARTY
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:inviteToParty', function(targetSource)
    if not Config.Party.enabled then return end
    
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    local TargetPlayer = QBCore.Functions.GetPlayer(targetSource)
    
    if not Player or not TargetPlayer then return end
    
    local citizenid = Player.PlayerData.citizenid
    local targetCitizenid = TargetPlayer.PlayerData.citizenid
    
    -- Check if in party
    local partyId = PlayerParties[citizenid]
    if not partyId then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'You are not in a crew!', 'error')
        return
    end
    
    local party = Parties[partyId]
    if not party then return end
    
    -- Check if leader
    if party.leader ~= citizenid then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Only the leader can invite!', 'error')
        return
    end
    
    -- Check party size
    if #party.members >= Config.Party.maxMembers then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Crew is full!', 'error')
        return
    end
    
    -- Check if target already in party
    if PlayerParties[targetCitizenid] then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Player is already in a crew!', 'error')
        return
    end
    
    -- Send invite
    PendingInvites[targetCitizenid] = {
        partyId = partyId,
        expiry = os.time() + Config.Party.inviteTimeout
    }
    
    TriggerClientEvent('zr-carboosting:client:partyInvite', targetSource, {
        partyId = partyId,
        leaderName = Player.PlayerData.charinfo.firstname .. ' ' .. Player.PlayerData.charinfo.lastname
    })
    
    TriggerClientEvent('zr-carboosting:client:notify', src, 'Invite sent!', 'success')
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- ACCEPT/DECLINE INVITE
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:acceptPartyInvite', function(partyId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    
    -- Check invite
    local invite = PendingInvites[citizenid]
    if not invite or invite.partyId ~= partyId then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Invalid invite!', 'error')
        return
    end
    
    -- Check expiry
    if os.time() > invite.expiry then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Invite expired!', 'error')
        PendingInvites[citizenid] = nil
        return
    end
    
    local party = Parties[partyId]
    if not party then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Crew no longer exists!', 'error')
        PendingInvites[citizenid] = nil
        return
    end
    
    -- Check party size
    if #party.members >= Config.Party.maxMembers then
        TriggerClientEvent('zr-carboosting:client:notify', src, 'Crew is full!', 'error')
        PendingInvites[citizenid] = nil
        return
    end
    
    -- Add to party
    local memberData = {
        citizenid = citizenid,
        name = Player.PlayerData.charinfo.firstname .. ' ' .. Player.PlayerData.charinfo.lastname,
        source = src,
        isLeader = false
    }
    
    table.insert(party.members, memberData)
    PlayerParties[citizenid] = partyId
    PendingInvites[citizenid] = nil
    
    -- Notify all members
    for _, member in ipairs(party.members) do
        if member.source and member.citizenid ~= citizenid then
            TriggerClientEvent('zr-carboosting:client:partyMemberJoined', member.source, memberData)
        end
    end
    
    -- Notify new member
    TriggerClientEvent('zr-carboosting:client:partyJoined', src, partyId, party.members, false)
end)

RegisterNetEvent('zr-carboosting:server:declinePartyInvite', function(partyId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    PendingInvites[Player.PlayerData.citizenid] = nil
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- LEAVE PARTY
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:leaveParty', function()
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local citizenid = Player.PlayerData.citizenid
    local partyId = PlayerParties[citizenid]
    
    if not partyId then return end
    
    local party = Parties[partyId]
    if not party then
        PlayerParties[citizenid] = nil
        return
    end
    
    local playerName = Player.PlayerData.charinfo.firstname .. ' ' .. Player.PlayerData.charinfo.lastname
    
    -- Check if leader
    if party.leader == citizenid then
        -- Disband or transfer leadership
        if #party.members > 1 then
            -- Find new leader
            for i, member in ipairs(party.members) do
                if member.citizenid == citizenid then
                    table.remove(party.members, i)
                    break
                end
            end
            
            -- Transfer to first remaining member
            local newLeader = party.members[1]
            party.leader = newLeader.citizenid
            newLeader.isLeader = true
            
            -- Notify new leader
            TriggerClientEvent('zr-carboosting:client:promotedToLeader', newLeader.source)
            
            -- Notify all members
            for _, member in ipairs(party.members) do
                TriggerClientEvent('zr-carboosting:client:partyMemberLeft', member.source, citizenid, playerName)
            end
        else
            -- Disband party
            for _, member in ipairs(party.members) do
                PlayerParties[member.citizenid] = nil
                TriggerClientEvent('zr-carboosting:client:partyDisbanded', member.source)
            end
            Parties[partyId] = nil
        end
    else
        -- Remove member
        for i, member in ipairs(party.members) do
            if member.citizenid == citizenid then
                table.remove(party.members, i)
                break
            end
        end
        
        -- Notify remaining members
        for _, member in ipairs(party.members) do
            TriggerClientEvent('zr-carboosting:client:partyMemberLeft', member.source, citizenid, playerName)
        end
    end
    
    PlayerParties[citizenid] = nil
    TriggerClientEvent('zr-carboosting:client:leftParty', src)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- MEMBER POSITIONS
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:requestMemberPositions', function(partyId)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local party = Parties[partyId]
    if not party then return end
    
    local positions = {}
    
    for _, member in ipairs(party.members) do
        local MemberPlayer = QBCore.Functions.GetPlayer(member.source)
        if MemberPlayer then
            local ped = GetPlayerPed(member.source)
            if ped then
                local coords = GetEntityCoords(ped)
                positions[member.citizenid] = {
                    x = coords.x,
                    y = coords.y,
                    z = coords.z
                }
            end
        end
    end
    
    TriggerClientEvent('zr-carboosting:client:updateMemberPositions', src, positions)
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- SHARE OBJECTIVES
-- ═══════════════════════════════════════════════════════════════════════════════

RegisterNetEvent('zr-carboosting:server:shareObjective', function(objectiveType, data)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    if not Player then return end
    
    local partyId = PlayerParties[Player.PlayerData.citizenid]
    if not partyId then return end
    
    local party = Parties[partyId]
    if not party then return end
    
    -- Share with all members except sender
    for _, member in ipairs(party.members) do
        if member.source ~= src then
            TriggerClientEvent('zr-carboosting:client:shareContractObjective', member.source, objectiveType, data)
        end
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CLEANUP ON DISCONNECT
-- ═══════════════════════════════════════════════════════════════════════════════

AddEventHandler('playerDropped', function(reason)
    local src = source
    local Player = QBCore.Functions.GetPlayer(src)
    
    if Player then
        local citizenid = Player.PlayerData.citizenid
        
        -- Handle party membership
        local partyId = PlayerParties[citizenid]
        if partyId then
            -- Auto-leave party
            TriggerEvent('zr-carboosting:server:leaveParty')
        end
        
        -- Clear pending invites
        PendingInvites[citizenid] = nil
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('GetPartyForPlayer', function(citizenid)
    local partyId = PlayerParties[citizenid]
    if partyId then
        return Parties[partyId]
    end
    return nil
end)

exports('GetParty', function(partyId)
    return Parties[partyId]
end)

exports('IsInParty', function(citizenid)
    return PlayerParties[citizenid] ~= nil
end)
