local net = require 'net'
local process = require 'process'

local host = '0.0.0.0'
local port = 10086
local config = 
{
    heart_beat = 15,
    heart_beat_check = 30,
    max_connection = 3000,
}

local function test_server_stop()
    local server = net.create_server()
    server:start(host, port, function(err, serial, data) end)
    server:stop()
end

local function read_cb(server, err, serial, data)
end

local function test_server_poll()
    local server = net.create_server(config)
    server:start(host, port, function(err, serial, data)
        read_cb(server, err, serial, data)
    end)

    while true do
        net.poll()
        process.sleep(10)
    end
end

local function main()
    test_server_stop()
    test_server_poll()
end

main()
