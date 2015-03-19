local uv = require 'uv'
local process = require 'process'

local function test_listen()
    local s = uv.new_tcp()
    s:simultaneous_accepts(true)
    s:simultaneous_accepts(false)
    s:bind('127.0.0.1', 10086)
    s:listen(128, function(err)
        print('client accepted')
        assert(not err, err)
        local c = s:accept()
        print(c:getsockname())
        print(c:getpeername())
        c:nodelay(true)
        c:nodelay(false)
        c:keepalive(true, 10)
        c:keepalive(false, 10)
        uv.stop()
    end)
    local c = uv.new_tcp()
    c:connect('127.0.0.1', 10086, function(err) 
        print('connected server')
    end)
    uv.run()
end

local function test_echo()
    local sessions = {}
    local s = uv.new_tcp()
    s:bind('127.0.0.1', 10086)
    s:listen(128, function(err)
        assert(not err)
        local c = s:accept()
        table.insert(sessions, c)
        c:read_start(function(err, data)
            print(err, data)
            if not err then 
                c:write('world')
            else
                uv.stop()
            end
        end)
    end)
    
    local c = uv.new_tcp()
    c:connect('127.0.0.1', 10086, function(err)
        assert(not err)
        c:write('hello')
        c:read_start(function(err, data)
            print(err, data)
            c:read_stop()
            c:close()
        end)
    end)
    
    uv.run()
end

test_listen()
test_echo()

print('tcp passed')