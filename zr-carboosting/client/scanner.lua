-- ═══════════════════════════════════════════════════════════════════════════════
-- CLIENT SCANNER - DIGISCANNER FOR VEHICLE SEARCH
-- ═══════════════════════════════════════════════════════════════════════════════

local QBCore = exports['qb-core']:GetCoreObject()

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCAL VARIABLES
-- ═══════════════════════════════════════════════════════════════════════════════

local ScannerActive = false
local LastScanTime = 0
local ScannerScaleform = nil

-- ═══════════════════════════════════════════════════════════════════════════════
-- SCANNER LOOP
-- ═══════════════════════════════════════════════════════════════════════════════

CreateThread(function()
    while true do
        local sleep = 1000
        
        if Config.Scanner.enabled then
            local ActiveContract = exports['zr-carboosting']:GetActiveContract()
            
            if ActiveContract and ActiveContract.vehicleCoords then
                local ped = PlayerPedId()
                local currentWeapon = GetSelectedPedWeapon(ped)
                
                -- Check if holding scanner
                if currentWeapon == GetHashKey(Config.Scanner.weapon) then
                    sleep = Config.Scanner.updateInterval
                    
                    local playerCoords = GetEntityCoords(ped)
                    local targetCoords = ActiveContract.vehicleCoords
                    local distance = #(playerCoords - vector3(targetCoords.x, targetCoords.y, targetCoords.z))
                    
                    -- Get color based on distance
                    local color = GetScannerColor(distance)
                    
                    -- Draw scanner HUD
                    DrawScannerHUD(distance, color)
                    
                    -- Vibration effect (controller)
                    local vibration = Config.Scanner.vibrationIntensity(distance)
                    if vibration > 0 then
                        SetPadShake(0, math.floor(vibration / 2), math.floor(vibration))
                    end
                    
                    -- Beep sound based on distance
                    if GetGameTimer() - LastScanTime > GetBeepInterval(distance) then
                        PlayScannerBeep(distance)
                        LastScanTime = GetGameTimer()
                    end
                    
                    ScannerActive = true
                else
                    if ScannerActive then
                        ScannerActive = false
                        StopPadShake(0)
                    end
                end
            end
        end
        
        Wait(sleep)
    end
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- SCANNER FUNCTIONS
-- ═══════════════════════════════════════════════════════════════════════════════

function GetScannerColor(distance)
    for _, threshold in ipairs(Config.Scanner.distances) do
        if distance <= threshold.max then
            return threshold.color
        end
    end
    return { r = 255, g = 0, b = 0 }
end

function GetBeepInterval(distance)
    -- Closer = faster beeps
    local minInterval = 100
    local maxInterval = 1500
    local maxDistance = 300
    
    local interval = minInterval + (distance / maxDistance) * (maxInterval - minInterval)
    return math.min(maxInterval, math.max(minInterval, interval))
end

function PlayScannerBeep(distance)
    local pitch = 1.0 + (1.0 - math.min(distance / 200, 1.0)) * 0.5
    
    if distance < 25 then
        PlaySoundFrontend(-1, 'TIMER_STOP', 'HUD_MINI_GAME_SOUNDSET', true)
    elseif distance < 100 then
        PlaySoundFrontend(-1, 'NAV_UP_DOWN', 'HUD_FRONTEND_DEFAULT_SOUNDSET', true)
    else
        PlaySoundFrontend(-1, 'SELECT', 'HUD_FRONTEND_DEFAULT_SOUNDSET', true)
    end
end

function DrawScannerHUD(distance, color)
    -- Background
    DrawRect(0.5, 0.93, 0.25, 0.06, 0, 0, 0, 150)
    
    -- Signal strength bar
    local strength = math.max(0, 1.0 - (distance / 300))
    local barWidth = 0.2 * strength
    
    DrawRect(0.4 + barWidth/2, 0.93, barWidth, 0.03, color.r, color.g, color.b, 200)
    
    -- Distance text
    SetTextScale(0.4, 0.4)
    SetTextFont(4)
    SetTextProportional(1)
    SetTextColour(color.r, color.g, color.b, 255)
    SetTextDropShadow(0, 0, 0, 0, 255)
    SetTextEdge(2, 0, 0, 0, 150)
    SetTextDropShadow()
    SetTextOutline()
    SetTextEntry('STRING')
    SetTextCentre(1)
    
    local distText = string.format('%.0fm', distance)
    if distance < 25 then
        distText = 'VERY CLOSE!'
    elseif distance < 50 then
        distText = 'CLOSE - ' .. distText
    elseif distance < 100 then
        distText = 'NEARBY - ' .. distText
    else
        distText = 'SEARCHING - ' .. distText
    end
    
    AddTextComponentString(distText)
    DrawText(0.5, 0.905)
    
    -- Signal indicator dots
    local dotCount = 5
    local activeDots = math.ceil(strength * dotCount)
    
    for i = 1, dotCount do
        local dotX = 0.55 + (i - 1) * 0.02
        local alpha = i <= activeDots and 255 or 50
        DrawRect(dotX, 0.93, 0.012, 0.02, color.r, color.g, color.b, alpha)
    end
    
    -- Pulsing effect when very close
    if distance < 25 then
        local pulse = (math.sin(GetGameTimer() / 100) + 1) / 2
        DrawRect(0.5, 0.93, 0.26, 0.065, color.r, color.g, color.b, math.floor(pulse * 100))
    end
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- DIRECTIONAL INDICATOR
-- ═══════════════════════════════════════════════════════════════════════════════

CreateThread(function()
    while true do
        local sleep = 500
        
        if ScannerActive then
            sleep = 0
            
            local ActiveContract = exports['zr-carboosting']:GetActiveContract()
            
            if ActiveContract and ActiveContract.vehicleCoords then
                local ped = PlayerPedId()
                local playerCoords = GetEntityCoords(ped)
                local playerHeading = GetEntityHeading(ped)
                local targetCoords = ActiveContract.vehicleCoords
                
                -- Calculate direction to target
                local dx = targetCoords.x - playerCoords.x
                local dy = targetCoords.y - playerCoords.y
                local targetAngle = math.deg(math.atan(dy, dx))
                
                -- Normalize angles
                targetAngle = (90 - targetAngle) % 360
                local relativeAngle = (targetAngle - playerHeading + 360) % 360
                
                -- Draw direction indicator
                DrawDirectionalIndicator(relativeAngle)
            end
        end
        
        Wait(sleep)
    end
end)

function DrawDirectionalIndicator(angle)
    local centerX = 0.5
    local centerY = 0.85
    local radius = 0.03
    
    -- Convert angle to radians
    local rad = math.rad(angle - 90) -- Adjust for screen coordinates
    
    -- Calculate arrow position
    local arrowX = centerX + math.cos(rad) * radius
    local arrowY = centerY + math.sin(rad) * radius * 1.78 -- Aspect ratio correction
    
    -- Draw compass background
    DrawRect(centerX, centerY, 0.08, 0.08, 0, 0, 0, 100)
    
    -- Draw direction line
    local color = GetScannerColor(100) -- Use mid-range color
    
    -- Draw arrow
    DrawLine(centerX, centerY, 0.0, arrowX, arrowY, 0.0, color.r, color.g, color.b, 255)
    
    -- Arrow head (simple dot)
    DrawRect(arrowX, arrowY, 0.008, 0.012, color.r, color.g, color.b, 255)
    
    -- Center dot
    DrawRect(centerX, centerY, 0.005, 0.008, 255, 255, 255, 200)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('IsScannerActive', function()
    return ScannerActive
end)
