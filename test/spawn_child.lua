local mq = require 'mq'

local name = mq.name()
assert(name == 'child_node')

local count = 0
while true do
    local name, s = mq.recv()
    print(name, s)
    assert(name == 'test')
    count = count + 1
    if s == 'hello' then 
        mq.send(name, 'world')
        if count == 2 then 
            break
        end
    end
end