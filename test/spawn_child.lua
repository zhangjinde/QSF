local mq = require 'mq'

local name = mq.name()
assert(name == 'child_node')

while true do
    local name, s = mq.recv()
    print(name, s)
    assert(name == 'test')
    if s == 'hello' then 
        mq.send(name, 'world')
    elseif s == 'exit' then
        mq.send(name, 'roger')
        break
    end
end