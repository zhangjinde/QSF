local uv = require 'luv'
local zmq = require 'zmq'


local address = 'tcp://127.0.0.1:5055'

local server = zmq.socket(zmq.ROUTER)
server:bind(address)

local client = zmq.socket(zmq.DEALER)
client:connect(address)
client:send('hello')

local count = 1
while true do 
    local ident = server:recv(zmq.DONTWAIT)
    if ident then 
        local req = server:recv()
        print('request:', req)
        if req == 'hello' then
            server:send(ident, zmq.SNDMORE)
            server:send('world')
        end
    end
    local res = client:recv(zmq.DONTWAIT)
    if res then 
        print('response', res)
        if res == 'world' then
            client:send('hello')
        end
        count = count + 1
        if count == 10 then 
            break
        end
    end
    uv.sleep(100)
end

print('Passed')
