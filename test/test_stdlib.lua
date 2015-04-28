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
end

math_round()
string_endswith()
print('stdlib passed')