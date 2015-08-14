local node = require 'node'


local name, msg = node.recv()
node.send(name, msg)
