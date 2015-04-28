local mp = require 'msgpack'
local json = require 'cjson'
local dumpstring = require 'dump'.dumpstring


local function test_numbers()
    assert(mp.unpack(mp.pack(1/0)) == 1/0)
    assert(mp.unpack(mp.pack(-1/0)) == -1/0)
    local nan = mp.unpack(mp.pack(0/0))
    assert(nan ~= nan)
end

local function test_array()
    local a = {'a', 'b', 'c'}
    local data = mp.pack(a)
    local r = mp.unpack(data)
    print(dumpstring(r))
    
    -- nil hole
    a = {10, 20, nil, 40, 30}
    data = mp.pack(a)
    r = mp.unpack(data)
    print(dumpstring(r))
    
    -- integer key
    a = {}
    a[100] = 1
    a[111] = 12
    a[122] = 123
    data = mp.pack(a)
    r = mp.unpack(data)
    print(dumpstring(r))
end

local function test_table()
    local s = mp.pack{}     -- empty table as array
    assert(s:byte() == 0x90)

    local t = {a=10, b=-20, nil, d=40}
    s = mp.pack(t)
    mp.unpack(s)
    
    local nested = {
        a = 0,
        b = 0xffffffff,
        c = nil,
        --e = 1/0,
        this_is_a_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_long_key = -2147483647,
        nation = {
            china = {
                provice = {
                    name = 'cd',
                    gdp = 3.14159,
                    gov = {
                        road = 'Hi Tech 5th',
                    }
                }
            }
        }
    }
    s = mp.pack(nested)
    mp.unpack(s)
end

test_array()
