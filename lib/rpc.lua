local qsf = require 'qsf'
local cmsgpack = require 'cmsgpack'
local dump = require 'dump'

local type = type
local table = table
local pcall = pcall
local print = print
local assert = assert
local coroutine = coroutine

local rpc = {}

local MIN_FRAME_TIME = 10 --ms

local session_id = 0
local session_id_coroutine = {}
local coroutine_session_id = {}

-- remote procedure call, current coroutine will be suspend
function rpc.call(node, method, ...)
    local session_id = coroutine_session_id[coroutine.running()]
    local request = {method=method, id=session_id, params={...}}
    qsf.send(node, cmsgpack.pack(request))
    return coroutine.yield()
end

-- send notify message
function rpc.notify(node, method, ...)
    local request = {method=method, params={...}}
    qsf.send(node, cmsgpack.pack(request))
end

-- create a coroutine
local function co_create(session_id)
    local co = coroutine.create(function (service, method, params)
        local func = service[method]
        if func then
            local result = {func(table.unpack(params))}
            if #result > 0 then
                return {id=session_id, result=result}
            end
        else
            error('rpc method not found: ', method)
        end
    end)
    return co
end

local function unknown_response(response)
    print('unknown response', response.id)
end

-- dispatch inproc message
local function dispatch_rpc_message(from, data, service)
    local msg = cmsgpack.unpack(data)
    if msg.method == nil then -- response
        local co = session_id_coroutine[msg.id]
        if co then
            coroutine.resume(co, table.unpack(msg.params))
        else
            unknown_response(msg)
        end
    else -- request
        session_id = session_id + 1
        local co = co_create(session_id)
        coroutine_session_id[co] = session_id
        session_id_coroutine[session_id] = co
        local ok, response = coroutine.resume(co, service, msg.method, msg.params))
        if ok then
            if response and msg.id then
                qsf.send(from, cmsgpack.pack(response))
            end
        end
        session_id_coroutine[session_id] = nil
        coroutine_session_id[co] = nil        
    end
end

local function dispatch_message(service, func)
    local opt = (func ~= nil and 'nowait' or nil)
    local from, data = qsf.recv(opt)
    if from and data then
        -- exit signal
        if from == 'sys' and data == 'exit' then
            qsf.send(from, 'roger')
            return false
        else
            dispatch_rpc_message(from, data, service)
        end
        return true
    end
    -- dispatch user-defined function
    if func then
        func()
    end
    return true
end

function rpc.run(service, func)
    assert(func == nil or type(func) == 'function')
    local elapsed_tick = 0
    while true do
        local start_tick = qsf.gettick()
        local ok, result = xpcall(dispatch_message, dump.dump_stack, service, func)
        if ok then 
            if not result then break end
        else
            print(result) -- error message
        end
        local over_tick = qsf.gettick()
        local frame_tick = over_tick - start_tick
        if frame_tick < MIN_FRAME_TIME then
            qsf.sleep(1) -- yield OS thread context
        end
        elapsed_tick = elapsed_tick + frame_tick
    end
end

return rpc