-- ═══════════════════════════════════════════════════════════════════════════════
-- SERVER DATABASE - DATABASE SETUP & QUERIES
-- ═══════════════════════════════════════════════════════════════════════════════

-- ═══════════════════════════════════════════════════════════════════════════════
-- DATABASE INITIALIZATION
-- ═══════════════════════════════════════════════════════════════════════════════

MySQL.ready(function()
    -- Players table
    MySQL.Async.execute([[
        CREATE TABLE IF NOT EXISTS `boosting_players` (
            `id` INT AUTO_INCREMENT PRIMARY KEY,
            `citizenid` VARCHAR(50) NOT NULL UNIQUE,
            `level` INT DEFAULT 1,
            `points` INT DEFAULT 0,
            `currency` INT DEFAULT 0,
            `heat` INT DEFAULT 0,
            `completed_contracts` INT DEFAULT 0,
            `failed_contracts` INT DEFAULT 0,
            `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            INDEX idx_citizenid (`citizenid`)
        )
    ]])
    
    -- Active contracts table (for state persistence)
    MySQL.Async.execute([[
        CREATE TABLE IF NOT EXISTS `boosting_active_contracts` (
            `id` INT AUTO_INCREMENT PRIMARY KEY,
            `contract_id` VARCHAR(100) NOT NULL UNIQUE,
            `citizenid` VARCHAR(50) NOT NULL,
            `class` VARCHAR(10) NOT NULL,
            `contract_type` VARCHAR(50) NOT NULL,
            `vehicle_model` VARCHAR(50),
            `plate` VARCHAR(10),
            `vehicle_netid` INT,
            `search_area` TEXT,
            `spawn_point` TEXT,
            `time_remaining` INT,
            `gps_disabled` TINYINT DEFAULT 0,
            `phase` VARCHAR(50) DEFAULT 'search',
            `chopped_parts` TEXT,
            `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_citizenid (`citizenid`),
            INDEX idx_contract_id (`contract_id`)
        )
    ]])
    
    -- Parties table
    MySQL.Async.execute([[
        CREATE TABLE IF NOT EXISTS `boosting_parties` (
            `id` INT AUTO_INCREMENT PRIMARY KEY,
            `party_id` VARCHAR(100) NOT NULL UNIQUE,
            `leader_citizenid` VARCHAR(50) NOT NULL,
            `members` TEXT,
            `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_party_id (`party_id`),
            INDEX idx_leader (`leader_citizenid`)
        )
    ]])
    
    -- Contract history
    MySQL.Async.execute([[
        CREATE TABLE IF NOT EXISTS `boosting_history` (
            `id` INT AUTO_INCREMENT PRIMARY KEY,
            `contract_id` VARCHAR(100) NOT NULL,
            `citizenid` VARCHAR(50) NOT NULL,
            `class` VARCHAR(10) NOT NULL,
            `contract_type` VARCHAR(50) NOT NULL,
            `vehicle` VARCHAR(50),
            `plate` VARCHAR(10),
            `status` ENUM('completed', 'failed', 'cancelled') NOT NULL,
            `rewards` TEXT,
            `penalties` TEXT,
            `duration` INT,
            `completed_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_citizenid (`citizenid`),
            INDEX idx_status (`status`)
        )
    ]])
    
    -- Store purchases
    MySQL.Async.execute([[
        CREATE TABLE IF NOT EXISTS `boosting_purchases` (
            `id` INT AUTO_INCREMENT PRIMARY KEY,
            `citizenid` VARCHAR(50) NOT NULL,
            `item_id` VARCHAR(50) NOT NULL,
            `amount` INT DEFAULT 1,
            `price` INT NOT NULL,
            `purchased_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_citizenid (`citizenid`)
        )
    ]])
    
    print('^2[ZR-CarBoosting]^0 Database tables initialized')
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT HISTORY
-- ═══════════════════════════════════════════════════════════════════════════════

function SaveContractHistory(contract, status, rewards, penalties)
    local duration = os.time() - contract.startTime
    
    MySQL.Async.execute([[
        INSERT INTO boosting_history 
        (contract_id, citizenid, class, contract_type, vehicle, plate, status, rewards, penalties, duration)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ]], {
        contract.id,
        contract.citizenid,
        contract.class,
        contract.contractType,
        contract.vehicle and contract.vehicle.model or nil,
        contract.plate,
        status,
        rewards and json.encode(rewards) or nil,
        penalties and json.encode(penalties) or nil,
        duration
    })
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- LEADERBOARD
-- ═══════════════════════════════════════════════════════════════════════════════

function GetLeaderboard(limit)
    limit = limit or 10
    
    return MySQL.Sync.fetchAll([[
        SELECT citizenid, level, points, currency, completed_contracts 
        FROM boosting_players 
        ORDER BY points DESC 
        LIMIT ?
    ]], { limit })
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- PLAYER STATS
-- ═══════════════════════════════════════════════════════════════════════════════

function GetPlayerStats(citizenid)
    return MySQL.Sync.fetchSingle([[
        SELECT 
            bp.*,
            (SELECT COUNT(*) FROM boosting_history WHERE citizenid = bp.citizenid AND status = 'completed') as total_completed,
            (SELECT COUNT(*) FROM boosting_history WHERE citizenid = bp.citizenid AND status = 'failed') as total_failed,
            (SELECT SUM(JSON_EXTRACT(rewards, '$.money')) FROM boosting_history WHERE citizenid = bp.citizenid AND status = 'completed') as total_earnings
        FROM boosting_players bp
        WHERE bp.citizenid = ?
    ]], { citizenid })
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

exports('SaveContractHistory', SaveContractHistory)
exports('GetLeaderboard', GetLeaderboard)
exports('GetPlayerStats', GetPlayerStats)
