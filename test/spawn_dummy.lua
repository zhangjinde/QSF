local mq = require 'mq'
local process = require 'process'

local name, msg = mq.recv()
mq.send(name, msg)
