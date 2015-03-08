--
-- testing case for uv timer
--

local process = require 'process'
local uv = require 'uv'

local global_counter = 0

local function timer_cb()
    global_counter = global_counter + 1
end

local function timer_once()
    global_counter = 0
    local timer = uv.new_timer()
    timer:start(200, 0, timer_cb)
    assert(timer:get_repeat() == 0)
    uv.run('once')
    assert(global_counter == 1)
    
    timer:start(200, 0, timer_cb)
    uv.run('once')
    assert(global_counter == 2)
end

local function timer_repeat()
    global_counter = 0
    local timer = uv.new_timer()
    timer:start(200, 200, timer_cb)
    assert(timer:get_repeat() == 200)
    uv.run('once')
    uv.run('once')
    assert(global_counter == 2)
end

timer_once()
timer_repeat()
print('timer test passed')