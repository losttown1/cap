Config = {}

-- ═══════════════════════════════════════════════════════════════════════════════
-- GENERAL SETTINGS
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Debug = false
Config.DevMode = false

Config.Framework = 'qb-core' -- 'qb-core' or 'esx'
Config.Inventory = 'ox_inventory' -- 'ox_inventory', 'qb-inventory', 'qs-inventory'
Config.Target = 'ox_target' -- 'ox_target', 'qb-target'
Config.Dispatch = 'ps-dispatch' -- 'ps-dispatch', 'cd_dispatch', 'custom'

Config.TabletItem = 'boosting_laptop' -- Item required to open tablet
Config.TabletCommand = 'boosting' -- Command to open tablet (for testing/admin)

-- ═══════════════════════════════════════════════════════════════════════════════
-- PROGRESSION SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Progression = {
    startingLevel = 1,
    maxLevel = 100,
    startingPoints = 0,
    startingCurrency = 0,
    
    -- Points needed per level (can be formula or table)
    pointsPerLevel = function(level)
        return math.floor(100 * (level ^ 1.5))
    end,
    
    -- Rewards per level up
    levelUpRewards = {
        currency = 500,
        notification = true,
        sound = true
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT CLASSES
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Classes = {
    ['D'] = {
        label = 'Class D',
        color = '#808080',
        icon = 'fa-car',
        requiredLevel = 1,
        requiredCurrency = 0,
        minPolice = 0,
        
        -- Rewards
        rewards = {
            points = { min = 10, max = 20 },
            currency = { min = 500, max = 1000 },
            money = { min = 1000, max = 2500 },
            items = {} -- { item = 'lockpick', chance = 50, amount = {1, 2} }
        },
        
        -- Penalties
        penalties = {
            points = 5,
            currency = 0,
            heat = 10
        },
        
        -- Mission settings
        searchRadius = 150,
        timeLimit = 900, -- 15 minutes
        gpsAlertInterval = 20,
        guardCount = 0,
        
        -- Cooldown
        cooldown = 300 -- 5 minutes
    },
    
    ['C'] = {
        label = 'Class C',
        color = '#00FF00',
        icon = 'fa-car-side',
        requiredLevel = 5,
        requiredCurrency = 1000,
        minPolice = 1,
        
        rewards = {
            points = { min = 25, max = 40 },
            currency = { min = 1000, max = 2000 },
            money = { min = 3000, max = 5000 },
            items = {
                { item = 'electronickit', chance = 30, amount = {1, 1} }
            }
        },
        
        penalties = {
            points = 10,
            currency = 500,
            heat = 15
        },
        
        searchRadius = 200,
        timeLimit = 720, -- 12 minutes
        gpsAlertInterval = 18,
        guardCount = 0,
        cooldown = 600
    },
    
    ['B'] = {
        label = 'Class B',
        color = '#0080FF',
        icon = 'fa-car-alt',
        requiredLevel = 15,
        requiredCurrency = 5000,
        minPolice = 2,
        
        rewards = {
            points = { min = 50, max = 75 },
            currency = { min = 2500, max = 4000 },
            money = { min = 6000, max = 10000 },
            items = {
                { item = 'electronickit', chance = 50, amount = {1, 2} },
                { item = 'advancedlockpick', chance = 25, amount = {1, 1} }
            }
        },
        
        penalties = {
            points = 20,
            currency = 1000,
            heat = 25
        },
        
        searchRadius = 250,
        timeLimit = 600, -- 10 minutes
        gpsAlertInterval = 15,
        guardCount = 1,
        cooldown = 900
    },
    
    ['A'] = {
        label = 'Class A',
        color = '#FF8000',
        icon = 'fa-racing-flag',
        requiredLevel = 30,
        requiredCurrency = 15000,
        minPolice = 3,
        
        rewards = {
            points = { min = 100, max = 150 },
            currency = { min = 5000, max = 8000 },
            money = { min = 15000, max = 25000 },
            items = {
                { item = 'electronickit', chance = 75, amount = {2, 3} },
                { item = 'thermite', chance = 30, amount = {1, 1} }
            }
        },
        
        penalties = {
            points = 40,
            currency = 3000,
            heat = 40
        },
        
        searchRadius = 300,
        timeLimit = 480, -- 8 minutes
        gpsAlertInterval = 12,
        guardCount = 2,
        cooldown = 1200
    },
    
    ['S'] = {
        label = 'Class S',
        color = '#FF0000',
        icon = 'fa-flag-checkered',
        requiredLevel = 50,
        requiredCurrency = 40000,
        minPolice = 4,
        
        rewards = {
            points = { min = 200, max = 300 },
            currency = { min = 10000, max = 15000 },
            money = { min = 30000, max = 50000 },
            items = {
                { item = 'electronickit', chance = 100, amount = {3, 5} },
                { item = 'thermite', chance = 50, amount = {1, 2} },
                { item = 'vin_scratch_tool', chance = 20, amount = {1, 1} }
            }
        },
        
        penalties = {
            points = 75,
            currency = 7500,
            heat = 60
        },
        
        searchRadius = 350,
        timeLimit = 420, -- 7 minutes
        gpsAlertInterval = 10,
        guardCount = 3,
        cooldown = 1800
    },
    
    ['S+'] = {
        label = 'Class S+',
        color = '#FF00FF',
        icon = 'fa-crown',
        requiredLevel = 75,
        requiredCurrency = 100000,
        minPolice = 5,
        
        rewards = {
            points = { min = 400, max = 600 },
            currency = { min = 20000, max = 30000 },
            money = { min = 75000, max = 125000 },
            items = {
                { item = 'electronickit', chance = 100, amount = {5, 8} },
                { item = 'thermite', chance = 75, amount = {2, 3} },
                { item = 'vin_scratch_tool', chance = 40, amount = {1, 2} }
            }
        },
        
        penalties = {
            points = 150,
            currency = 20000,
            heat = 100
        },
        
        searchRadius = 400,
        timeLimit = 360, -- 6 minutes
        gpsAlertInterval = 8,
        guardCount = 5,
        cooldown = 2400
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT TYPES
-- ═══════════════════════════════════════════════════════════════════════════════
Config.ContractTypes = {
    ['dropoff'] = {
        label = 'Drop-Off',
        description = 'Steal the vehicle and deliver it to the drop-off location',
        icon = 'fa-truck-loading',
        available = true,
        
        -- Required items to steal vehicle
        requiredItems = {
            { item = 'lockpick', amount = 1, remove = true },
        },
        
        -- GPS disable requirements
        gpsDisable = {
            requiredItem = 'electronickit',
            removeItem = true,
            minigameStages = 3
        }
    },
    
    ['chopchop'] = {
        label = 'Chop Chop',
        description = 'Steal the vehicle and chop it for parts',
        icon = 'fa-tools',
        available = true,
        
        requiredItems = {
            { item = 'lockpick', amount = 1, remove = true },
        },
        
        gpsDisable = {
            requiredItem = 'electronickit',
            removeItem = true,
            minigameStages = 3
        },
        
        -- Chop parts configuration
        chopParts = {
            { id = 'wheel_lf', label = 'Front Left Wheel', time = 5000, reward = { item = 'tire', amount = {1, 1} } },
            { id = 'wheel_rf', label = 'Front Right Wheel', time = 5000, reward = { item = 'tire', amount = {1, 1} } },
            { id = 'wheel_lr', label = 'Rear Left Wheel', time = 5000, reward = { item = 'tire', amount = {1, 1} } },
            { id = 'wheel_rr', label = 'Rear Right Wheel', time = 5000, reward = { item = 'tire', amount = {1, 1} } },
            { id = 'door_lf', label = 'Front Left Door', time = 8000, reward = { item = 'cardoor', amount = {1, 1} } },
            { id = 'door_rf', label = 'Front Right Door', time = 8000, reward = { item = 'cardoor', amount = {1, 1} } },
            { id = 'door_lr', label = 'Rear Left Door', time = 8000, reward = { item = 'cardoor', amount = {1, 1} } },
            { id = 'door_rr', label = 'Rear Right Door', time = 8000, reward = { item = 'cardoor', amount = {1, 1} } },
            { id = 'hood', label = 'Hood', time = 10000, reward = { item = 'carhood', amount = {1, 1} } },
            { id = 'trunk', label = 'Trunk', time = 10000, reward = { item = 'cartrunk', amount = {1, 1} } },
        }
    },
    
    ['vinscratch'] = {
        label = 'VIN Scratch',
        description = 'Steal the vehicle and scratch the VIN to keep it',
        icon = 'fa-id-card',
        available = true,
        requiredLevel = 25, -- Extra level requirement
        
        requiredItems = {
            { item = 'advancedlockpick', amount = 1, remove = true },
        },
        
        gpsDisable = {
            requiredItem = 'electronickit',
            removeItem = true,
            minigameStages = 4
        },
        
        -- VIN scratch requirements
        vinScratch = {
            requiredItem = 'vin_scratch_tool',
            removeItem = true,
            minigameStages = 5,
            successChance = 80 -- Base success chance
        }
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- MINIGAME CONFIGURATION (Plug & Play)
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Minigames = {
    -- Vehicle unlock minigame
    unlock = {
        export = 'ox_lib', -- Resource name
        name = 'skillCheck', -- Export function name
        args = function(class)
            local difficulty = {
                ['D'] = { 'easy', 'easy' },
                ['C'] = { 'easy', 'easy', 'medium' },
                ['B'] = { 'easy', 'medium', 'medium' },
                ['A'] = { 'medium', 'medium', 'hard' },
                ['S'] = { 'medium', 'hard', 'hard' },
                ['S+'] = { 'hard', 'hard', 'hard', 'hard' }
            }
            return { difficulty[class] or {'easy'}, {'w', 'a', 's', 'd'} }
        end
    },
    
    -- GPS disable minigame
    gpsDisable = {
        export = 'ox_lib',
        name = 'skillCheck',
        args = function(class, stage)
            local difficulty = {
                ['D'] = 'easy',
                ['C'] = 'easy',
                ['B'] = 'medium',
                ['A'] = 'medium',
                ['S'] = 'hard',
                ['S+'] = 'hard'
            }
            return { {difficulty[class] or 'easy'}, {'w', 'a', 's', 'd'} }
        end
    },
    
    -- Chop part minigame
    chop = {
        export = 'ox_lib',
        name = 'skillCheck',
        args = function(class, partId)
            return { {'easy', 'easy'}, {'w', 'a', 's', 'd'} }
        end
    },
    
    -- VIN scratch minigame
    vinScratch = {
        export = 'ox_lib',
        name = 'skillCheck',
        args = function(class, stage)
            return { {'hard', 'hard'}, {'w', 'a', 's', 'd'} }
        end
    },
    
    -- Hotwire minigame
    hotwire = {
        export = 'ox_lib',
        name = 'skillCheck',
        args = function(class)
            local difficulty = {
                ['D'] = { 'easy' },
                ['C'] = { 'easy', 'easy' },
                ['B'] = { 'easy', 'medium' },
                ['A'] = { 'medium', 'medium' },
                ['S'] = { 'medium', 'hard' },
                ['S+'] = { 'hard', 'hard' }
            }
            return { difficulty[class] or {'easy'}, {'w', 'a', 's', 'd'} }
        end
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- GUARDS CONFIGURATION
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Guards = {
    enabled = true,
    
    models = {
        'g_m_m_armboss_01',
        'g_m_m_armgoon_01',
        'g_m_m_armlieut_01',
        'g_m_y_armgoon_02',
        's_m_y_blackops_01'
    },
    
    weapons = {
        ['D'] = {},
        ['C'] = {},
        ['B'] = { 'WEAPON_PISTOL' },
        ['A'] = { 'WEAPON_PISTOL', 'WEAPON_SMG' },
        ['S'] = { 'WEAPON_SMG', 'WEAPON_CARBINERIFLE' },
        ['S+'] = { 'WEAPON_CARBINERIFLE', 'WEAPON_SPECIALCARBINE' }
    },
    
    spawnRadius = { min = 10, max = 25 },
    accuracy = {
        ['D'] = 20,
        ['C'] = 25,
        ['B'] = 35,
        ['A'] = 45,
        ['S'] = 55,
        ['S+'] = 70
    },
    
    health = {
        ['D'] = 100,
        ['C'] = 100,
        ['B'] = 150,
        ['A'] = 200,
        ['S'] = 250,
        ['S+'] = 300
    },
    
    alertRadius = 50,
    despawnDistance = 150
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- SCANNER CONFIGURATION
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Scanner = {
    enabled = true,
    weapon = 'WEAPON_DIGISCANNER',
    
    -- Distance thresholds for color
    distances = {
        { max = 25, color = { r = 0, g = 255, b = 0 } },    -- Green (very close)
        { max = 50, color = { r = 128, g = 255, b = 0 } },  -- Light green
        { max = 100, color = { r = 255, g = 255, b = 0 } }, -- Yellow
        { max = 200, color = { r = 255, g = 128, b = 0 } }, -- Orange
        { max = 999, color = { r = 255, g = 0, b = 0 } }    -- Red (far)
    },
    
    updateInterval = 100, -- ms
    vibrationIntensity = function(distance)
        return math.max(0, math.min(255, math.floor(255 - (distance * 1.5))))
    end
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- DRONE DELIVERY SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Drone = {
    enabled = true,
    model = 'فخمة ch_prop_casino_drone_02',
    
    spawnHeight = 150,
    flySpeed = 25.0,
    dropHeight = 3.0,
    
    -- Delivery timing
    minDeliveryTime = 60, -- seconds
    maxDeliveryTime = 180,
    
    -- Package
    packageModel = 'hei_prop_heist_box',
    packageCanBeStolen = true,
    packageStealRadius = 3.0,
    packageDespawnTime = 300, -- 5 minutes if not picked up
    
    -- Effects
    sounds = {
        flying = 'Chopper_Loop',
        drop = 'WEAPON_IMPACT'
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- TABLET STORE
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Store = {
    enabled = true,
    
    items = {
        {
            id = 'lockpick',
            label = 'Lockpick',
            description = 'Basic tool to unlock vehicles',
            price = 500,
            image = 'lockpick.png',
            maxPurchase = 10,
            requiredLevel = 1
        },
        {
            id = 'advancedlockpick',
            label = 'Advanced Lockpick',
            description = 'Professional grade lockpick',
            price = 2500,
            image = 'advancedlockpick.png',
            maxPurchase = 5,
            requiredLevel = 15
        },
        {
            id = 'electronickit',
            label = 'Electronic Kit',
            description = 'Used to disable GPS trackers',
            price = 3000,
            image = 'electronickit.png',
            maxPurchase = 5,
            requiredLevel = 5
        },
        {
            id = 'vin_scratch_tool',
            label = 'VIN Scratch Tool',
            description = 'Professional VIN removal tool',
            price = 25000,
            image = 'vinscratch.png',
            maxPurchase = 2,
            requiredLevel = 25
        },
        {
            id = 'thermite',
            label = 'Thermite',
            description = 'For those tough situations',
            price = 5000,
            image = 'thermite.png',
            maxPurchase = 3,
            requiredLevel = 30
        },
        {
            id = 'radio_jammer',
            label = 'Radio Jammer',
            description = 'Delays police dispatch alerts',
            price = 15000,
            image = 'radiojammer.png',
            maxPurchase = 1,
            requiredLevel = 40
        }
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- PARTY SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Party = {
    enabled = true,
    maxMembers = 4,
    inviteTimeout = 60, -- seconds
    
    rewardDistribution = 'equal', -- 'equal', 'leader', 'performance'
    
    -- Bonus for party completion
    partyBonus = {
        points = 1.1, -- 10% bonus
        currency = 1.1,
        money = 1.0 -- No bonus for money
    },
    
    shareBlips = true,
    shareProgress = true
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- HEAT SYSTEM
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Heat = {
    enabled = true,
    maxHeat = 100,
    
    -- Heat decay over time
    decayRate = 1, -- Per minute
    decayInterval = 60000, -- 1 minute
    
    -- Heat effects
    effects = {
        { heat = 25, effect = 'increased_police' }, -- More police during contracts
        { heat = 50, effect = 'higher_difficulty' }, -- Harder minigames
        { heat = 75, effect = 'more_guards' }, -- Extra guards
        { heat = 100, effect = 'contract_lockout' } -- Cannot accept contracts
    },
    
    -- Heat reduction methods
    reduction = {
        { item = 'burner_phone', amount = 25 },
        { item = 'fake_id', amount = 50 }
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- STATE MANAGEMENT
-- ═══════════════════════════════════════════════════════════════════════════════
Config.State = {
    saveInterval = 30000, -- Save active contracts every 30 seconds
    resumeTimeout = 300, -- 5 minutes to resume after disconnect
    restoreOnRestart = true,
    
    -- What to save
    saveData = {
        contractId = true,
        vehiclePlate = true,
        vehicleNetId = true,
        remainingTime = true,
        gpsDisabled = true,
        currentPhase = true,
        choppedParts = true
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- ANTI-EXPLOIT
-- ═══════════════════════════════════════════════════════════════════════════════
Config.AntiExploit = {
    enabled = true,
    
    -- Validation checks
    validateDistance = true,
    maxValidDistance = 50,
    
    validateEntity = true,
    validatePlate = true,
    validateOwnership = true,
    
    -- Rate limiting
    eventCooldown = 1000, -- 1 second between events
    maxEventsPerMinute = 30,
    
    -- Actions on exploit detection
    actions = {
        kick = true,
        ban = false,
        log = true,
        notify_admins = true
    },
    
    -- Admin notification
    adminWebhook = '' -- Discord webhook URL
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOGGING
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Logging = {
    enabled = true,
    webhook = '', -- Discord webhook URL
    
    logEvents = {
        contractStart = true,
        contractSuccess = true,
        contractFail = true,
        vehicleStolen = true,
        gpsDisabled = true,
        chopComplete = true,
        vinScratched = true,
        storePurchase = true,
        partyCreate = true,
        exploitDetected = true
    },
    
    embedColor = 0xFF0000, -- Red
    serverName = 'Your Server Name'
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- NOTIFICATIONS
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Notifications = {
    position = 'top-right',
    
    messages = {
        -- Contract messages
        contract_accepted = 'Contract accepted! Find the vehicle.',
        contract_completed = 'Contract completed! Rewards received.',
        contract_failed = 'Contract failed!',
        contract_expired = 'Contract expired! Time ran out.',
        
        -- Vehicle messages
        vehicle_found = 'Vehicle located!',
        vehicle_unlocked = 'Vehicle unlocked!',
        vehicle_hotwired = 'Vehicle hotwired!',
        gps_disabled = 'GPS tracker disabled!',
        
        -- Police messages
        police_alerted = 'Police have been alerted to your location!',
        not_enough_police = 'Not enough police online for this contract.',
        
        -- Requirements
        missing_item = 'You are missing required items!',
        not_in_vehicle = 'You must be in the target vehicle!',
        wrong_vehicle = 'This is not the target vehicle!',
        
        -- Progression
        level_up = 'Level Up! You are now level %s!',
        points_earned = '+%s Boosting Points',
        currency_earned = '+$%s Illegal Currency',
        
        -- Store
        purchase_success = 'Item purchased! Delivery incoming.',
        insufficient_funds = 'Not enough currency!',
        
        -- Party
        party_invite = '%s has invited you to their crew!',
        party_joined = '%s has joined the crew!',
        party_left = '%s has left the crew.',
        
        -- Errors
        already_in_contract = 'You already have an active contract!',
        class_locked = 'This class is locked! Requirements not met.',
        on_cooldown = 'This contract type is on cooldown!'
    }
}

-- ═══════════════════════════════════════════════════════════════════════════════
-- BLIP CONFIGURATION
-- ═══════════════════════════════════════════════════════════════════════════════
Config.Blips = {
    searchArea = {
        sprite = 5,
        color = 1,
        alpha = 100
    },
    
    vehicle = {
        sprite = 225,
        color = 5,
        scale = 0.8
    },
    
    dropoff = {
        sprite = 326,
        color = 2,
        scale = 1.0
    },
    
    chop = {
        sprite = 446,
        color = 1,
        scale = 1.0
    },
    
    vin = {
        sprite = 521,
        color = 5,
        scale = 1.0
    }
}
