local math = require 'math'
local string = require 'string'

local function math_round()
    assert(type(math.round) == 'function')
    assert(math.round(1.01) == 1)
    assert(math.round(1.21) == 1)
    assert(math.round(1.34) == 1)
    assert(math.round(1.49999) == 1)
    assert(math.round(1.5001) == 2)
    assert(math.round(1.678) == 2)
    assert(math.round(1.9999) == 2)
end

local function string_endswith()
    local s = '/usr/local/include'
    assert(type(string.ends_with) == 'function')
    assert(s:ends_with('include'))
    assert(s:ends_with(''))
    assert(not s:ends_with('/'))
    assert(not s:ends_with('/usr'))
    assert(not string.ends_with('include', s))
end

local function string_startwith()
    local s = '/usr/local/include'
    assert(type(string.start_with) == 'function')
    assert(s:start_with('/usr'))
    assert(s:start_with('/'))
    assert(s:start_with(''))
    assert(not s:start_with('include'))
    assert(not string.start_with('/usr', s))
end

math_round()
string_endswith()
string_startwith()
print('stdlib passed')