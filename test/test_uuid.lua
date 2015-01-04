local uuid = require 'uuid'

local uuid1 = uuid.create()
assert(#uuid1 == 16)
print(uuid1)
local s1 = tostring(uuid1)
local s2 = tostring(uuid1)
assert(s1 == s2)

local uuid2 = uuid.create()
assert(uuid1 ~= uuid2)
assert(uuid1 == uuid1)

local cmp = uuid1 < uuid2
cmp = uuid1 <= uuid2
cmp = uuid1 > uuid2

print('uuid OK')