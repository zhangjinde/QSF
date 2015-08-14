local node = require 'node'

local name = node.name()
assert(name == 'child_node')

local count = 0
while true do
    local name, s = node.recv()
    print(name, s)
    assert(name == 'test')
    count = count + 1
    if s == 'hello' then 
        node.send(name, 'world')
        if count == 2 then 
            break
        end
    end
end