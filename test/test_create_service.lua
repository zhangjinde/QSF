local mq = require 'mq'
local process = require 'process'

local all_nodes = {}

local function create_service(count)
    for n=1, count do
        local name = 'node' .. n
        assert(mq.launch(name, '../test/spawn_dummy.lua', mq.name()))
        all_nodes[name] = true
    end
    process.sleep(1000)
end

create_service(100)

for name, _ in pairs(all_nodes) do 
    mq.send(name, 'exit')
    mq.recv()  
end