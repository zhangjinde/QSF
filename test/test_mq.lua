local mq = require 'mq'
local process = require 'process'

local node_name = 'child_node'

local function mq_launch()
    local name = mq.name()
    assert(name == 'test')
    assert(mq.launch(node_name, '../test/spawn_child.lua') == true)
    print('spawn child')
    process.sleep(1000) -- wait child thread
end

local function mq_recv()
    mq.send(node_name, 'hello')
    local name, s = mq.recv()
    print(name, s)
    assert(name == node_name)
    assert(s == 'world')
end

local function mq_recv_nowait()
    mq.send(node_name, 'hello')
    while true do 
        local name, s = mq.recv('nowait')
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

print('mq passed')