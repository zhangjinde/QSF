--
-- Stacktrace for xpcall
--

local dump = require 'dump'
local debug = debug
local string = string

local M = 
{
    need_upvalue = false,
    dumper = print, -- print to console by default
}

local function trace_level(level, dumper, record)
    local info = debug.getinfo(level)
    if not info or info.name == 'xpcall' then return end
    dumper(string.format('%s:%d: in function %s', info.short_src, 
        info.linedefined, info.name or info.func))
    dumper('Local:')
    local i = 1
    while true do
        local name, value = debug.getlocal(level, i)
        if not name then break end
        if name:byte(1) ~= 40 and value and not record[value] then -- not start with '('
            dumper(string.format('\t%s = %s', name, dump.dumpstring(value)))
            record[value] = true
        end
        i = i + 1
    end
    if M.need_upvalue then
        dumper('Upvalue:')
        local func = debug.getinfo(i).func
        i = 1
        while true do 
            local name, value = debug.getupvalue(func, i)
            if not name then break end
            if name:byte(1) ~= 40 and value and not record[value] then -- start with '('
                dumper(string.format('\t%s = %s', name, dump.dumpstring(value)))
                record[value] = true
            end
            i = i + 1
        end
    end
    return true
end

function M.dump_stack(errmsg)
    local dumper = M.dumper
    dumper(errmsg)
    dumper(debug.traceback())
    --[[
    local record = {}
    dumper('stack variables:\n')
    for level=3, 10 do
        if not trace_level(level, dumper, record) then
            break
        end
    end
    --]]
end

return M
