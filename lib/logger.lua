--
-- logger service
--
local mq = require 'mq'
local fs = require 'fs'
local mp = require 'MessagePack'

local print, assert, ipairs, tostring, table, string, io, os
    = print, assert, ipairs, tostring, table, string, io, os

local mq_recv, mq_send, mp_unpack = mq.recv, mq.send, mp.unpack
local tunpack, tconcat, strformat, os_date 
    = table.unpack, table.concat, string.format, os.date
    
local service = {}

local writer

function service.print(from, ...)
    local args = {... }
    for n=1, #args do
        args[n] = tostring(args[n]) .. ' '
    end
    local timestamp = os_date('%H:%M:%S')
    local text = strformat('%s %s: %s', timestamp, from, tconcat(args))
    writer(text)
end

local function dispatch_message()
    local from, data = mq_recv()
    assert(from and data)
    if from == 'sys' and data == 'exit' then
        mq_send(from, 'roger')
        return false
    else
        --print(from .. ' ==> ' .. mq.name(), data)
        local msg = mp_unpack(data)
        local func = service[msg.method]
        if func then
            func(from, tunpack(msg.params))
        else
            print('method not found', msg.method)
        end
    end
    return true
end

local function main(args)
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
    --writer = print
    while dispatch_message() do
        -- nothing
    end
    print('logger quit')
end

main{...}
