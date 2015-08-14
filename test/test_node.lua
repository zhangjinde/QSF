local uv = require 'luv'
local node = require 'node'


local node_name = 'child_node'

local function mq_launch()
    local name = node.name()
    assert(name == 'test')
    assert(node.launch(node_name, '../test/spawn_child.lua') == true)
    print('spawn child')
    uv.sleep(1000) -- wait child thread
end

local function mq_recv()
    node.send(node_name, 'hello')
    local name, s = node.recv()
    print(name, s)
    assert(name == node_name)
    assert(s == 'world')
end

local function mq_recv_nowait()
    node.send(node_name, 'hello')
    while true do 
        local name, s = node.recv('nowait')
        if name and s then 
            assert(name == node_name)
            assert(s == 'world')
            break
        end
    end
end

mq_launch()
mq_recv()
mq_recv_nowait()

print('node passed')