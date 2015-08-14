-- Extensions to the string module

local string = require 'string'
local table = table

--- Split a string at a given separator.
function string.split(s, sep)
    local r, patt = {}
    if sep == '' then
        table.insert(r, '')
    else
        patt = '(.-)' .. (sep or '%s+')
    end
    local b, lens = 0, #s
    while b <= lens do
        local _, n, m = string.find (s, patt, b + 1)
        table.insert(r, m or s:sub (b + 1, lens))
        b = n or lens + 1
    end
    return r
end

function string.join(tbl, delim)
    return table.concat(tbl, delim)
end

--- Remove leading and trailing matter from a string.
function string.trim (s, r)
    r = r or '%s+'
    return (s:gsub ('^' .. r, ''):gsub (r .. '$', ''))
end

--- Remove leading matter from a string.
function string.ltrim(s, r)
    return (s:gsub ('^' .. (r or '%s+'), ''))
end

--- Remove trailing matter from a string.
function string.rtrim(s, r)
    return (s:gsub ((r or '%s+') .. '$', ''))
end

function string.startswith(str, s)
    return string.find(str, s) == 1
end

function string.endswith(str, s)
    if s == '' then return true end
    local init = #str - #s
    if init >= 0 then
        return str:find(s, init) == init + 1
    end
end