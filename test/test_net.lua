local node = require 'node'
local net = require 'net'


local host = '0.0.0.0'
local port = 10086
local config = 
{
    heartbeat = 60,
    heartbeat_check = 15,
    max_connections = 3000,
}

local function read_cb(server, err, serial, data)
    print(err, serial, data)
    if not err then 
        server:write(serial, data)
    end
end

local function start_server()
    local server = net.createServer(config.max_connections, config.heartbeat, config.heartbeat_check)
    server:start(host, port, function(err, serial, data)
        read_cb(server, err, serial, data)
    end)
    print('server started')
end

local function main()
    start_server()
    while true do 
        node.run()
    end    
end

main()
