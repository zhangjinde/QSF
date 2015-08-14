--
-- Service的消息分发
--
local uv = require 'luv'
local node = require 'node'
local mp = require 'MessagePack'
local trace = require 'trace'
local proto = require 'proto'


local xpcall = xpcall
local node_send, node_recv, mp_pack = node.send, node.recv, mp.pack

local qsf = {}
local router

-- send notify message
function qsf.notify(node, method, ...)
    node_send(node, mp_pack{method=method, params={...}})
end

function qsf.launch(name, path, ...)
    node.launch(name, path, ...)
end

function qsf.log(...)
    qsf.notify('logger', 'print', ...)
end

local function dispatch_message()
    while true do
        local from, data = node_recv('nowait')
        if from and data then
            --print(from .. ' ==> ' .. mq.name(), data)
            local response = proto.dispatch_ipc_message(router, data)
            if response then
                node_send(from, response)
            end
        else
            return
        end
    end
end

function qsf.timeout(func, msec, rp)
    assert(type(func) == 'function')
    local timer = uv.createTimer()
    timer:start(msec, rp or 0, func)
    return timer
end

function qsf.start(service)
    trace.dumper = qsf.log
    router = service
    local timer = qsf.timeout(dispatch_message, 50, 50)
    while true do
        local ok, result = xpcall(node.run, trace.dump_stack)
        if not ok then
            qsf.log(result)
            break
        end
    end
end

return qsf