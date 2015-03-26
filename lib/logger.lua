--
-- log service
--
local mq = require 'mq'
local fs = require 'fs'
local mp = require 'MessagePack'
local dumpstring = require 'dump'.dumpstring

local print, assert, tostring, table, string, io, os
    = print, assert, tostring, table, string, io, os

    
local service = {}

local writer

function service.print(from, ...)
    local args = {... }
    for n=1, #args do
        args[n] = tostring(args[n]) .. ' '
    end
    local timestamp = os.date('%H:%M:%S')
    local text = string.format('%s %s: %s', timestamp, from, table.concat(args))
    writer(text)
end

local function dispatch_message()
    local from, data = mq.recv()
    assert(from and data)
    if from == 'sys' and data == 'exit' then
        mq.send(from, 'roger')
        return false
    else
        --print(from .. ' ==> ' .. mq.name(), data)
        local msg = mp.unpack(data)
        local func = service[msg.method]
        if func then
            func(from, table.unpack(msg.params))
        else
            print('method not found', msg.method)
        end
    end
    return true
end

local function main(args)
    print(dumpstring(args))
    if args[2] then -- log to file
        fs.mkdir('logs')
        local name = (args[2])
        writer = function(...)
            local filename = string.format('logs/%s_%s.log', name, os.date('%Y-%m-%d'))
            local fp = assert(io.open(filename, 'a+'))
            fp:write(...)
            fp:write('\n')
            fp:close()
        end
    else -- log to console
        writer = print
    end
    writer = print
    while dispatch_message() do
        -- nothing
    end
    print('logger quit')
end

main{...}
