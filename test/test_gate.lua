local C = require 'qsf.c'
local gate = require 'gate'

local host = '127.0.0.1'
local port = 10086

local s = gate.create_server()
s:start(host, port, function(err, serial, data) 
    print(err, serial, data)
    if err == 0 then
        s:send(serial, data)
    end
end)

local c = gate.create_client()
c:connect(host, port)
c:send('hello,kitty')
c:read(function(data) 
    print('client->', data)
    c:send(data)
    C.sleep(200)
end)

while true do 
    gate.poll()
    C.sleep(100)
end    