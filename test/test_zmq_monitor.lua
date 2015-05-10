local mq = require 'mq'
local zmq = require 'zmq'
local process = require 'process'

local bind_address = 'tcp://127.0.0.1:6666'
local monitor_address = 'inproc://monitor.rep'

local client = zmq.socket(zmq.REQ)
client:monitor(monitor_address, zmq.EVENT_ALL)

---
local m = zmq.socket(zmq.PAIR)
m:connect(monitor_address)

client:connect(bind_address)

print('Enter:')
local s = io.read()
client:send(s)

local function loop()
    local s = client:recv(zmq.DONTWAIT)
    if s then
        print('data:', s)
        client:send(s)
    end
end

local function monitor()
    local data = m:recv(zmq.DONTWAIT)
    if data then
        local event = string.unpack('<I2', data)
        local value = string.unpack('<I4', data, 3)
        print('event:', event, value)
        
        local address = m:recv()
        print('address:', address)
    end
end

while true do
    loop()
    monitor()
    process.sleep(10)
end
