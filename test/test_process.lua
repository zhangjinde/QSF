local process = require 'process'

local function process_rand()
    local set = {}
    for n=1, 100 do 
        local num = process.rand32()
        assert(set[num] == nil)
        set[num] = 1
    end
end

local function process_uuid()
    local uuid1 = process.createUUID()
    local uuid2 = process.createUUID()
    assert(uuid1 ~= uuid2)
    print('uuid len', #uuid1)
    local uuid3 = process.createUUID('hex')
    assert(uuid3 ~= uuid2)
    assert(#uuid3 > #uuid2)
end

process_rand()
process_uuid()

print('process passed')