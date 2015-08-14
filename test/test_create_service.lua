local uv = require 'luv'
local node = require 'node'


local all_nodes = {}

local function create_service(count)
    for n=1, count do
        local name = 'node' .. n
        assert(node.launch(name, '../test/spawn_dummy.lua', node.name()))
        all_nodes[name] = true
    end
    uv.sleep(1000)
end

create_service(100)

for name, _ in pairs(all_nodes) do 
    node.send(name, 'exit')
    node.recv()  
end