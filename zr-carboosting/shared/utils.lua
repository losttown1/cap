-- ═══════════════════════════════════════════════════════════════════════════════
-- SHARED UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

Utils = {}

-- ═══════════════════════════════════════════════════════════════════════════════
-- STRING UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function Utils.GeneratePlate()
    local chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    local nums = '0123456789'
    local plate = ''
    
    for i = 1, 3 do
        plate = plate .. chars:sub(math.random(#chars), math.random(#chars))
    end
    
    plate = plate .. ' '
    
    for i = 1, 4 do
        plate = plate .. nums:sub(math.random(#nums), math.random(#nums))
    end
    
    return plate
end

function Utils.FormatTime(seconds)
    local mins = math.floor(seconds / 60)
    local secs = seconds % 60
    return string.format('%02d:%02d', mins, secs)
end

function Utils.FormatMoney(amount)
    local formatted = tostring(amount)
    local k
    while true do
        formatted, k = string.gsub(formatted, '^(-?%d+)(%d%d%d)', '%1,%2')
        if k == 0 then break end
    end
    return '$' .. formatted
end

function Utils.FormatNumber(num)
    local formatted = tostring(num)
    local k
    while true do
        formatted, k = string.gsub(formatted, '^(-?%d+)(%d%d%d)', '%1,%2')
        if k == 0 then break end
    end
    return formatted
end

function Utils.RandomString(length)
    local chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
    local str = ''
    for i = 1, length do
        local rand = math.random(#chars)
        str = str .. chars:sub(rand, rand)
    end
    return str
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- MATH UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function Utils.RandomInRange(min, max)
    return math.random() * (max - min) + min
end

function Utils.RandomIntInRange(min, max)
    return math.random(min, max)
end

function Utils.Clamp(value, min, max)
    return math.max(min, math.min(max, value))
end

function Utils.Lerp(a, b, t)
    return a + (b - a) * t
end

function Utils.Round(num, decimals)
    local mult = 10 ^ (decimals or 0)
    return math.floor(num * mult + 0.5) / mult
end

function Utils.CalculateDistance(pos1, pos2)
    return #(pos1 - pos2)
end

function Utils.IsInRange(pos1, pos2, range)
    return Utils.CalculateDistance(pos1, pos2) <= range
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- TABLE UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function Utils.DeepCopy(original)
    local copy
    if type(original) == 'table' then
        copy = {}
        for key, value in next, original, nil do
            copy[Utils.DeepCopy(key)] = Utils.DeepCopy(value)
        end
        setmetatable(copy, Utils.DeepCopy(getmetatable(original)))
    else
        copy = original
    end
    return copy
end

function Utils.TableContains(tbl, value)
    for _, v in pairs(tbl) do
        if v == value then
            return true
        end
    end
    return false
end

function Utils.TableFind(tbl, predicate)
    for k, v in pairs(tbl) do
        if predicate(v, k) then
            return v, k
        end
    end
    return nil
end

function Utils.TableFilter(tbl, predicate)
    local result = {}
    for k, v in pairs(tbl) do
        if predicate(v, k) then
            table.insert(result, v)
        end
    end
    return result
end

function Utils.TableMap(tbl, transform)
    local result = {}
    for k, v in pairs(tbl) do
        result[k] = transform(v, k)
    end
    return result
end

function Utils.TableMerge(t1, t2)
    local result = Utils.DeepCopy(t1)
    for k, v in pairs(t2) do
        if type(v) == 'table' and type(result[k]) == 'table' then
            result[k] = Utils.TableMerge(result[k], v)
        else
            result[k] = v
        end
    end
    return result
end

function Utils.TableLength(tbl)
    local count = 0
    for _ in pairs(tbl) do
        count = count + 1
    end
    return count
end

function Utils.TableKeys(tbl)
    local keys = {}
    for k in pairs(tbl) do
        table.insert(keys, k)
    end
    return keys
end

function Utils.TableValues(tbl)
    local values = {}
    for _, v in pairs(tbl) do
        table.insert(values, v)
    end
    return values
end

function Utils.ShuffleTable(tbl)
    local shuffled = Utils.DeepCopy(tbl)
    for i = #shuffled, 2, -1 do
        local j = math.random(i)
        shuffled[i], shuffled[j] = shuffled[j], shuffled[i]
    end
    return shuffled
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- CONTRACT UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function Utils.GenerateContractId()
    return string.format('BOOST-%s-%d', Utils.RandomString(6), os.time())
end

function Utils.CalculateReward(rewardConfig)
    local reward = {}
    
    if rewardConfig.points then
        reward.points = Utils.RandomIntInRange(rewardConfig.points.min, rewardConfig.points.max)
    end
    
    if rewardConfig.currency then
        reward.currency = Utils.RandomIntInRange(rewardConfig.currency.min, rewardConfig.currency.max)
    end
    
    if rewardConfig.money then
        reward.money = Utils.RandomIntInRange(rewardConfig.money.min, rewardConfig.money.max)
    end
    
    if rewardConfig.items and #rewardConfig.items > 0 then
        reward.items = {}
        for _, itemConfig in ipairs(rewardConfig.items) do
            if math.random(100) <= itemConfig.chance then
                local amount = 1
                if itemConfig.amount then
                    amount = Utils.RandomIntInRange(itemConfig.amount[1], itemConfig.amount[2])
                end
                table.insert(reward.items, {
                    item = itemConfig.item,
                    amount = amount
                })
            end
        end
    end
    
    return reward
end

function Utils.CalculateLevelFromPoints(points)
    local level = 1
    local totalRequired = 0
    
    while true do
        local required = Config.Progression.pointsPerLevel(level)
        if totalRequired + required > points then
            break
        end
        totalRequired = totalRequired + required
        level = level + 1
        
        if level >= Config.Progression.maxLevel then
            break
        end
    end
    
    return level
end

function Utils.GetPointsForNextLevel(currentLevel)
    return Config.Progression.pointsPerLevel(currentLevel)
end

function Utils.CanAccessClass(playerData, class)
    local classConfig = Config.Classes[class]
    if not classConfig then return false end
    
    if playerData.level < classConfig.requiredLevel then
        return false, 'level', classConfig.requiredLevel
    end
    
    if playerData.currency < classConfig.requiredCurrency then
        return false, 'currency', classConfig.requiredCurrency
    end
    
    return true
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- VALIDATION UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function Utils.IsValidPlate(plate)
    if not plate or type(plate) ~= 'string' then return false end
    return #plate >= 1 and #plate <= 8
end

function Utils.IsValidClass(class)
    return Config.Classes[class] ~= nil
end

function Utils.IsValidContractType(contractType)
    return Config.ContractTypes[contractType] ~= nil
end

function Utils.SanitizeString(str)
    if not str then return '' end
    return str:gsub('[<>]', ''):sub(1, 100)
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- DEBUG UTILITIES
-- ═══════════════════════════════════════════════════════════════════════════════

function Utils.Debug(...)
    if Config.Debug then
        local args = {...}
        local str = '[ZR-CarBoosting]'
        for _, v in ipairs(args) do
            str = str .. ' ' .. tostring(v)
        end
        print(str)
    end
end

function Utils.DebugTable(tbl, indent)
    if not Config.Debug then return end
    indent = indent or 0
    local prefix = string.rep('  ', indent)
    
    for k, v in pairs(tbl) do
        if type(v) == 'table' then
            print(prefix .. tostring(k) .. ':')
            Utils.DebugTable(v, indent + 1)
        else
            print(prefix .. tostring(k) .. ': ' .. tostring(v))
        end
    end
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- EXPORTS
-- ═══════════════════════════════════════════════════════════════════════════════

return Utils
