local utils = require 'qsf.utils'

local function test_random()
    for n=1, 100 do
        local r2 = utils.random(100)
        assert(r2 >= 0 and r2 <= 100)

        local r3 = utils.random(1000, 10000)
        assert(r3 >= 1000 and r3 <= 10000)
    end
end

test_random()
print('random OK')