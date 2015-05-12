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

local function process_memory()
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

local function process_uuid()
    local uuid1 = process.new_uuid()
    local uuid2 = process.new_uuid()
    assert(uuid1 ~= uuid2)
    print('uuid len', #uuid1)
end

process_tick()
process_memory()
procss_dir()
process_uuid()

print('process passed')