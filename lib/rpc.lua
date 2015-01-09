local qsf = require 'qsf'
local cmsgpack = require 'cmsgpack'
local trace = require 'trace'

local type = type
local pcall = pcall
local print = print
local error = error
local assert = assert
local table = table
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

-- reuse coroutine
local coroutine_pool = {}
local coroutine_count = 0

local function co_create(f)
    local co = table.remove(coroutine_pool)
    if co == nil then
        co = coroutine.create(function(...)
            f(...)
            while true do
                f = nil
                coroutine_pool[#coroutine_pool+1] = co
                f = coroutine.yield('EXIT')
                f(coroutine.yield())
            end
        end)
        coroutine_count = coroutine_count + 1
        if coroutine_count > 1024 then
            error("coroutine more than 1024 may overload")
            coroutine_count = 0
        end
    else
        coroutine.resume(co, f)
    end
    return co
end

local function suspend(co, ok, command)
    local session = coroutine_session_id[co]
    if not ok then
        coroutine_session_id[co] = nil
        session_id_coroutine[session_id] = nil
        error('session not found')
    end
    if command == 'EXIT' then
        coroutine_session_id[co] = nil
        session_id_coroutine[session_id] = nil        
    end
end

-- dispatch inproc message
local function dispatch_rpc_message(from, data, service)
    print('co num', table.size(coroutine_session_id), table.size(session_id_coroutine))
    local msg = cmsgpack.unpack(data)
    if msg.method then --request from other service
        session_id = session_id + 1
        local co = co_create(function ()
            local func = service[msg.method]
            if func then
                local result = {func(table.unpack(msg.params))}
                if #result > 0 then
                    qsf.send(from, cmsgpack.pack({id=session_id, result=result}))
                end
            else
                qsf.send(from, cmsgpack.pack({id=session_id, error='method not found'}))
            end
        end)
        coroutine_session_id[co] = session_id
        session_id_coroutine[session_id] = co
        suspend(co, coroutine.resume(co))
    else --response for this service
        local co = session_id_coroutine[msg.id]
        if co then
            coroutine.resume(co, table.unpack(msg.result))
        else
            print('unknown response:', msg.id)
        end
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
        local ok, result = xpcall(dispatch_message, trace.dump_stack, service, func)
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