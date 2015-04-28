local net = require 'net'
local process = require 'process'

local host = '127.0.0.1'
local port = 10086

local function test_client()
    local c = net.create_client()
    c:connect(host, port, function(err)
        if not err then 
            c:write('hello,kitty')
            c:read(function(err, data) 
                if not err then 
                    c:send(data)
                end
            end)          
        end
    end) 
    while true do 
        net.poll()
        process.sleep(10)
    end
end

local function main()
    test_client()
end

main()
