local zmq = require 'zmq'

local public_key = [[E>-TgoMAf+R*WZ]8[NQ-tV>c1vN{WryLVrFkS8>y]]
local secret_key = [[4{DUIc#[BilXOmYjg[c#]uTD3J4>EoJVl===jx3M]]

local address = 'tcp://127.0.0.1:10086'
local monitor_addr = 'inproc://monitor.addr'

local function reply_server()
    local s = zmq.socket(zmq.REP)
    s:set_accept_filter('127.0.0.1')
    s:bind(address)
    while true do
        local str = s:recv(zmq.DONTWAIT)
        if str then 
            assert(str == 'hello')
            s:send('world')
            break
        end
    end
    return s
end

local function monitor_client()
    local c = zmq.socket(zmq.DEALER)
    c:monitor(monitor_addr, zmq.EVENT_ALL)
    local m = zmq.socket(zmq.PAIR)
    m:connect(monitor_addr)
    c:connect(address)
    c:send('hello')
    return c, m
end

local function curve_server()
    local s = zmq.socket(zmq.REP)
    s:set_curve_public_key(zmq.z85_decode(public_key))
    s:set_curve_secret_key(zmq.z85_decode(secret_key))
    s:set_curve_server(true)
    s:bind(address)
    local str = s:recv()
    assert(str == 'hello')
    s:send('world')
    return s
end

local function curve_client()
    local c = zmq.socket(zmq.REQ)
    local pub, secret = zmq.curve_keypair()
    c:set_curve_public_key(zmq.z85_decode(pub))
    c:set_curve_secret_key(zmq.z85_decode(secret))
    c:set_curve_server_key(public_key)
    c:connect(address)
    return c
end

local function test_curve()
    local c = curve_client()
    c:send('hello')
    local s = curve_server()
    assert(c:recv() == 'world')
    c:close()
    s:close()
end


test_curve()

print('zmq passed')

