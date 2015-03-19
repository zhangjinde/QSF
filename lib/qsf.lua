--
-- Dispatch service message
--
local mq = require 'mq'
local process = require 'process'
local mp = require 'MessagePack'
local trace = require 'trace'
local dumpstring = require 'dump'.dumpstring -- debugging only

local xpcall, tunpack, tpack = xpcall
local mq_send, mq_recv, mp_pack = mq.send, mq.recv, mp.pack
local process_gettick = process.gettick
local process_sleep = process.sleep
local trace_stack = trace.dump_stack

local qsf = {}

-- send notify message
function qsf.notify(node, method, ...)
    mq_send(node, mp_pack{method=method, params={...}})
end

-- launch new service
function qsf.launch(name, path, ...)
    mq.launch(name, path, ...)
end

-- send log text to logger service
function qsf.log(...)
    qsf.notify('logger', 'print', ...)
end

local function dispatch_message(service, idle)
    local opt = (func ~= nil and 'nowait' or nil)
    local from, data = mq_recv(opt)
    if from and data then
        --print(from .. ' ==> ' .. mq.name(), data)
        local request = mp_unpack(data) -- JSON RPC variant
        local func = service[msg.method]
        if func then 
            local result = {func(tunpack(msg.params))}
            if #result > 0 then 
                local response = mp_pack{method=msg.method, params=result}
                mq_send(from, response)
            end
        end
    else
        if idle then 
            idle() -- dispatch user-defined function
        end
    end
end

function qsf.run(service, func)
    assert(func == nil or type(func) == 'function')
    trace.dumper = qsf.log
    while true do
        local ticks = process_gettick()
        xpcall(dispatch_message, trace_stack, service, func)
        ticks = process_gettick() - ticks
        if ticks < 10 then
            process_sleep(5) -- yield OS thread context
        end
    end
end

return qsf