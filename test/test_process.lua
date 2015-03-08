local process = require 'process'

local function process_tick()
    local s1 = process.gettick()
    process.sleep(10)
    local s2 = process.gettick()
    assert(s2 >= s1)
    s1 = process.hrtime()
    process.sleep(10)
    s2 = process.hrtime()
    assert(s2 >= s1)
end

local function process_os()
    local os_name = process.os()
    print('os name:' .. os_name)
    assert(type(os_name) == 'string')
    local pid = process.pid()
    print('process id:' .. pid)
    assert(type(pid) == 'number')
    print('memory bytes:', process.total_memory())
end

local function procss_dir()
    local cwd = process.cwd()
    print('cwd: ' .. cwd)
    assert(type(cwd) == 'string')
    local exepath = process.exepath()
    print('exepath: ' .. exepath)
    assert(type(exepath) == 'string')
    
    process.chdir('../')
    assert(cwd ~= process.cwd())
    process.chdir(cwd)
end

process_tick()
process_os()
procss_dir()

print('process passed')