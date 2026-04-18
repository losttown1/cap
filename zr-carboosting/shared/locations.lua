-- ═══════════════════════════════════════════════════════════════════════════════
-- SPAWN LOCATIONS
-- ═══════════════════════════════════════════════════════════════════════════════

Config.Locations = {
    -- ═══════════════════════════════════════════════════════════════════════════
    -- VEHICLE SPAWN ZONES (Weighted by class)
    -- ═══════════════════════════════════════════════════════════════════════════
    spawnZones = {
        -- Poor Areas (Class D, C)
        {
            coords = vector3(100.0, -1940.0, 20.0),
            radius = 200,
            classes = {'D', 'C'},
            weight = 10,
            name = 'Strawberry'
        },
        {
            coords = vector3(310.0, -1780.0, 29.0),
            radius = 180,
            classes = {'D', 'C'},
            weight = 10,
            name = 'Davis'
        },
        {
            coords = vector3(-150.0, -1650.0, 33.0),
            radius = 200,
            classes = {'D', 'C'},
            weight = 8,
            name = 'South LS'
        },
        {
            coords = vector3(420.0, -2050.0, 22.0),
            radius = 150,
            classes = {'D', 'C'},
            weight = 8,
            name = 'Rancho'
        },
        
        -- Mid Areas (Class B, C)
        {
            coords = vector3(-200.0, -800.0, 30.0),
            radius = 200,
            classes = {'B', 'C'},
            weight = 8,
            name = 'Pillbox Hill'
        },
        {
            coords = vector3(150.0, -1000.0, 29.0),
            radius = 180,
            classes = {'B', 'C'},
            weight = 7,
            name = 'Mission Row'
        },
        {
            coords = vector3(-500.0, -600.0, 30.0),
            radius = 200,
            classes = {'B', 'C', 'A'},
            weight = 7,
            name = 'Little Seoul'
        },
        {
            coords = vector3(950.0, -1700.0, 30.0),
            radius = 200,
            classes = {'B', 'C'},
            weight = 6,
            name = 'La Mesa'
        },
        
        -- Wealthy Areas (Class A, S)
        {
            coords = vector3(-1350.0, -450.0, 30.0),
            radius = 250,
            classes = {'A', 'S'},
            weight = 6,
            name = 'Morningwood'
        },
        {
            coords = vector3(-1600.0, -300.0, 45.0),
            radius = 200,
            classes = {'A', 'S'},
            weight = 7,
            name = 'Del Perro'
        },
        {
            coords = vector3(-1850.0, -350.0, 40.0),
            radius = 180,
            classes = {'A', 'S', 'S+'},
            weight = 8,
            name = 'Del Perro Beach'
        },
        {
            coords = vector3(-100.0, -600.0, 35.0),
            radius = 200,
            classes = {'A', 'S'},
            weight = 6,
            name = 'Downtown LS'
        },
        
        -- Premium Areas (Class S, S+)
        {
            coords = vector3(-1700.0, 250.0, 60.0),
            radius = 300,
            classes = {'S', 'S+'},
            weight = 10,
            name = 'Richman'
        },
        {
            coords = vector3(-1500.0, 850.0, 180.0),
            radius = 300,
            classes = {'S', 'S+'},
            weight = 10,
            name = 'Richman Glen'
        },
        {
            coords = vector3(-2200.0, 250.0, 170.0),
            radius = 250,
            classes = {'S+'},
            weight = 10,
            name = 'Pacific Bluffs'
        },
        {
            coords = vector3(-800.0, 800.0, 200.0),
            radius = 350,
            classes = {'S', 'S+'},
            weight = 9,
            name = 'Vinewood Hills'
        },
        {
            coords = vector3(100.0, 500.0, 145.0),
            radius = 200,
            classes = {'S', 'S+'},
            weight = 8,
            name = 'Mirror Park'
        }
    },
    
    -- ═══════════════════════════════════════════════════════════════════════════
    -- DROP-OFF LOCATIONS
    -- ═══════════════════════════════════════════════════════════════════════════
    dropOff = {
        {
            coords = vector3(485.0, -1310.0, 29.5),
            heading = 270.0,
            npcModel = 's_m_y_dealer_01',
            name = 'La Mesa Warehouse'
        },
        {
            coords = vector3(860.0, -2350.0, 30.5),
            heading = 90.0,
            npcModel = 'g_m_y_mexgoon_01',
            name = 'South LS Docks'
        },
        {
            coords = vector3(-595.0, -1620.0, 26.5),
            heading = 180.0,
            npcModel = 'g_m_y_korean_01',
            name = 'Airport Hangar'
        },
        {
            coords = vector3(155.0, -3240.0, 6.0),
            heading = 270.0,
            npcModel = 's_m_y_dealer_01',
            name = 'Terminal Dock'
        },
        {
            coords = vector3(1215.0, -2885.0, 5.9),
            heading = 180.0,
            npcModel = 'g_m_y_mexgoon_02',
            name = 'Elysian Island'
        },
        {
            coords = vector3(2680.0, 1460.0, 24.5),
            heading = 90.0,
            npcModel = 'g_m_m_mexboss_02',
            name = 'Grand Senora'
        },
        {
            coords = vector3(2430.0, 4970.0, 46.8),
            heading = 45.0,
            npcModel = 's_m_y_dealer_01',
            name = 'Grapeseed'
        }
    },
    
    -- ═══════════════════════════════════════════════════════════════════════════
    -- CHOP SHOP LOCATIONS
    -- ═══════════════════════════════════════════════════════════════════════════
    chopShops = {
        {
            coords = vector3(485.0, -1340.0, 29.5),
            heading = 180.0,
            blip = true,
            name = 'La Mesa Chop Shop'
        },
        {
            coords = vector3(-595.0, -1640.0, 26.5),
            heading = 270.0,
            blip = true,
            name = 'Airport Chop Shop'
        },
        {
            coords = vector3(1110.0, -2005.0, 31.0),
            heading = 90.0,
            blip = false,
            name = 'Hidden Chop Shop'
        },
        {
            coords = vector3(730.0, -1335.0, 26.3),
            heading = 0.0,
            blip = true,
            name = 'East LS Chop Shop'
        }
    },
    
    -- ═══════════════════════════════════════════════════════════════════════════
    -- VIN SCRATCH LOCATIONS
    -- ═══════════════════════════════════════════════════════════════════════════
    vinLocations = {
        {
            coords = vector3(970.0, -1830.0, 31.2),
            heading = 0.0,
            name = 'Underground Garage'
        },
        {
            coords = vector3(-355.0, -130.0, 39.0),
            heading = 90.0,
            name = 'Burton Basement'
        },
        {
            coords = vector3(538.0, -1905.0, 25.5),
            heading = 180.0,
            name = 'Industrial Unit'
        }
    },
    
    -- ═══════════════════════════════════════════════════════════════════════════
    -- DRONE DELIVERY LOCATIONS
    -- ═══════════════════════════════════════════════════════════════════════════
    droneDrops = {
        {
            coords = vector3(380.0, -815.0, 29.5),
            name = 'Alta Construction'
        },
        {
            coords = vector3(-725.0, -935.0, 19.5),
            name = 'Little Seoul Parking'
        },
        {
            coords = vector3(225.0, -1685.0, 29.3),
            name = 'Davis Lot'
        },
        {
            coords = vector3(-1190.0, -1510.0, 4.5),
            name = 'Vespucci Beach'
        },
        {
            coords = vector3(750.0, -970.0, 24.5),
            name = 'La Mesa Drop'
        }
    },
    
    -- ═══════════════════════════════════════════════════════════════════════════
    -- BLACKLIST ZONES (No vehicle spawning)
    -- ═══════════════════════════════════════════════════════════════════════════
    blacklistZones = {
        -- Police Stations
        {
            coords = vector3(425.0, -980.0, 30.0),
            radius = 150,
            name = 'Mission Row PD'
        },
        {
            coords = vector3(-1095.0, -845.0, 14.0),
            radius = 100,
            name = 'Vespucci PD'
        },
        {
            coords = vector3(620.0, 0.0, 80.0),
            radius = 100,
            name = 'Vinewood PD'
        },
        {
            coords = vector3(1850.0, 3690.0, 34.0),
            radius = 100,
            name = 'Sandy Shores PD'
        },
        {
            coords = vector3(-450.0, 6015.0, 32.0),
            radius = 100,
            name = 'Paleto Bay PD'
        },
        
        -- Hospitals
        {
            coords = vector3(300.0, -1440.0, 30.0),
            radius = 100,
            name = 'Pillbox Hospital'
        },
        {
            coords = vector3(-450.0, -340.0, 35.0),
            radius = 100,
            name = 'Mount Zonah'
        },
        
        -- Safe Zones
        {
            coords = vector3(-270.0, -960.0, 31.0),
            radius = 100,
            name = 'Legion Square'
        },
        {
            coords = vector3(-1040.0, -2740.0, 20.0),
            radius = 200,
            name = 'Airport'
        },
        
        -- Military
        {
            coords = vector3(-2250.0, 3120.0, 32.0),
            radius = 500,
            name = 'Fort Zancudo'
        },
        
        -- Prison
        {
            coords = vector3(1850.0, 2600.0, 46.0),
            radius = 300,
            name = 'Bolingbroke'
        }
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOCATION UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function GetSpawnZonesForClass(class)
    local zones = {}
    for _, zone in ipairs(Config.Locations.spawnZones) do
        for _, zoneClass in ipairs(zone.classes) do
            if zoneClass == class then
                table.insert(zones, zone)
                break
            end
        end
    end
    return zones
end

function GetWeightedRandomZone(zones)
    local totalWeight = 0
    for _, zone in ipairs(zones) do
        totalWeight = totalWeight + (zone.weight or 1)
    end
    
    local random = math.random() * totalWeight
    local current = 0
    
    for _, zone in ipairs(zones) do
        current = current + (zone.weight or 1)
        if random <= current then
            return zone
        end
    end
    
    return zones[1]
end

function IsInBlacklistZone(coords)
    for _, zone in ipairs(Config.Locations.blacklistZones) do
        local dist = #(coords - zone.coords)
        if dist < zone.radius then
            return true, zone.name
        end
    end
    return false, nil
end

function GetRandomDropOff()
    return Config.Locations.dropOff[math.random(#Config.Locations.dropOff)]
end

function GetRandomChopShop()
    return Config.Locations.chopShops[math.random(#Config.Locations.chopShops)]
end

function GetRandomVinLocation()
    return Config.Locations.vinLocations[math.random(#Config.Locations.vinLocations)]
end

function GetRandomDroneLocation()
    return Config.Locations.droneDrops[math.random(#Config.Locations.droneDrops)]
end

function GetRandomPointInRadius(center, radius, minRadius)
    minRadius = minRadius or 0
    local angle = math.random() * 2 * math.pi
    local distance = minRadius + math.random() * (radius - minRadius)
    
    local x = center.x + math.cos(angle) * distance
    local y = center.y + math.sin(angle) * distance
    local z = center.z
    
    return vector3(x, y, z)
end

function FindSafeSpawnPoint(center, radius)
    local maxAttempts = 50
    
    for i = 1, maxAttempts do
        local point = GetRandomPointInRadius(center, radius, radius * 0.3)
        
        -- Check if in blacklist
        if not IsInBlacklistZone(point) then
            -- Get ground Z
            local found, groundZ = GetGroundZFor_3dCoord(point.x, point.y, point.z + 100.0, false)
            if found then
                point = vector3(point.x, point.y, groundZ + 0.5)
                return point
            end
        end
    end
    
    return nil
end
