-- Extensions to the math module

local math = require 'math'

local assert = assert

--四舍五入
function math.round(v) 
    return math.floor(v + 0.5)
end

--浮点数随机
function math.randf(low, up)
    local r = math.random()
    return r * (up - low) + low
end

-- 最大公倍数
function math.gcd(a, b)
    if b ~= 0 then
        return math.gcd(b, a % b)
    else
        return math.abs(a)
    end
end

--打乱数组
function math.shuffle_array(array)
    for n = 2, #array do
        local off = math.random(n)
        array[off], array[n] = array[n], array[off]
    end
    return array
end

--生成一个随机数组
function math.random_array(min, max)
    assert(min <= max)
    local res = {}
    for n = min, max do
        res[#res + 1] = n
    end
    return math.shuffle_array(res)
end