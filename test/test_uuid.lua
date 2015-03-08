local uuid = require 'uuid'

local function uuid_create()
    local list = {}
    for n=1, 100 do
        local s = tostring(uuid.create())
        assert(not list[s])
        list[s] = true
    end
end

local function uuid_compare()
    local a = uuid.create()
    assert(#a == 16)
    local b = uuid.create()
    assert(a ~= b)
end

uuid_create()
uuid_compare()
print('uuid passed')