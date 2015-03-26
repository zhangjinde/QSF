-- Extensions to the string module

local string = require 'string'

local tinsert = table.insert
local strfind = string.find

--- Split a string at a given separator.
function string.split(s, sep)
    local r, patt = {}
    if sep == '' then
        patt = '(.)'
        tinsert(r, '')
    else
        patt = '(.-)' .. (sep or '%s+')
    end
    local b, lens = 0, #s
    while b <= lens do
        local e, n, m = strfind (s, patt, b + 1)
        tinsert(r, m or s:sub (b + 1, lens))
        b = n or lens + 1
    end
    return r
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

function string.start_with(str, s)
    return strfind(str, s) == 1
end
    